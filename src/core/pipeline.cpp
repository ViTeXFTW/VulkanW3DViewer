#include "pipeline.hpp"

#include "vulkan_context.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "shader_loader.hpp"

namespace w3d {

Pipeline::~Pipeline() {
  destroy();
}

void Pipeline::create(VulkanContext &context, const std::string &vertShaderPath,
                      const std::string &fragShaderPath) {
  createWithTexture(context, vertShaderPath, fragShaderPath, {});
}

void Pipeline::createWithTexture(VulkanContext &context, const std::string &vertShaderPath,
                                 const std::string &fragShaderPath, const PipelineConfig &config) {
  device_ = context.device();

  auto vertShaderCode = readFile(vertShaderPath);
  auto fragShaderCode = readFile(fragShaderPath);

  auto vertShaderModule = createShaderModule(vertShaderCode);
  auto fragShaderModule = createShaderModule(fragShaderCode);

  vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
      {}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main"};

  vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
      {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main"};

  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo,
                                                                   fragShaderStageInfo};

  // Vertex input
  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
      {}, bindingDescription, attributeDescriptions};

  // Input assembly
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
      {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE};

  // Dynamic viewport and scissor
  std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport,
                                                   vk::DynamicState::eScissor};

  vk::PipelineDynamicStateCreateInfo dynamicState{{}, dynamicStates};

  vk::PipelineViewportStateCreateInfo viewportState{
      {},
      1,
      nullptr, // viewport count, but dynamic
      1,
      nullptr  // scissor count, but dynamic
  };

  // Rasterizer
  vk::PipelineRasterizationStateCreateInfo rasterizer{
      {},
      VK_FALSE, // depthClampEnable
      VK_FALSE, // rasterizerDiscardEnable
      vk::PolygonMode::eFill,
      config.twoSided ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack,
      vk::FrontFace::eCounterClockwise,
      VK_FALSE, // depthBiasEnable
      0.0f,
      0.0f,
      0.0f,
      1.0f // lineWidth
  };

  // Multisampling
  vk::PipelineMultisampleStateCreateInfo multisampling{{}, vk::SampleCountFlagBits::e1, VK_FALSE};

  // Depth stencil
  vk::PipelineDepthStencilStateCreateInfo depthStencil{
      {},
      VK_TRUE,                                // depthTestEnable
      config.depthWrite ? VK_TRUE : VK_FALSE, // depthWriteEnable
      vk::CompareOp::eLess,
      VK_FALSE,                               // depthBoundsTestEnable
      VK_FALSE                                // stencilTestEnable
  };

  // Color blending
  vk::PipelineColorBlendAttachmentState colorBlendAttachment;
  if (config.enableBlending) {
    colorBlendAttachment = vk::PipelineColorBlendAttachmentState{
        VK_TRUE,
        config.alphaBlend ? vk::BlendFactor::eSrcAlpha : vk::BlendFactor::eOne,
        config.alphaBlend ? vk::BlendFactor::eOneMinusSrcAlpha : vk::BlendFactor::eOne,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
  } else {
    colorBlendAttachment = vk::PipelineColorBlendAttachmentState{
        VK_FALSE,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
  }

  vk::PipelineColorBlendStateCreateInfo colorBlending{
      {}, VK_FALSE, vk::LogicOp::eCopy, colorBlendAttachment};

  // Descriptor set layout: binding 0 = UBO, binding 1 = texture sampler
  std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
      vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer,        1,
                                     vk::ShaderStageFlagBits::eVertex  },
      vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eCombinedImageSampler, 1,
                                     vk::ShaderStageFlagBits::eFragment}
  };

  vk::DescriptorSetLayoutCreateInfo layoutInfo{{}, bindings};

  descriptorSetLayout_ = device_.createDescriptorSetLayout(layoutInfo);

  // Push constant range for material data
  vk::PushConstantRange pushConstantRange{vk::ShaderStageFlagBits::eFragment, 0,
                                          sizeof(MaterialPushConstant)};

  // Pipeline layout with push constants
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{{}, descriptorSetLayout_, pushConstantRange};

  pipelineLayout_ = device_.createPipelineLayout(pipelineLayoutInfo);

  // Create pipeline with render pass (Vulkan 1.2)
  vk::GraphicsPipelineCreateInfo pipelineInfo{
      {},
      shaderStages,
      &vertexInputInfo,
      &inputAssembly,
      nullptr, // tessellation
      &viewportState,
      &rasterizer,
      &multisampling,
      &depthStencil,
      &colorBlending,
      &dynamicState,
      pipelineLayout_,
      context.renderPass(),
      0 // subpass
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

void Pipeline::createSkinned(VulkanContext &context, const std::string &vertShaderPath,
                             const std::string &fragShaderPath, const PipelineConfig &config) {
  device_ = context.device();

  auto vertShaderCode = readFile(vertShaderPath);
  auto fragShaderCode = readFile(fragShaderPath);

  auto vertShaderModule = createShaderModule(vertShaderCode);
  auto fragShaderModule = createShaderModule(fragShaderCode);

  vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
      {}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main"};

  vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
      {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main"};

  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo,
                                                                   fragShaderStageInfo};

  // Skinned vertex input (includes bone index)
  auto bindingDescription = SkinnedVertex::getBindingDescription();
  auto attributeDescriptions = SkinnedVertex::getAttributeDescriptions();

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
      {}, bindingDescription, attributeDescriptions};

  // Input assembly
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
      {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE};

  // Dynamic viewport and scissor
  std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport,
                                                   vk::DynamicState::eScissor};

  vk::PipelineDynamicStateCreateInfo dynamicState{{}, dynamicStates};

  vk::PipelineViewportStateCreateInfo viewportState{
      {},
      1,
      nullptr, // viewport count, but dynamic
      1,
      nullptr  // scissor count, but dynamic
  };

  // Rasterizer
  vk::PipelineRasterizationStateCreateInfo rasterizer{
      {},
      VK_FALSE, // depthClampEnable
      VK_FALSE, // rasterizerDiscardEnable
      vk::PolygonMode::eFill,
      config.twoSided ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack,
      vk::FrontFace::eCounterClockwise,
      VK_FALSE, // depthBiasEnable
      0.0f,
      0.0f,
      0.0f,
      1.0f // lineWidth
  };

  // Multisampling
  vk::PipelineMultisampleStateCreateInfo multisampling{{}, vk::SampleCountFlagBits::e1, VK_FALSE};

  // Depth stencil
  vk::PipelineDepthStencilStateCreateInfo depthStencil{
      {},
      VK_TRUE,                                // depthTestEnable
      config.depthWrite ? VK_TRUE : VK_FALSE, // depthWriteEnable
      vk::CompareOp::eLess,
      VK_FALSE,                               // depthBoundsTestEnable
      VK_FALSE                                // stencilTestEnable
  };

  // Color blending
  vk::PipelineColorBlendAttachmentState colorBlendAttachment;
  if (config.enableBlending) {
    colorBlendAttachment = vk::PipelineColorBlendAttachmentState{
        VK_TRUE,
        config.alphaBlend ? vk::BlendFactor::eSrcAlpha : vk::BlendFactor::eOne,
        config.alphaBlend ? vk::BlendFactor::eOneMinusSrcAlpha : vk::BlendFactor::eOne,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
  } else {
    colorBlendAttachment = vk::PipelineColorBlendAttachmentState{
        VK_FALSE,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
  }

  vk::PipelineColorBlendStateCreateInfo colorBlending{
      {}, VK_FALSE, vk::LogicOp::eCopy, colorBlendAttachment};

  // Descriptor set layout: binding 0 = UBO, binding 1 = texture sampler, binding 2 = bone SSBO
  std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {
      vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer,        1,
                                     vk::ShaderStageFlagBits::eVertex  },
      vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eCombinedImageSampler, 1,
                                     vk::ShaderStageFlagBits::eFragment},
      vk::DescriptorSetLayoutBinding{2, vk::DescriptorType::eStorageBuffer,        1,
                                     vk::ShaderStageFlagBits::eVertex  }
  };

  vk::DescriptorSetLayoutCreateInfo layoutInfo{{}, bindings};

  descriptorSetLayout_ = device_.createDescriptorSetLayout(layoutInfo);

  // Push constant range for material data
  vk::PushConstantRange pushConstantRange{vk::ShaderStageFlagBits::eFragment, 0,
                                          sizeof(MaterialPushConstant)};

  // Pipeline layout with push constants
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{{}, descriptorSetLayout_, pushConstantRange};

  pipelineLayout_ = device_.createPipelineLayout(pipelineLayoutInfo);

  // Create pipeline with render pass
  vk::GraphicsPipelineCreateInfo pipelineInfo{
      {},
      shaderStages,
      &vertexInputInfo,
      &inputAssembly,
      nullptr, // tessellation
      &viewportState,
      &rasterizer,
      &multisampling,
      &depthStencil,
      &colorBlending,
      &dynamicState,
      pipelineLayout_,
      context.renderPass(),
      0 // subpass
  };

  auto result = device_.createGraphicsPipeline(nullptr, pipelineInfo);
  if (result.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create skinned graphics pipeline");
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

std::vector<char> Pipeline::readFile(const std::string &filename) {
  // Extract just the filename from the path (e.g., "shaders/basic.vert.spv" -> "basic.vert.spv")
  std::filesystem::path path(filename);
  std::string shaderName = path.filename().string();

  // Load from embedded shaders
  return loadEmbeddedShader(shaderName);
}

vk::ShaderModule Pipeline::createShaderModule(const std::vector<char> &code) {
  vk::ShaderModuleCreateInfo createInfo{
      {}, code.size(), reinterpret_cast<const uint32_t *>(code.data())};

  return device_.createShaderModule(createInfo);
}

DescriptorManager::~DescriptorManager() {
  destroy();
}

void DescriptorManager::create(VulkanContext &context, vk::DescriptorSetLayout layout,
                               uint32_t frameCount) {
  createWithTexture(context, layout, frameCount);
}

void DescriptorManager::createWithTexture(VulkanContext &context, vk::DescriptorSetLayout layout,
                                          uint32_t frameCount, uint32_t maxTextures) {
  device_ = context.device();
  layout_ = layout;
  frameCount_ = frameCount;
  maxTextures_ = maxTextures;

  // Calculate total descriptor sets needed:
  // - frameCount for the base frame descriptor sets
  // - frameCount * maxTextures for per-texture descriptor sets
  uint32_t totalSets = frameCount + frameCount * maxTextures;

  // Create descriptor pool with both UBO and sampler types
  std::array<vk::DescriptorPoolSize, 2> poolSizes = {
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer,        totalSets},
      vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, totalSets}
  };

  vk::DescriptorPoolCreateInfo poolInfo{{}, totalSets, poolSizes};

  descriptorPool_ = device_.createDescriptorPool(poolInfo);

  // Allocate base descriptor sets (one per frame)
  std::vector<vk::DescriptorSetLayout> layouts(frameCount, layout);

  vk::DescriptorSetAllocateInfo allocInfo{descriptorPool_, layouts};

  descriptorSets_ = device_.allocateDescriptorSets(allocInfo);

  // Pre-allocate per-texture descriptor sets
  uint32_t textureSetsCount = frameCount * maxTextures;
  std::vector<vk::DescriptorSetLayout> textureLayouts(textureSetsCount, layout);
  vk::DescriptorSetAllocateInfo textureAllocInfo{descriptorPool_, textureLayouts};
  textureDescriptorSets_ = device_.allocateDescriptorSets(textureAllocInfo);
  textureDescriptorSetInitialized_.resize(textureSetsCount, false);
}

void DescriptorManager::destroy() {
  if (device_) {
    if (descriptorPool_) {
      device_.destroyDescriptorPool(descriptorPool_);
      descriptorPool_ = nullptr;
    }
    descriptorSets_.clear();
    textureDescriptorSets_.clear();
    textureDescriptorSetInitialized_.clear();
    layout_ = nullptr;
    frameCount_ = 0;
    maxTextures_ = 0;
    device_ = nullptr;
  }
}

void DescriptorManager::updateUniformBuffer(uint32_t frameIndex, vk::Buffer buffer,
                                            vk::DeviceSize size) {
  vk::DescriptorBufferInfo bufferInfo{buffer, 0, size};

  vk::WriteDescriptorSet descriptorWrite{descriptorSets_[frameIndex],        0,  0,
                                         vk::DescriptorType::eUniformBuffer, {}, bufferInfo};

  device_.updateDescriptorSets(descriptorWrite, {});
}

void DescriptorManager::updateTexture(uint32_t frameIndex, vk::ImageView imageView,
                                      vk::Sampler sampler) {
  vk::DescriptorImageInfo imageInfo{sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal};

  vk::WriteDescriptorSet descriptorWrite{descriptorSets_[frameIndex],
                                         1, // binding 1
                                         0, vk::DescriptorType::eCombinedImageSampler, imageInfo};

  device_.updateDescriptorSets(descriptorWrite, {});
}

vk::DescriptorSet DescriptorManager::getTextureDescriptorSet(uint32_t frameIndex,
                                                             uint32_t textureIndex,
                                                             vk::ImageView imageView,
                                                             vk::Sampler sampler) {
  // Check bounds
  if (textureIndex >= maxTextures_ || frameIndex >= frameCount_) {
    // Fallback to base descriptor set
    return descriptorSets_[frameIndex];
  }

  uint32_t setIndex = frameIndex * maxTextures_ + textureIndex;
  vk::DescriptorSet set = textureDescriptorSets_[setIndex];

  // Initialize descriptor set if not already done
  if (!textureDescriptorSetInitialized_[setIndex]) {
    // Copy UBO binding from the base descriptor set for this frame
    vk::CopyDescriptorSet copyUbo{descriptorSets_[frameIndex], 0, 0, set, 0, 0, 1};

    // Update texture binding
    vk::DescriptorImageInfo imageInfo{sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal};
    vk::WriteDescriptorSet writeTexture{set, 1, 0, vk::DescriptorType::eCombinedImageSampler,
                                        imageInfo};

    device_.updateDescriptorSets(writeTexture, copyUbo);
    textureDescriptorSetInitialized_[setIndex] = true;
  }

  return set;
}

// SkinnedDescriptorManager implementation

SkinnedDescriptorManager::~SkinnedDescriptorManager() {
  destroy();
}

void SkinnedDescriptorManager::create(VulkanContext &context, vk::DescriptorSetLayout layout,
                                      uint32_t frameCount, uint32_t maxTextures) {
  device_ = context.device();
  layout_ = layout;
  frameCount_ = frameCount;
  maxTextures_ = maxTextures;

  // Calculate total descriptor sets needed
  uint32_t totalSets = frameCount + frameCount * maxTextures;

  // Create descriptor pool with UBO, sampler, and storage buffer types
  std::array<vk::DescriptorPoolSize, 3> poolSizes = {
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer,        totalSets},
      vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, totalSets},
      vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer,        totalSets}
  };

  vk::DescriptorPoolCreateInfo poolInfo{{}, totalSets, poolSizes};

  descriptorPool_ = device_.createDescriptorPool(poolInfo);

  // Allocate base descriptor sets (one per frame)
  std::vector<vk::DescriptorSetLayout> layouts(frameCount, layout);
  vk::DescriptorSetAllocateInfo allocInfo{descriptorPool_, layouts};
  descriptorSets_ = device_.allocateDescriptorSets(allocInfo);

  // Pre-allocate per-texture descriptor sets
  uint32_t textureSetsCount = frameCount * maxTextures;
  std::vector<vk::DescriptorSetLayout> textureLayouts(textureSetsCount, layout);
  vk::DescriptorSetAllocateInfo textureAllocInfo{descriptorPool_, textureLayouts};
  textureDescriptorSets_ = device_.allocateDescriptorSets(textureAllocInfo);
  textureDescriptorSetInitialized_.resize(textureSetsCount, false);
}

void SkinnedDescriptorManager::destroy() {
  if (device_) {
    if (descriptorPool_) {
      device_.destroyDescriptorPool(descriptorPool_);
      descriptorPool_ = nullptr;
    }
    descriptorSets_.clear();
    textureDescriptorSets_.clear();
    textureDescriptorSetInitialized_.clear();
    layout_ = nullptr;
    frameCount_ = 0;
    maxTextures_ = 0;
    device_ = nullptr;
  }
}

void SkinnedDescriptorManager::updateUniformBuffer(uint32_t frameIndex, vk::Buffer buffer,
                                                   vk::DeviceSize size) {
  vk::DescriptorBufferInfo bufferInfo{buffer, 0, size};
  vk::WriteDescriptorSet descriptorWrite{descriptorSets_[frameIndex],        0,  0,
                                         vk::DescriptorType::eUniformBuffer, {}, bufferInfo};
  device_.updateDescriptorSets(descriptorWrite, {});
}

void SkinnedDescriptorManager::updateBoneBuffer(uint32_t frameIndex, vk::Buffer buffer,
                                                vk::DeviceSize size) {
  vk::DescriptorBufferInfo bufferInfo{buffer, 0, size};
  vk::WriteDescriptorSet descriptorWrite{descriptorSets_[frameIndex],        2,  0,
                                         vk::DescriptorType::eStorageBuffer, {}, bufferInfo};
  device_.updateDescriptorSets(descriptorWrite, {});
}

vk::DescriptorSet
SkinnedDescriptorManager::getDescriptorSet(uint32_t frameIndex, uint32_t textureIndex,
                                           vk::ImageView imageView, vk::Sampler sampler,
                                           vk::Buffer boneBuffer, vk::DeviceSize boneBufferSize) {
  // Check bounds
  if (textureIndex >= maxTextures_ || frameIndex >= frameCount_) {
    return descriptorSets_[frameIndex];
  }

  uint32_t setIndex = frameIndex * maxTextures_ + textureIndex;
  vk::DescriptorSet set = textureDescriptorSets_[setIndex];

  // Initialize descriptor set if not already done
  if (!textureDescriptorSetInitialized_[setIndex]) {
    // Copy UBO binding from the base descriptor set
    vk::CopyDescriptorSet copyUbo{descriptorSets_[frameIndex], 0, 0, set, 0, 0, 1};

    // Update texture binding
    vk::DescriptorImageInfo imageInfo{sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal};
    vk::WriteDescriptorSet writeTexture{set, 1, 0, vk::DescriptorType::eCombinedImageSampler,
                                        imageInfo};

    // Update bone buffer binding
    vk::DescriptorBufferInfo boneInfo{boneBuffer, 0, boneBufferSize};
    vk::WriteDescriptorSet writeBones{set, 2, 0, vk::DescriptorType::eStorageBuffer, {}, boneInfo};

    std::array<vk::WriteDescriptorSet, 2> writes = {writeTexture, writeBones};
    device_.updateDescriptorSets(writes, copyUbo);
    textureDescriptorSetInitialized_[setIndex] = true;
  }

  return set;
}

} // namespace w3d
