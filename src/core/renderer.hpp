#pragma once

#include "core/buffer.hpp"
#include "core/pipeline.hpp"
#include "core/render_state.hpp"
#include "core/vulkan_context.hpp"

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <cstdint>
#include <vector>

#include "render/bone_buffer.hpp"
#include "render/camera.hpp"
#include "render/hlod_model.hpp"
#include "render/hover_detector.hpp"
#include "render/material.hpp"
#include "render/renderable_mesh.hpp"
#include "render/skeleton_renderer.hpp"
#include "render/texture.hpp"
#include "ui/imgui_backend.hpp"

namespace w3d {

/**
 * Manages all Vulkan rendering operations including command buffers,
 * pipelines, and frame rendering.
 */
class Renderer {
public:
  Renderer() = default;
  ~Renderer() = default;

  // Non-copyable
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  /**
   * Initialize the renderer with Vulkan context and window.
   */
  void init(GLFWwindow *window, VulkanContext &context, ImGuiBackend &imguiBackend,
            TextureManager &textureManager, BoneMatrixBuffer &boneMatrixBuffer);

  /**
   * Clean up rendering resources.
   */
  void cleanup();

  /**
   * Recreate swapchain when window is resized.
   */
  void recreateSwapchain(int width, int height);

  /**
   * Wait for the current frame's fence to be signaled.
   * Call this before updating any per-frame resources (e.g., bone matrices)
   * to ensure the GPU is done reading from that frame's buffers.
   */
  void waitForCurrentFrame();

  /**
   * Draw a single frame. Call waitForCurrentFrame() first if you need to
   * update per-frame resources before drawing.
   */
  void drawFrame(Camera &camera, RenderableMesh &renderableMesh, HLodModel &hlodModel,
                 SkeletonRenderer &skeletonRenderer, const HoverDetector &hoverDetector,
                 const RenderState &renderState);

  /**
   * Mark framebuffer as resized.
   */
  void setFramebufferResized(bool resized) { framebufferResized_ = resized; }

  /**
   * Get current frame index for double-buffered resources.
   */
  uint32_t currentFrame() const { return currentFrame_; }

  // Accessors
  Pipeline &pipeline() { return pipeline_; }
  Pipeline &skinnedPipeline() { return skinnedPipeline_; }
  DescriptorManager &descriptorManager() { return descriptorManager_; }
  SkinnedDescriptorManager &skinnedDescriptorManager() { return skinnedDescriptorManager_; }

private:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  void createCommandBuffers();
  void createSyncObjects();
  void updateUniformBuffer(uint32_t frameIndex, const Camera &camera);
  void recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex, RenderableMesh &renderableMesh,
                           HLodModel &hlodModel, SkeletonRenderer &skeletonRenderer,
                           const HoverDetector &hoverDetector, const RenderState &renderState);

  // External resources (not owned)
  GLFWwindow *window_ = nullptr;
  VulkanContext *context_ = nullptr;
  ImGuiBackend *imguiBackend_ = nullptr;
  TextureManager *textureManager_ = nullptr;
  BoneMatrixBuffer *boneMatrixBuffer_ = nullptr;

  // Pipelines and descriptors
  Pipeline pipeline_;
  Pipeline skinnedPipeline_;
  DescriptorManager descriptorManager_;
  SkinnedDescriptorManager skinnedDescriptorManager_;
  UniformBuffer<UniformBufferObject> uniformBuffers_;

  // Command buffers and synchronization
  std::vector<vk::CommandBuffer> commandBuffers_;
  std::vector<vk::Semaphore> imageAvailableSemaphores_;
  std::vector<vk::Semaphore> renderFinishedSemaphores_;
  std::vector<vk::Fence> inFlightFences_;

  uint32_t currentFrame_ = 0;
  bool framebufferResized_ = false;
  bool frameWaited_ = false; // Track if waitForCurrentFrame() was called this frame

  // Default material
  Material defaultMaterial_;
};

} // namespace w3d
