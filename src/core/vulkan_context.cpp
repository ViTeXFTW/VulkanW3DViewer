#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>

#include "vulkan_context.hpp"

namespace w3d {

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackC(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, [[maybe_unused]] void *pUserData) {

  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "Validation: " << pCallbackData->pMessage << "\n";
  }
  return VK_FALSE;
}

VulkanContext::~VulkanContext() {
  cleanup();
}

void VulkanContext::init(GLFWwindow *window, bool enableValidation) {
  validationEnabled_ = enableValidation;

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  createInstance(enableValidation);
  createSurface(window);
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
  createImageViews();
  createDepthResources();
  createRenderPass();
  createFramebuffers();
  createCommandPool();
}

void VulkanContext::cleanup() {
  if (device_) {
    device_.waitIdle();

    cleanupSwapchain();

    if (renderPass_) {
      device_.destroyRenderPass(renderPass_);
      renderPass_ = nullptr;
    }

    if (commandPool_) {
      device_.destroyCommandPool(commandPool_);
      commandPool_ = nullptr;
    }

    device_.destroy();
    device_ = nullptr;
  }

  if (instance_) {
    if (surface_) {
      instance_.destroySurfaceKHR(surface_);
      surface_ = nullptr;
    }

    if (validationEnabled_ && debugMessenger_) {
      auto destroyFunc = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
          instance_.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
      if (destroyFunc) {
        destroyFunc(instance_, debugMessenger_, nullptr);
      }
      debugMessenger_ = nullptr;
    }

    instance_.destroy();
    instance_ = nullptr;
  }
}

void VulkanContext::cleanupSwapchain() {
  for (auto framebuffer : framebuffers_) {
    device_.destroyFramebuffer(framebuffer);
  }
  framebuffers_.clear();

  if (depthImageView_) {
    device_.destroyImageView(depthImageView_);
    depthImageView_ = nullptr;
  }
  if (depthImage_) {
    device_.destroyImage(depthImage_);
    depthImage_ = nullptr;
  }
  if (depthImageMemory_) {
    device_.freeMemory(depthImageMemory_);
    depthImageMemory_ = nullptr;
  }

  for (auto imageView : swapchainImageViews_) {
    device_.destroyImageView(imageView);
  }
  swapchainImageViews_.clear();

  if (swapchain_) {
    device_.destroySwapchainKHR(swapchain_);
    swapchain_ = nullptr;
  }
}

void VulkanContext::recreateSwapchain(uint32_t width, uint32_t height) {
  device_.waitIdle();
  cleanupSwapchain();
  createSwapchain(width, height);
  createImageViews();
  createDepthResources();
  createFramebuffers();
}

void VulkanContext::createInstance(bool enableValidation) {
  vk::ApplicationInfo appInfo{"W3D Viewer", VK_MAKE_VERSION(1, 0, 0), "W3D Engine",
                              VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_2};

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  if (!glfwExtensions) {
    throw std::runtime_error("Failed to get required GLFW extensions");
  }

  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
  std::vector<const char *> layers;

  if (enableValidation) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    layers.insert(layers.end(), validationLayers.begin(), validationLayers.end());
  }

  vk::InstanceCreateInfo createInfo{{}, &appInfo, layers, extensions};

  instance_ = vk::createInstance(createInfo);

  if (enableValidation) {
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallbackC;

    auto createFunc = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        instance_.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    if (createFunc) {
      VkDebugUtilsMessengerEXT messenger;
      createFunc(instance_, &debugCreateInfo, nullptr, &messenger);
      debugMessenger_ = messenger;
    }
  }
}

void VulkanContext::createSurface(GLFWwindow *window) {
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(instance_, window, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create window surface");
  }
  surface_ = surface;
}

void VulkanContext::pickPhysicalDevice() {
  auto devices = instance_.enumeratePhysicalDevices();
  if (devices.empty()) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      physicalDevice_ = device;
      break;
    }
  }

  if (!physicalDevice_) {
    throw std::runtime_error("Failed to find a suitable GPU");
  }

  auto props = physicalDevice_.getProperties();
  std::cout << "Selected GPU: " << props.deviceName << "\n";
}

bool VulkanContext::isDeviceSuitable(vk::PhysicalDevice device) {
  auto indices = findQueueFamilies(device);
  if (!indices.isComplete())
    return false;

  // Check extension support
  auto extensions = device.enumerateDeviceExtensionProperties();
  std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
  for (const auto &ext : extensions) {
    requiredExtensions.erase(ext.extensionName);
  }
  if (!requiredExtensions.empty())
    return false;

  // Check swapchain support
  auto swapchainSupport = querySwapchainSupport(device);
  if (swapchainSupport.formats.empty() || swapchainSupport.presentModes.empty()) {
    return false;
  }

  return true;
}

QueueFamilyIndices VulkanContext::findQueueFamilies(vk::PhysicalDevice device) {
  QueueFamilyIndices indices;
  auto queueFamilies = device.getQueueFamilyProperties();

  uint32_t i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphicsFamily = i;
    }

    if (device.getSurfaceSupportKHR(i, surface_)) {
      indices.presentFamily = i;
    }

    if (indices.isComplete())
      break;
    i++;
  }

  return indices;
}

void VulkanContext::createLogicalDevice() {
  queueFamilies_ = findQueueFamilies(physicalDevice_);

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {queueFamilies_.graphicsFamily.value(),
                                            queueFamilies_.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo queueCreateInfo{{}, queueFamily, 1, &queuePriority};
    queueCreateInfos.push_back(queueCreateInfo);
  }

  vk::PhysicalDeviceFeatures deviceFeatures{};

  vk::DeviceCreateInfo createInfo{{},
                                  queueCreateInfos,
                                  {}, // No device-level layers in modern Vulkan
                                  deviceExtensions,
                                  &deviceFeatures};

  device_ = physicalDevice_.createDevice(createInfo);

  graphicsQueue_ = device_.getQueue(queueFamilies_.graphicsFamily.value(), 0);
  presentQueue_ = device_.getQueue(queueFamilies_.presentFamily.value(), 0);
}

SwapchainSupportDetails VulkanContext::querySwapchainSupport(vk::PhysicalDevice device) {
  SwapchainSupportDetails details;
  details.capabilities = device.getSurfaceCapabilitiesKHR(surface_);
  details.formats = device.getSurfaceFormatsKHR(surface_);
  details.presentModes = device.getSurfacePresentModesKHR(surface_);
  return details;
}

vk::SurfaceFormatKHR
VulkanContext::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &formats) {
  for (const auto &format : formats) {
    if (format.format == vk::Format::eB8G8R8A8Srgb &&
        format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return format;
    }
  }
  return formats[0];
}

vk::PresentModeKHR
VulkanContext::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &modes) {
  for (const auto &mode : modes) {
    if (mode == vk::PresentModeKHR::eMailbox) {
      return mode;
    }
  }
  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanContext::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities,
                                             uint32_t width, uint32_t height) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  vk::Extent2D extent = {width, height};
  extent.width = std::clamp(extent.width, capabilities.minImageExtent.width,
                            capabilities.maxImageExtent.width);
  extent.height = std::clamp(extent.height, capabilities.minImageExtent.height,
                             capabilities.maxImageExtent.height);
  return extent;
}

void VulkanContext::createSwapchain(uint32_t width, uint32_t height) {
  auto support = querySwapchainSupport(physicalDevice_);
  auto surfaceFormat = chooseSwapSurfaceFormat(support.formats);
  auto presentMode = chooseSwapPresentMode(support.presentModes);
  auto extent = chooseSwapExtent(support.capabilities, width, height);

  uint32_t imageCount = support.capabilities.minImageCount + 1;
  if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
    imageCount = support.capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo{{},
                                        surface_,
                                        imageCount,
                                        surfaceFormat.format,
                                        surfaceFormat.colorSpace,
                                        extent,
                                        1,
                                        vk::ImageUsageFlagBits::eColorAttachment,
                                        vk::SharingMode::eExclusive,
                                        {},
                                        support.capabilities.currentTransform,
                                        vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                        presentMode,
                                        VK_TRUE};

  uint32_t queueFamilyIndices[] = {queueFamilies_.graphicsFamily.value(),
                                   queueFamilies_.presentFamily.value()};

  if (queueFamilies_.graphicsFamily != queueFamilies_.presentFamily) {
    createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }

  swapchain_ = device_.createSwapchainKHR(createInfo);
  swapchainImages_ = device_.getSwapchainImagesKHR(swapchain_);
  swapchainImageFormat_ = surfaceFormat.format;
  swapchainExtent_ = extent;
}

void VulkanContext::createImageViews() {
  swapchainImageViews_.resize(swapchainImages_.size());

  for (size_t i = 0; i < swapchainImages_.size(); i++) {
    vk::ImageViewCreateInfo createInfo{
        {},
        swapchainImages_[i],
        vk::ImageViewType::e2D,
        swapchainImageFormat_,
        {},
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    };
    swapchainImageViews_[i] = device_.createImageView(createInfo);
  }
}

vk::Format VulkanContext::findDepthFormat() {
  return findSupportedFormat(
      {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
      vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::Format VulkanContext::findSupportedFormat(const std::vector<vk::Format> &candidates,
                                              vk::ImageTiling tiling,
                                              vk::FormatFeatureFlags features) {
  for (auto format : candidates) {
    auto props = physicalDevice_.getFormatProperties(format);
    if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == vk::ImageTiling::eOptimal &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("Failed to find supported format");
}

void VulkanContext::createDepthResources() {
  depthFormat_ = findDepthFormat();

  vk::ImageCreateInfo imageInfo{
      {},
      vk::ImageType::e2D,
      depthFormat_,
      {swapchainExtent_.width, swapchainExtent_.height, 1},
      1,
      1,
      vk::SampleCountFlagBits::e1,
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment,
      vk::SharingMode::eExclusive
  };

  depthImage_ = device_.createImage(imageInfo);

  auto memRequirements = device_.getImageMemoryRequirements(depthImage_);
  vk::MemoryAllocateInfo allocInfo{
      memRequirements.size,
      findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)};

  depthImageMemory_ = device_.allocateMemory(allocInfo);
  device_.bindImageMemory(depthImage_, depthImageMemory_, 0);

  vk::ImageViewCreateInfo viewInfo{
      {},
      depthImage_, vk::ImageViewType::e2D,
      depthFormat_, {},
      {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}
  };

  depthImageView_ = device_.createImageView(viewInfo);
}

void VulkanContext::createRenderPass() {
  // Color attachment
  vk::AttachmentDescription colorAttachment{{},
                                            swapchainImageFormat_,
                                            vk::SampleCountFlagBits::e1,
                                            vk::AttachmentLoadOp::eClear,
                                            vk::AttachmentStoreOp::eStore,
                                            vk::AttachmentLoadOp::eDontCare,
                                            vk::AttachmentStoreOp::eDontCare,
                                            vk::ImageLayout::eUndefined,
                                            vk::ImageLayout::ePresentSrcKHR};

  // Depth attachment
  vk::AttachmentDescription depthAttachment{{},
                                            depthFormat_,
                                            vk::SampleCountFlagBits::e1,
                                            vk::AttachmentLoadOp::eClear,
                                            vk::AttachmentStoreOp::eDontCare,
                                            vk::AttachmentLoadOp::eDontCare,
                                            vk::AttachmentStoreOp::eDontCare,
                                            vk::ImageLayout::eUndefined,
                                            vk::ImageLayout::eDepthStencilAttachmentOptimal};

  vk::AttachmentReference colorAttachmentRef{0, vk::ImageLayout::eColorAttachmentOptimal};

  vk::AttachmentReference depthAttachmentRef{1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

  vk::SubpassDescription subpass{
      {}, vk::PipelineBindPoint::eGraphics, {}, colorAttachmentRef, {}, &depthAttachmentRef};

  // Dependency to ensure external operations complete before we start
  vk::SubpassDependency dependency{VK_SUBPASS_EXTERNAL,
                                   0,
                                   vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                       vk::PipelineStageFlagBits::eEarlyFragmentTests,
                                   vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                       vk::PipelineStageFlagBits::eEarlyFragmentTests,
                                   {},
                                   vk::AccessFlagBits::eColorAttachmentWrite |
                                       vk::AccessFlagBits::eDepthStencilAttachmentWrite};

  std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

  vk::RenderPassCreateInfo renderPassInfo{{}, attachments, subpass, dependency};

  renderPass_ = device_.createRenderPass(renderPassInfo);
}

void VulkanContext::createFramebuffers() {
  framebuffers_.resize(swapchainImageViews_.size());

  for (size_t i = 0; i < swapchainImageViews_.size(); i++) {
    std::array<vk::ImageView, 2> attachments = {swapchainImageViews_[i], depthImageView_};

    vk::FramebufferCreateInfo framebufferInfo{
        {}, renderPass_, attachments, swapchainExtent_.width, swapchainExtent_.height, 1};

    framebuffers_[i] = device_.createFramebuffer(framebufferInfo);
  }
}

void VulkanContext::createCommandPool() {
  vk::CommandPoolCreateInfo poolInfo{vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                     queueFamilies_.graphicsFamily.value()};
  commandPool_ = device_.createCommandPool(poolInfo);
}

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
  auto memProperties = physicalDevice_.getMemoryProperties();
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("Failed to find suitable memory type");
}

vk::CommandBuffer VulkanContext::beginSingleTimeCommands() {
  vk::CommandBufferAllocateInfo allocInfo{commandPool_, vk::CommandBufferLevel::ePrimary, 1};

  auto commandBuffers = device_.allocateCommandBuffers(allocInfo);
  vk::CommandBuffer commandBuffer = commandBuffers[0];

  vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
  commandBuffer.begin(beginInfo);

  return commandBuffer;
}

void VulkanContext::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
  commandBuffer.end();

  vk::SubmitInfo submitInfo{};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  graphicsQueue_.submit(submitInfo);
  graphicsQueue_.waitIdle();

  device_.freeCommandBuffers(commandPool_, commandBuffer);
}

} // namespace w3d
