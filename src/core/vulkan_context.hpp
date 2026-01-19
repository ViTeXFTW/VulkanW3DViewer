#pragma once

#include <optional>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace w3d {

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapchainSupportDetails {
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;
};

class VulkanContext {
public:
  VulkanContext() = default;
  ~VulkanContext();

  VulkanContext(const VulkanContext &) = delete;
  VulkanContext &operator=(const VulkanContext &) = delete;

  void init(GLFWwindow *window, bool enableValidation = true);
  void cleanup();

  void recreateSwapchain(uint32_t width, uint32_t height);

  // Accessors
  vk::Instance instance() const { return instance_; }
  vk::PhysicalDevice physicalDevice() const { return physicalDevice_; }
  vk::Device device() const { return device_; }
  vk::Queue graphicsQueue() const { return graphicsQueue_; }
  vk::Queue presentQueue() const { return presentQueue_; }
  vk::SurfaceKHR surface() const { return surface_; }
  vk::SwapchainKHR swapchain() const { return swapchain_; }
  vk::Format swapchainImageFormat() const { return swapchainImageFormat_; }
  vk::Extent2D swapchainExtent() const { return swapchainExtent_; }
  const std::vector<vk::ImageView> &swapchainImageViews() const { return swapchainImageViews_; }
  const std::vector<vk::Image> &swapchainImages() const { return swapchainImages_; }
  vk::ImageView depthImageView() const { return depthImageView_; }
  vk::Image depthImage() const { return depthImage_; }
  vk::Format depthFormat() const { return depthFormat_; }
  vk::CommandPool commandPool() const { return commandPool_; }
  uint32_t graphicsQueueFamily() const { return queueFamilies_.graphicsFamily.value(); }
  vk::RenderPass renderPass() const { return renderPass_; }
  vk::Framebuffer framebuffer(uint32_t index) const { return framebuffers_[index]; }

  // Helper for one-time commands
  vk::CommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

  uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

private:
  void createInstance(bool enableValidation);
  void createSurface(GLFWwindow *window);
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapchain(uint32_t width, uint32_t height);
  void createImageViews();
  void createDepthResources();
  void createRenderPass();
  void createFramebuffers();
  void createCommandPool();

  void cleanupSwapchain();

  QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
  SwapchainSupportDetails querySwapchainSupport(vk::PhysicalDevice device);
  bool isDeviceSuitable(vk::PhysicalDevice device);
  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &formats);
  vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &modes);
  vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities, uint32_t width,
                                uint32_t height);
  vk::Format findDepthFormat();
  vk::Format findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
                                 vk::FormatFeatureFlags features);

  vk::Instance instance_;
  vk::DebugUtilsMessengerEXT debugMessenger_;
  vk::SurfaceKHR surface_;
  vk::PhysicalDevice physicalDevice_;
  vk::Device device_;
  vk::Queue graphicsQueue_;
  vk::Queue presentQueue_;
  QueueFamilyIndices queueFamilies_;

  vk::SwapchainKHR swapchain_;
  std::vector<vk::Image> swapchainImages_;
  std::vector<vk::ImageView> swapchainImageViews_;
  vk::Format swapchainImageFormat_;
  vk::Extent2D swapchainExtent_;

  vk::Image depthImage_;
  vk::DeviceMemory depthImageMemory_;
  vk::ImageView depthImageView_;
  vk::Format depthFormat_;

  vk::CommandPool commandPool_;

  vk::RenderPass renderPass_;
  std::vector<vk::Framebuffer> framebuffers_;

  bool validationEnabled_ = false;

  static constexpr std::array<const char *, 1> validationLayers = {"VK_LAYER_KHRONOS_validation"};

  static constexpr std::array<const char *, 1> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

} // namespace w3d
