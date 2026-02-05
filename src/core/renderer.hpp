#pragma once

#include "lib/gfx/buffer.hpp"
#include "lib/gfx/pipeline.hpp"
#include "lib/gfx/vulkan_context.hpp"

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <cstdint>
#include <vector>

#include "core/render_state.hpp"
#include "render/bone_buffer.hpp"
#include "lib/gfx/camera.hpp"
#include "lib/formats/w3d/hlod_model.hpp"
#include "render/hover_detector.hpp"
#include "render/material.hpp"
#include "render/renderable_mesh.hpp"
#include "render/skeleton_renderer.hpp"
#include "lib/gfx/texture.hpp"
#include "ui/imgui_backend.hpp"

namespace w3d {

/**
 * Context object that bundles all data needed for rendering a frame.
 * This reduces coupling by grouping related parameters together.
 */
struct FrameContext {
  gfx::Camera &camera;
  RenderableMesh &renderableMesh;
  HLodModel &hlodModel;
  SkeletonRenderer &skeletonRenderer;
  const HoverDetector &hoverDetector;
  const RenderState &renderState;
};

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
  void init(GLFWwindow *window, gfx::VulkanContext &context, ImGuiBackend &imguiBackend,
            gfx::TextureManager &textureManager, BoneMatrixBuffer &boneMatrixBuffer);

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
  void drawFrame(const FrameContext &ctx);

  /**
   * Mark framebuffer as resized.
   */
  void setFramebufferResized(bool resized) { framebufferResized_ = resized; }

  /**
   * Get current frame index for double-buffered resources.
   */
  uint32_t currentFrame() const { return currentFrame_; }

  // Accessors
  gfx::Pipeline &pipeline() { return pipeline_; }
  gfx::Pipeline &skinnedPipeline() { return skinnedPipeline_; }
  gfx::DescriptorManager &descriptorManager() { return descriptorManager_; }
  gfx::SkinnedDescriptorManager &skinnedDescriptorManager() { return skinnedDescriptorManager_; }

private:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  void createCommandBuffers();
  void createSyncObjects();
  void updateUniformBuffer(uint32_t frameIndex, const gfx::Camera &camera);
  void recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex, const FrameContext &ctx);

  // External resources (not owned)
  GLFWwindow *window_ = nullptr;
  gfx::VulkanContext *context_ = nullptr;
  ImGuiBackend *imguiBackend_ = nullptr;
  gfx::TextureManager *textureManager_ = nullptr;
  BoneMatrixBuffer *boneMatrixBuffer_ = nullptr;

  // Pipelines and descriptors
  gfx::Pipeline pipeline_;
  gfx::Pipeline skinnedPipeline_;
  gfx::DescriptorManager descriptorManager_;
  gfx::SkinnedDescriptorManager skinnedDescriptorManager_;
  gfx::UniformBuffer<gfx::UniformBufferObject> uniformBuffers_;

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
