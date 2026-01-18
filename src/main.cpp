#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <array>
#include <chrono>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/vulkan_context.hpp"
#include "core/buffer.hpp"
#include "core/pipeline.hpp"

class VulkanApp {
public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  GLFWwindow* window_ = nullptr;
  w3d::VulkanContext context_;
  w3d::Pipeline pipeline_;
  w3d::DescriptorManager descriptorManager_;
  w3d::VertexBuffer<w3d::Vertex> vertexBuffer_;
  w3d::IndexBuffer indexBuffer_;
  w3d::UniformBuffer<w3d::UniformBufferObject> uniformBuffers_;

  std::vector<vk::CommandBuffer> commandBuffers_;
  std::vector<vk::Semaphore> imageAvailableSemaphores_;
  std::vector<vk::Semaphore> renderFinishedSemaphores_;
  std::vector<vk::Fence> inFlightFences_;

  uint32_t currentFrame_ = 0;
  bool framebufferResized_ = false;

  static constexpr uint32_t WIDTH = 800;
  static constexpr uint32_t HEIGHT = 600;
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  // Cube vertices
  const std::vector<w3d::Vertex> cubeVertices_ = {
    // Front face (red)
    {{ -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
    {{  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
    {{  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }},
    {{ -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }},
    // Back face (green)
    {{ -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
    {{ -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }},
    {{  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }},
    {{  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
    // Top face (blue)
    {{ -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
    {{ -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
    {{  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
    {{  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
    // Bottom face (yellow)
    {{ -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }},
    {{  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }},
    {{  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f }},
    {{ -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 0.0f }},
    // Right face (magenta)
    {{  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }},
    {{  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f }},
    {{  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f }},
    {{  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }},
    // Left face (cyan)
    {{ -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 1.0f }},
    {{ -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f }},
    {{ -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f }},
    {{ -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f, 1.0f }},
  };

  const std::vector<uint32_t> cubeIndices_ = {
    0,  1,  2,   2,  3,  0,   // Front
    4,  5,  6,   6,  7,  4,   // Back
    8,  9,  10,  10, 11, 8,   // Top
    12, 13, 14,  14, 15, 12,  // Bottom
    16, 17, 18,  18, 19, 16,  // Right
    20, 21, 22,  22, 23, 20   // Left
  };

  static void framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {
    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    app->framebufferResized_ = true;
  }

  void initWindow() {
    if (!glfwInit()) {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window_ = glfwCreateWindow(WIDTH, HEIGHT, "W3D Viewer", nullptr, nullptr);
    if (!window_) {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
  }

  void initVulkan() {
    context_.init(window_, true);
    pipeline_.create(context_, "shaders/basic.vert.spv", "shaders/basic.frag.spv");

    vertexBuffer_.create(context_, cubeVertices_);
    indexBuffer_.create(context_, cubeIndices_);
    uniformBuffers_.create(context_, MAX_FRAMES_IN_FLIGHT);

    descriptorManager_.create(context_, pipeline_.descriptorSetLayout(), MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      descriptorManager_.updateUniformBuffer(
        i,
        uniformBuffers_.buffer(i),
        sizeof(w3d::UniformBufferObject)
      );
    }

    createCommandBuffers();
    createSyncObjects();
  }

  void createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo{
      context_.commandPool(),
      vk::CommandBufferLevel::ePrimary,
      MAX_FRAMES_IN_FLIGHT
    };

    commandBuffers_ = context_.device().allocateCommandBuffers(allocInfo);
  }

  void createSyncObjects() {
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semaphoreInfo{};
    vk::FenceCreateInfo fenceInfo{ vk::FenceCreateFlagBits::eSignaled };

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      imageAvailableSemaphores_[i] = context_.device().createSemaphore(semaphoreInfo);
      renderFinishedSemaphores_[i] = context_.device().createSemaphore(semaphoreInfo);
      inFlightFences_[i] = context_.device().createFence(fenceInfo);
    }
  }

  void updateUniformBuffer(uint32_t frameIndex) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
      currentTime - startTime).count();

    w3d::UniformBufferObject ubo{};
    ubo.model = glm::rotate(
      glm::mat4(1.0f),
      time * glm::radians(45.0f),
      glm::vec3(0.0f, 1.0f, 0.0f)
    );
    ubo.model = glm::rotate(
      ubo.model,
      time * glm::radians(30.0f),
      glm::vec3(1.0f, 0.0f, 0.0f)
    );
    ubo.view = glm::lookAt(
      glm::vec3(2.0f, 2.0f, 2.0f),
      glm::vec3(0.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, 1.0f, 0.0f)
    );

    auto extent = context_.swapchainExtent();
    ubo.proj = glm::perspective(
      glm::radians(45.0f),
      static_cast<float>(extent.width) / static_cast<float>(extent.height),
      0.1f,
      100.0f
    );
    ubo.proj[1][1] *= -1;  // Flip Y for Vulkan

    uniformBuffers_.update(frameIndex, ubo);
  }

  void recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex) {
    vk::CommandBufferBeginInfo beginInfo{};
    cmd.begin(beginInfo);

    auto extent = context_.swapchainExtent();

    // Clear values for color and depth attachments
    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].color = vk::ClearColorValue{ std::array<float, 4>{ 0.1f, 0.1f, 0.1f, 1.0f } };
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

    // Begin render pass (Vulkan 1.2 style)
    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.renderPass = context_.renderPass();
    renderPassInfo.framebuffer = context_.framebuffer(imageIndex);
    renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_.pipeline());

    vk::Viewport viewport{
      0.0f, 0.0f,
      static_cast<float>(extent.width), static_cast<float>(extent.height),
      0.0f, 1.0f
    };
    cmd.setViewport(0, viewport);

    vk::Rect2D scissor{ { 0, 0 }, extent };
    cmd.setScissor(0, scissor);

    vk::Buffer vertexBuffers[] = { vertexBuffer_.buffer() };
    vk::DeviceSize offsets[] = { 0 };
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(indexBuffer_.buffer(), 0, vk::IndexType::eUint32);

    cmd.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      pipeline_.layout(),
      0,
      descriptorManager_.descriptorSet(currentFrame_),
      {}
    );

    cmd.drawIndexed(indexBuffer_.indexCount(), 1, 0, 0, 0);

    cmd.endRenderPass();

    cmd.end();
  }

  void drawFrame() {
    auto device = context_.device();

    // Wait for previous frame
    auto waitResult = device.waitForFences(inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);
    if (waitResult != vk::Result::eSuccess) {
      throw std::runtime_error("Failed waiting for fence");
    }

    // Acquire next image
    uint32_t imageIndex;
    auto acquireResult = device.acquireNextImageKHR(
      context_.swapchain(),
      UINT64_MAX,
      imageAvailableSemaphores_[currentFrame_],
      nullptr
    );

    if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
      recreateSwapchain();
      return;
    } else if (acquireResult.result != vk::Result::eSuccess &&
               acquireResult.result != vk::Result::eSuboptimalKHR) {
      throw std::runtime_error("Failed to acquire swap chain image");
    }
    imageIndex = acquireResult.value;

    device.resetFences(inFlightFences_[currentFrame_]);

    // Update uniform buffer
    updateUniformBuffer(currentFrame_);

    // Record command buffer
    commandBuffers_[currentFrame_].reset();
    recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex);

    // Submit (using old API for compatibility)
    vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphores_[currentFrame_];
    submitInfo.pWaitDstStageMask = &waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[currentFrame_];

    context_.graphicsQueue().submit(submitInfo, inFlightFences_[currentFrame_]);

    // Present
    vk::SwapchainKHR swapchain = context_.swapchain();
    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores_[currentFrame_];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;

    auto presentResult = context_.presentQueue().presentKHR(presentInfo);

    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR ||
        framebufferResized_) {
      framebufferResized_ = false;
      recreateSwapchain();
    } else if (presentResult != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to present swap chain image");
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void recreateSwapchain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window_, &width, &height);
      glfwWaitEvents();
    }

    context_.device().waitIdle();
    context_.recreateSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
      drawFrame();
    }

    context_.device().waitIdle();
  }

  void cleanup() {
    auto device = context_.device();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      device.destroySemaphore(imageAvailableSemaphores_[i]);
      device.destroySemaphore(renderFinishedSemaphores_[i]);
      device.destroyFence(inFlightFences_[i]);
    }

    descriptorManager_.destroy();
    uniformBuffers_.destroy();
    indexBuffer_.destroy();
    vertexBuffer_.destroy();
    pipeline_.destroy();
    context_.cleanup();

    if (window_) {
      glfwDestroyWindow(window_);
    }
    glfwTerminate();
  }
};

int main() {
  VulkanApp app;

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
