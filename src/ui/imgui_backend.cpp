#include "imgui_backend.hpp"

#include "lib/gfx/vulkan_context.hpp"

#include "core/app_paths.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace w3d {

// Using declarations for gfx types
using gfx::VulkanContext;

ImGuiBackend::~ImGuiBackend() {
  cleanup();
}

void ImGuiBackend::init(GLFWwindow *window, gfx::VulkanContext &context) {
  context_ = &context;

  // Create descriptor pool for ImGui
  createDescriptorPool(context);

  // Setup ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  // Set custom ImGui.ini path in app data directory
  // Static storage ensures the string outlives ImGui's usage
  static std::string imguiIniPath;
  if (auto path = AppPaths::imguiIniPath()) {
    AppPaths::ensureAppDataDir();
    imguiIniPath = path->string();
    io.IniFilename = imguiIniPath.c_str();
  } else {
    // Disable ini persistence if path determination fails
    io.IniFilename = nullptr;
  }

  // Enable docking
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // Enable keyboard navigation
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Setup style
  ImGui::StyleColorsDark();

  // Initialize GLFW backend
  ImGui_ImplGlfw_InitForVulkan(window, true);

  // Initialize Vulkan backend
  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.ApiVersion = VK_API_VERSION_1_2;
  initInfo.Instance = context.instance();
  initInfo.PhysicalDevice = context.physicalDevice();
  initInfo.Device = context.device();
  initInfo.QueueFamily = context.graphicsQueueFamily();
  initInfo.Queue = context.graphicsQueue();
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = descriptorPool_;
  initInfo.MinImageCount = 2;
  initInfo.ImageCount = static_cast<uint32_t>(context.swapchainImages().size());
  initInfo.UseDynamicRendering = false;

  // Pipeline info for render pass
  initInfo.PipelineInfoMain.RenderPass = context.renderPass();
  initInfo.PipelineInfoMain.Subpass = 0;
  initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&initInfo);

  initialized_ = true;
}

void ImGuiBackend::cleanup() {
  if (!initialized_)
    return;

  if (context_) {
    context_->device().waitIdle();
  }

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  if (context_ && descriptorPool_) {
    context_->device().destroyDescriptorPool(descriptorPool_);
    descriptorPool_ = nullptr;
  }

  initialized_ = false;
}

void ImGuiBackend::newFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiBackend::render(vk::CommandBuffer cmd) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void ImGuiBackend::onSwapchainRecreate() {
  // ImGui handles this internally, but we may need to update image count
  // For now, no action needed as we recreate the full context
}

void ImGuiBackend::createDescriptorPool(VulkanContext &context) {
  // Create a descriptor pool large enough for ImGui
  std::array<vk::DescriptorPoolSize, 11> poolSizes = {
      {
       {vk::DescriptorType::eSampler, 1000},
       {vk::DescriptorType::eCombinedImageSampler, 1000},
       {vk::DescriptorType::eSampledImage, 1000},
       {vk::DescriptorType::eStorageImage, 1000},
       {vk::DescriptorType::eUniformTexelBuffer, 1000},
       {vk::DescriptorType::eStorageTexelBuffer, 1000},
       {vk::DescriptorType::eUniformBuffer, 1000},
       {vk::DescriptorType::eStorageBuffer, 1000},
       {vk::DescriptorType::eUniformBufferDynamic, 1000},
       {vk::DescriptorType::eStorageBufferDynamic, 1000},
       {vk::DescriptorType::eInputAttachment, 1000},
       }
  };

  vk::DescriptorPoolCreateInfo poolInfo{};
  poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
  poolInfo.maxSets = 1000;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();

  descriptorPool_ = context.device().createDescriptorPool(poolInfo);
}

} // namespace w3d
