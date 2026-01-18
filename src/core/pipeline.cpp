#include "pipeline.hpp"
#include "vulkan_context.hpp"
#include <fstream>
#include <stdexcept>

namespace w3d {

Pipeline::~Pipeline() {
  destroy();
}

void Pipeline::create(VulkanContext& context,
                      const std::string& vertShaderPath,
                      const std::string& fragShaderPath) {
  device_ = context.device();

  auto vertShaderCode = readFile(vertShaderPath);
  auto fragShaderCode = readFile(fragShaderPath);

  auto vertShaderModule = createShaderModule(vertShaderCode);
  auto fragShaderModule = createShaderModule(fragShaderCode);

  vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
    {},
    vk::ShaderStageFlagBits::eVertex,
    vertShaderModule,
    "main"
  };

  vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
    {},
    vk::ShaderStageFlagBits::eFragment,
    fragShaderModule,
    "main"
  };

  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
    vertShaderStageInfo, fragShaderStageInfo
  };

  // Vertex input
  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
    {},
    bindingDescription,
    attributeDescriptions
  };

  // Input assembly
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
    {},
    vk::PrimitiveTopology::eTriangleList,
    VK_FALSE
  };

  // Dynamic viewport and scissor
  std::array<vk::DynamicState, 2> dynamicStates = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };

  vk::PipelineDynamicStateCreateInfo dynamicState{
    {},
    dynamicStates
  };

  vk::PipelineViewportStateCreateInfo viewportState{
    {},
    1, nullptr,  // viewport count, but dynamic
    1, nullptr   // scissor count, but dynamic
  };

  // Rasterizer
  vk::PipelineRasterizationStateCreateInfo rasterizer{
    {},
    VK_FALSE,  // depthClampEnable
    VK_FALSE,  // rasterizerDiscardEnable
    vk::PolygonMode::eFill,
    vk::CullModeFlagBits::eBack,
    vk::FrontFace::eCounterClockwise,
    VK_FALSE,  // depthBiasEnable
    0.0f, 0.0f, 0.0f,
    1.0f  // lineWidth
  };

  // Multisampling
  vk::PipelineMultisampleStateCreateInfo multisampling{
    {},
    vk::SampleCountFlagBits::e1,
    VK_FALSE
  };

  // Depth stencil
  vk::PipelineDepthStencilStateCreateInfo depthStencil{
    {},
    VK_TRUE,   // depthTestEnable
    VK_TRUE,   // depthWriteEnable
    vk::CompareOp::eLess,
    VK_FALSE,  // depthBoundsTestEnable
    VK_FALSE   // stencilTestEnable
  };

  // Color blending
  vk::PipelineColorBlendAttachmentState colorBlendAttachment{
    VK_FALSE,
    vk::BlendFactor::eOne,
    vk::BlendFactor::eZero,
    vk::BlendOp::eAdd,
    vk::BlendFactor::eOne,
    vk::BlendFactor::eZero,
    vk::BlendOp::eAdd,
    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
  };

  vk::PipelineColorBlendStateCreateInfo colorBlending{
    {},
    VK_FALSE,
    vk::LogicOp::eCopy,
    colorBlendAttachment
  };

  // Descriptor set layout for UBO
  vk::DescriptorSetLayoutBinding uboLayoutBinding{
    0,
    vk::DescriptorType::eUniformBuffer,
    1,
    vk::ShaderStageFlagBits::eVertex
  };

  vk::DescriptorSetLayoutCreateInfo layoutInfo{
    {},
    uboLayoutBinding
  };

  descriptorSetLayout_ = device_.createDescriptorSetLayout(layoutInfo);

  // Pipeline layout
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
    {},
    descriptorSetLayout_
  };

  pipelineLayout_ = device_.createPipelineLayout(pipelineLayoutInfo);

  // Create pipeline with render pass (Vulkan 1.2)
  vk::GraphicsPipelineCreateInfo pipelineInfo{
    {},
    shaderStages,
    &vertexInputInfo,
    &inputAssembly,
    nullptr,  // tessellation
    &viewportState,
    &rasterizer,
    &multisampling,
    &depthStencil,
    &colorBlending,
    &dynamicState,
    pipelineLayout_,
    context.renderPass(),
    0  // subpass
  };

  auto result = device_.createGraphicsPipeline(nullptr, pipelineInfo);
  if (result.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create graphics pipeline");
  }
  pipeline_ = result.value;

  // Cleanup shader modules
  device_.destroyShaderModule(vertShaderModule);
  device_.destroyShaderModule(fragShaderModule);
}

void Pipeline::destroy() {
  if (device_) {
    if (pipeline_) {
      device_.destroyPipeline(pipeline_);
      pipeline_ = nullptr;
    }
    if (pipelineLayout_) {
      device_.destroyPipelineLayout(pipelineLayout_);
      pipelineLayout_ = nullptr;
    }
    if (descriptorSetLayout_) {
      device_.destroyDescriptorSetLayout(descriptorSetLayout_);
      descriptorSetLayout_ = nullptr;
    }
    device_ = nullptr;
  }
}

std::vector<char> Pipeline::readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

  return buffer;
}

vk::ShaderModule Pipeline::createShaderModule(const std::vector<char>& code) {
  vk::ShaderModuleCreateInfo createInfo{
    {},
    code.size(),
    reinterpret_cast<const uint32_t*>(code.data())
  };

  return device_.createShaderModule(createInfo);
}

DescriptorManager::~DescriptorManager() {
  destroy();
}

void DescriptorManager::create(VulkanContext& context,
                                vk::DescriptorSetLayout layout,
                                uint32_t frameCount) {
  device_ = context.device();

  // Create descriptor pool
  vk::DescriptorPoolSize poolSize{
    vk::DescriptorType::eUniformBuffer,
    frameCount
  };

  vk::DescriptorPoolCreateInfo poolInfo{
    {},
    frameCount,
    poolSize
  };

  descriptorPool_ = device_.createDescriptorPool(poolInfo);

  // Allocate descriptor sets
  std::vector<vk::DescriptorSetLayout> layouts(frameCount, layout);

  vk::DescriptorSetAllocateInfo allocInfo{
    descriptorPool_,
    layouts
  };

  descriptorSets_ = device_.allocateDescriptorSets(allocInfo);
}

void DescriptorManager::destroy() {
  if (device_) {
    if (descriptorPool_) {
      device_.destroyDescriptorPool(descriptorPool_);
      descriptorPool_ = nullptr;
    }
    descriptorSets_.clear();
    device_ = nullptr;
  }
}

void DescriptorManager::updateUniformBuffer(uint32_t frameIndex,
                                            vk::Buffer buffer,
                                            vk::DeviceSize size) {
  vk::DescriptorBufferInfo bufferInfo{
    buffer,
    0,
    size
  };

  vk::WriteDescriptorSet descriptorWrite{
    descriptorSets_[frameIndex],
    0,
    0,
    vk::DescriptorType::eUniformBuffer,
    {},
    bufferInfo
  };

  device_.updateDescriptorSets(descriptorWrite, {});
}

} // namespace w3d
