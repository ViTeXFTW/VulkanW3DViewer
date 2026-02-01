#include "renderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <stdexcept>

namespace w3d {

void Renderer::init(GLFWwindow *window, VulkanContext &context, ImGuiBackend &imguiBackend,
                    TextureManager &textureManager, BoneMatrixBuffer &boneMatrixBuffer) {
  window_ = window;
  context_ = &context;
  imguiBackend_ = &imguiBackend;
  textureManager_ = &textureManager;
  boneMatrixBuffer_ = &boneMatrixBuffer;

  // Create pipelines
  pipeline_.create(context, "shaders/basic.vert.spv", "shaders/basic.frag.spv");
  skinnedPipeline_.createSkinned(context, "shaders/skinned.vert.spv", "shaders/basic.frag.spv");

  // Create uniform buffers
  uniformBuffers_.create(context, MAX_FRAMES_IN_FLIGHT);

  // Create descriptor managers
  descriptorManager_.create(context, pipeline_.descriptorSetLayout(), MAX_FRAMES_IN_FLIGHT);
  skinnedDescriptorManager_.create(context, skinnedPipeline_.descriptorSetLayout(),
                                   MAX_FRAMES_IN_FLIGHT);

  // Get default texture for descriptor binding
  const auto &defaultTex = textureManager.texture(0);

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    descriptorManager_.updateUniformBuffer(i, uniformBuffers_.buffer(i),
                                           sizeof(UniformBufferObject));
    descriptorManager_.updateTexture(i, defaultTex.view, defaultTex.sampler);

    // Initialize skinned descriptor manager
    skinnedDescriptorManager_.updateUniformBuffer(i, uniformBuffers_.buffer(i),
                                                  sizeof(UniformBufferObject));
    skinnedDescriptorManager_.updateBoneBuffer(i, boneMatrixBuffer.buffer(i),
                                               sizeof(glm::mat4) * BoneMatrixBuffer::MAX_BONES);
  }

  // Create default material
  defaultMaterial_ = createDefaultMaterial();

  // Create command buffers and sync objects
  createCommandBuffers();
  createSyncObjects();
}

void Renderer::cleanup() {
  auto device = context_->device();

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    device.destroySemaphore(imageAvailableSemaphores_[i]);
    device.destroySemaphore(renderFinishedSemaphores_[i]);
    device.destroyFence(inFlightFences_[i]);
  }

  skinnedDescriptorManager_.destroy();
  descriptorManager_.destroy();
  uniformBuffers_.destroy();
  skinnedPipeline_.destroy();
  pipeline_.destroy();
}

void Renderer::createCommandBuffers() {
  vk::CommandBufferAllocateInfo allocInfo{context_->commandPool(), vk::CommandBufferLevel::ePrimary,
                                          MAX_FRAMES_IN_FLIGHT};

  commandBuffers_ = context_->device().allocateCommandBuffers(allocInfo);
}

void Renderer::createSyncObjects() {
  imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

  vk::SemaphoreCreateInfo semaphoreInfo{};
  vk::FenceCreateInfo fenceInfo{vk::FenceCreateFlagBits::eSignaled};

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    imageAvailableSemaphores_[i] = context_->device().createSemaphore(semaphoreInfo);
    renderFinishedSemaphores_[i] = context_->device().createSemaphore(semaphoreInfo);
    inFlightFences_[i] = context_->device().createFence(fenceInfo);
  }
}

void Renderer::updateUniformBuffer(uint32_t frameIndex, const Camera &camera) {
  UniformBufferObject ubo{};

  // Always use camera-based view
  ubo.model = glm::mat4(1.0f);
  ubo.view = camera.viewMatrix();

  auto extent = context_->swapchainExtent();
  ubo.proj = glm::perspective(glm::radians(45.0f),
                              static_cast<float>(extent.width) / static_cast<float>(extent.height),
                              0.01f, 10000.0f);
  ubo.proj[1][1] *= -1; // Flip Y for Vulkan

  uniformBuffers_.update(frameIndex, ubo);
}

void Renderer::recreateSwapchain(int width, int height) {
  context_->device().waitIdle();
  context_->recreateSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
  imguiBackend_->onSwapchainRecreate();
}

void Renderer::recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex,
                                   const FrameContext &ctx) {
  vk::CommandBufferBeginInfo beginInfo{};
  cmd.begin(beginInfo);

  auto extent = context_->swapchainExtent();

  // Clear values for color and depth attachments
  std::array<vk::ClearValue, 2> clearValues{};
  clearValues[0].color = vk::ClearColorValue{
      std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f}
  };
  clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

  // Begin render pass
  vk::RenderPassBeginInfo renderPassInfo{};
  renderPassInfo.renderPass = context_->renderPass();
  renderPassInfo.framebuffer = context_->framebuffer(imageIndex);
  renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
  renderPassInfo.renderArea.extent = extent;
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

  // Draw 3D content
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_.pipeline());

  vk::Viewport viewport{
      0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
  cmd.setViewport(0, viewport);

  vk::Rect2D scissor{
      {0, 0},
      extent
  };
  cmd.setScissor(0, scissor);

  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_.layout(), 0,
                         descriptorManager_.descriptorSet(currentFrame_), {});

  // Draw loaded mesh (either HLod model or simple renderable mesh)
  if (ctx.renderState.showMesh) {
    if (ctx.renderState.useHLodModel && ctx.hlodModel.hasData()) {
      if (ctx.renderState.useSkinnedRendering && ctx.hlodModel.hasSkinning()) {
        // Draw with skinned pipeline (GPU skinning)
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, skinnedPipeline_.pipeline());

        ctx.hlodModel.drawSkinnedWithTextures(cmd, [&](const std::string &textureName) {
          MaterialPushConstant materialData{};
          materialData.diffuseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
          materialData.emissiveColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
          materialData.specularColor = glm::vec4(0.2f, 0.2f, 0.2f, 32.0f);
          materialData.hoverTint = glm::vec3(1.0f); // No tint for HLod (not yet implemented)
          materialData.flags = 0;
          materialData.alphaThreshold = 0.5f;

          // Look up texture by name
          uint32_t texIdx = 0;
          if (!textureName.empty()) {
            texIdx = textureManager_->findTexture(textureName);
          }

          if (texIdx > 0) {
            const auto &tex = textureManager_->texture(texIdx);
            vk::DescriptorSet texDescSet = skinnedDescriptorManager_.getDescriptorSet(
                currentFrame_, texIdx, tex.view, tex.sampler, boneMatrixBuffer_->buffer(currentFrame_),
                sizeof(glm::mat4) * BoneMatrixBuffer::MAX_BONES);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skinnedPipeline_.layout(), 0,
                                   texDescSet, {});
            materialData.useTexture = 1;
          } else {
            const auto &defaultTex = textureManager_->texture(0);
            vk::DescriptorSet defaultDescSet = skinnedDescriptorManager_.getDescriptorSet(
                currentFrame_, 0, defaultTex.view, defaultTex.sampler, boneMatrixBuffer_->buffer(currentFrame_),
                sizeof(glm::mat4) * BoneMatrixBuffer::MAX_BONES);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skinnedPipeline_.layout(), 0,
                                   defaultDescSet, {});
            materialData.useTexture = 0;
          }

          cmd.pushConstants(skinnedPipeline_.layout(), vk::ShaderStageFlagBits::eFragment, 0,
                            sizeof(MaterialPushConstant), &materialData);
        });

        // Switch back to regular pipeline for skeleton overlay
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_.pipeline());
      } else {
        // Draw with regular pipeline (CPU-transformed vertices)
        ctx.hlodModel.drawWithTextures(cmd, [&](const std::string &textureName) {
          MaterialPushConstant materialData{};
          materialData.diffuseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
          materialData.emissiveColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
          materialData.specularColor = glm::vec4(0.2f, 0.2f, 0.2f, 32.0f);
          materialData.hoverTint = glm::vec3(1.0f); // No tint for HLod (not yet implemented)
          materialData.flags = 0;
          materialData.alphaThreshold = 0.5f;

          // Look up texture by name
          uint32_t texIdx = 0;
          if (!textureName.empty()) {
            texIdx = textureManager_->findTexture(textureName);
          }

          if (texIdx > 0) {
            // Get pre-allocated descriptor set for this texture
            const auto &tex = textureManager_->texture(texIdx);
            vk::DescriptorSet texDescSet = descriptorManager_.getTextureDescriptorSet(
                currentFrame_, texIdx, tex.view, tex.sampler);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_.layout(), 0,
                                   texDescSet, {});
            materialData.useTexture = 1;
          } else {
            // Use default texture descriptor set
            const auto &defaultTex = textureManager_->texture(0);
            vk::DescriptorSet defaultDescSet = descriptorManager_.getTextureDescriptorSet(
                currentFrame_, 0, defaultTex.view, defaultTex.sampler);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_.layout(), 0,
                                   defaultDescSet, {});
            materialData.useTexture = 0;
          }

          cmd.pushConstants(pipeline_.layout(), vk::ShaderStageFlagBits::eFragment, 0,
                            sizeof(MaterialPushConstant), &materialData);
        });
      }
    } else if (ctx.renderableMesh.hasData()) {
      // Simple mesh without textures
      MaterialPushConstant materialData{};
      materialData.diffuseColor = glm::vec4(defaultMaterial_.diffuse, defaultMaterial_.opacity);
      materialData.emissiveColor = glm::vec4(defaultMaterial_.emissive, 1.0f);
      materialData.specularColor = glm::vec4(defaultMaterial_.specular, defaultMaterial_.shininess);
      materialData.flags = 0;
      materialData.alphaThreshold = 0.5f;
      materialData.useTexture = 0;

      // Use hover detection for simple meshes
      const glm::vec3 hoverTint(1.5f, 1.5f, 1.3f); // Warm highlight
      const auto &hover = ctx.hoverDetector.state();

      ctx.renderableMesh.drawWithHover(
          cmd, hover.type == HoverType::Mesh ? static_cast<int>(hover.objectIndex) : -1,
          hoverTint, [&](size_t /*meshIndex*/, const glm::vec3 &tint) {
            materialData.hoverTint = tint;
            cmd.pushConstants(pipeline_.layout(), vk::ShaderStageFlagBits::eFragment, 0,
                              sizeof(MaterialPushConstant), &materialData);
          });
    }
  }

  // Draw skeleton overlay
  if (ctx.renderState.showSkeleton && ctx.skeletonRenderer.hasData()) {
    // Skeleton uses same descriptor set layout, so we can reuse the bound descriptor
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, ctx.skeletonRenderer.pipelineLayout(), 0,
                           descriptorManager_.descriptorSet(currentFrame_), {});

    // Apply hover tint if hovering over skeleton
    const glm::vec3 hoverTint(1.5f, 1.5f, 1.3f); // Warm highlight
    const auto &hover = ctx.hoverDetector.state();
    glm::vec3 skeletonTint =
        (hover.type == HoverType::Bone || hover.type == HoverType::Joint) ? hoverTint
                                                                           : glm::vec3(1.0f);

    ctx.skeletonRenderer.drawWithHover(cmd, currentFrame_, skeletonTint);
  }

  // Draw ImGui
  imguiBackend_->render(cmd);

  cmd.endRenderPass();
  cmd.end();
}

void Renderer::waitForCurrentFrame() {
  if (frameWaited_) {
    return; // Already waited this frame
  }

  auto device = context_->device();
  auto waitResult = device.waitForFences(inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);
  if (waitResult != vk::Result::eSuccess) {
    throw std::runtime_error("Failed waiting for fence");
  }

  frameWaited_ = true;
}

void Renderer::drawFrame(const FrameContext &ctx) {
  auto device = context_->device();

  // Wait for previous frame (skipped if waitForCurrentFrame() was already called)
  waitForCurrentFrame();

  // Acquire next image
  uint32_t imageIndex;
  auto acquireResult = device.acquireNextImageKHR(
      context_->swapchain(), UINT64_MAX, imageAvailableSemaphores_[currentFrame_], nullptr);

  if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    recreateSwapchain(width, height);
    return;
  } else if (acquireResult.result != vk::Result::eSuccess &&
             acquireResult.result != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("Failed to acquire swap chain image");
  }
  imageIndex = acquireResult.value;

  device.resetFences(inFlightFences_[currentFrame_]);

  // Update uniform buffer
  updateUniformBuffer(currentFrame_, ctx.camera);

  // Record command buffer
  commandBuffers_[currentFrame_].reset();
  recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex, ctx);

  // Submit
  vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  vk::SubmitInfo submitInfo{};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &imageAvailableSemaphores_[currentFrame_];
  submitInfo.pWaitDstStageMask = &waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[currentFrame_];

  context_->graphicsQueue().submit(submitInfo, inFlightFences_[currentFrame_]);

  // Present
  vk::SwapchainKHR swapchain = context_->swapchain();
  vk::PresentInfoKHR presentInfo{};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &renderFinishedSemaphores_[currentFrame_];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = &imageIndex;

  auto presentResult = context_->presentQueue().presentKHR(presentInfo);

  if (presentResult == vk::Result::eErrorOutOfDateKHR ||
      presentResult == vk::Result::eSuboptimalKHR || framebufferResized_) {
    framebufferResized_ = false;
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    recreateSwapchain(width, height);
  } else if (presentResult != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to present swap chain image");
  }

  currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
  frameWaited_ = false; // Reset for next frame
}

} // namespace w3d
