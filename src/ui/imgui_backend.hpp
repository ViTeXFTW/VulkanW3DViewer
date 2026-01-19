#pragma once

#include <vulkan/vulkan.hpp>

#include <functional>

struct GLFWwindow;

namespace w3d {

class VulkanContext;

class ImGuiBackend {
public:
  ImGuiBackend() = default;
  ~ImGuiBackend();

  ImGuiBackend(const ImGuiBackend &) = delete;
  ImGuiBackend &operator=(const ImGuiBackend &) = delete;

  // Initialize ImGui with Vulkan and GLFW
  void init(GLFWwindow *window, VulkanContext &context);

  // Cleanup resources
  void cleanup();

  // Begin a new ImGui frame
  void newFrame();

  // Render ImGui draw data
  void render(vk::CommandBuffer cmd);

  // Handle swapchain recreation
  void onSwapchainRecreate();

private:
  void createDescriptorPool(VulkanContext &context);

  VulkanContext *context_ = nullptr;
  vk::DescriptorPool descriptorPool_;
  bool initialized_ = false;
};

} // namespace w3d
