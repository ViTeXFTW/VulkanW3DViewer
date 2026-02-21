#include "lib/gfx/pipeline.hpp"

#include "lib/gfx/vulkan_context.hpp"

#include <filesystem>
#include <stdexcept>

#include "core/shader_loader.hpp"

namespace w3d::gfx {

Pipeline::~Pipeline() {
  destroy();
}

void Pipeline::create(VulkanContext &context, const gfx::PipelineCreateInfo &createInfo) {
  device_ = context.device();

  auto vertShaderCode = readFile(createInfo.vertShaderPath);
  auto fragShaderCode = readFile(createInfo.fragShaderPath);

  auto vertShaderModule = createShaderModule(vertShaderCode);
  auto fragShaderModule = createShaderModule(fragShaderCode);

  vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
      {}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main"};

  vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
      {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main"};

  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo,
                                                                   fragShaderStageInfo};

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
      {}, createInfo.vertexInput.binding, createInfo.vertexInput.attributes};

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{{}, createInfo.topology, VK_FALSE};

  std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport,
                                                   vk::DynamicState::eScissor};

  vk::PipelineDynamicStateCreateInfo dynamicState{{}, dynamicStates};

  vk::PipelineViewportStateCreateInfo viewportState{{}, 1, nullptr, 1, nullptr};

  vk::PipelineRasterizationStateCreateInfo rasterizer{
      {},
      VK_FALSE,
      VK_FALSE,
      vk::PolygonMode::eFill,
      createInfo.config.twoSided ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack,
      vk::FrontFace::eCounterClockwise,
      VK_FALSE,
      0.0f,
      0.0f,
      0.0f,
      1.0f};

  vk::PipelineMultisampleStateCreateInfo multisampling{{}, vk::SampleCountFlagBits::e1, VK_FALSE};

  vk::PipelineDepthStencilStateCreateInfo depthStencil{
      {},       VK_TRUE, createInfo.config.depthWrite ? VK_TRUE : VK_FALSE, vk::CompareOp::eLess,
      VK_FALSE, VK_FALSE};

  vk::PipelineColorBlendAttachmentState colorBlendAttachment;
  if (createInfo.config.enableBlending) {
    colorBlendAttachment = vk::PipelineColorBlendAttachmentState{
        VK_TRUE,
        createInfo.config.alphaBlend ? vk::BlendFactor::eSrcAlpha : vk::BlendFactor::eOne,
        createInfo.config.alphaBlend ? vk::BlendFactor::eOneMinusSrcAlpha : vk::BlendFactor::eOne,
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

  vk::DescriptorSetLayoutCreateInfo layoutInfo{{}, createInfo.descriptorBindings};

  descriptorSetLayout_ = device_.createDescriptorSetLayout(layoutInfo);

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
      {}, descriptorSetLayout_, createInfo.pushConstants};

  pipelineLayout_ = device_.createPipelineLayout(pipelineLayoutInfo);

  vk::GraphicsPipelineCreateInfo pipelineInfo{{},
                                              shaderStages,
                                              &vertexInputInfo,
                                              &inputAssembly,
                                              nullptr,
                                              &viewportState,
                                              &rasterizer,
                                              &multisampling,
                                              &depthStencil,
                                              &colorBlending,
                                              &dynamicState,
                                              pipelineLayout_,
                                              context.renderPass(),
                                              0};

  auto result = device_.createGraphicsPipeline(nullptr, pipelineInfo);
  if (result.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create graphics pipeline");
  }
  pipeline_ = result.value;

  device_.destroyShaderModule(vertShaderModule);
  device_.destroyShaderModule(fragShaderModule);
}

void Pipeline::createWithTexture(VulkanContext &context, const std::string &vertShaderPath,
                                 const std::string &fragShaderPath, const PipelineConfig &config) {
  auto createInfo = PipelineCreateInfo::standard();
  createInfo.vertShaderPath = vertShaderPath;
  createInfo.fragShaderPath = fragShaderPath;
  createInfo.config = config;
  create(context, createInfo);
}

void Pipeline::createSkinned(VulkanContext &context, const std::string &vertShaderPath,
                             const std::string &fragShaderPath, const PipelineConfig &config) {
  auto createInfo = PipelineCreateInfo::skinned();
  createInfo.vertShaderPath = vertShaderPath;
  createInfo.fragShaderPath = fragShaderPath;
  createInfo.config = config;
  create(context, createInfo);
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
  std::filesystem::path path(filename);
  std::string shaderName = path.filename().string();

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

  uint32_t totalSets = frameCount + frameCount * maxTextures;

  std::array<vk::DescriptorPoolSize, 2> poolSizes = {
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer,        totalSets},
      vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, totalSets}
  };

  vk::DescriptorPoolCreateInfo poolInfo{{}, totalSets, poolSizes};

  descriptorPool_ = device_.createDescriptorPool(poolInfo);

  std::vector<vk::DescriptorSetLayout> layouts(frameCount, layout);

  vk::DescriptorSetAllocateInfo allocInfo{descriptorPool_, layouts};

  descriptorSets_ = device_.allocateDescriptorSets(allocInfo);

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

  vk::WriteDescriptorSet descriptorWrite{descriptorSets_[frameIndex], 1, 0,
                                         vk::DescriptorType::eCombinedImageSampler, imageInfo};

  device_.updateDescriptorSets(descriptorWrite, {});
}

vk::DescriptorSet DescriptorManager::getTextureDescriptorSet(uint32_t frameIndex,
                                                             uint32_t textureIndex,
                                                             vk::ImageView imageView,
                                                             vk::Sampler sampler) {
  if (textureIndex >= maxTextures_ || frameIndex >= frameCount_) {
    return descriptorSets_[frameIndex];
  }

  uint32_t setIndex = frameIndex * maxTextures_ + textureIndex;
  vk::DescriptorSet set = textureDescriptorSets_[setIndex];

  if (!textureDescriptorSetInitialized_[setIndex]) {
    vk::CopyDescriptorSet copyUbo{descriptorSets_[frameIndex], 0, 0, set, 0, 0, 1};

    vk::DescriptorImageInfo imageInfo{sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal};
    vk::WriteDescriptorSet writeTexture{set, 1, 0, vk::DescriptorType::eCombinedImageSampler,
                                        imageInfo};

    device_.updateDescriptorSets(writeTexture, copyUbo);
    textureDescriptorSetInitialized_[setIndex] = true;
  }

  return set;
}

SkinnedDescriptorManager::~SkinnedDescriptorManager() {
  destroy();
}

void SkinnedDescriptorManager::create(VulkanContext &context, vk::DescriptorSetLayout layout,
                                      uint32_t frameCount, uint32_t maxTextures) {
  device_ = context.device();
  layout_ = layout;
  frameCount_ = frameCount;
  maxTextures_ = maxTextures;

  uint32_t totalSets = frameCount + frameCount * maxTextures;

  std::array<vk::DescriptorPoolSize, 3> poolSizes = {
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer,        totalSets},
      vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, totalSets},
      vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer,        totalSets}
  };

  vk::DescriptorPoolCreateInfo poolInfo{{}, totalSets, poolSizes};

  descriptorPool_ = device_.createDescriptorPool(poolInfo);

  std::vector<vk::DescriptorSetLayout> layouts(frameCount, layout);
  vk::DescriptorSetAllocateInfo allocInfo{descriptorPool_, layouts};
  descriptorSets_ = device_.allocateDescriptorSets(allocInfo);

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
  if (textureIndex >= maxTextures_ || frameIndex >= frameCount_) {
    return descriptorSets_[frameIndex];
  }

  uint32_t setIndex = frameIndex * maxTextures_ + textureIndex;
  vk::DescriptorSet set = textureDescriptorSets_[setIndex];

  if (!textureDescriptorSetInitialized_[setIndex]) {
    vk::CopyDescriptorSet copyUbo{descriptorSets_[frameIndex], 0, 0, set, 0, 0, 1};

    vk::DescriptorImageInfo imageInfo{sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal};
    vk::WriteDescriptorSet writeTexture{set, 1, 0, vk::DescriptorType::eCombinedImageSampler,
                                        imageInfo};

    vk::DescriptorBufferInfo boneInfo{boneBuffer, 0, boneBufferSize};
    vk::WriteDescriptorSet writeBones{set, 2, 0, vk::DescriptorType::eStorageBuffer, {}, boneInfo};

    std::array<vk::WriteDescriptorSet, 2> writes = {writeTexture, writeBones};
    device_.updateDescriptorSets(writes, copyUbo);
    textureDescriptorSetInitialized_[setIndex] = true;
  }

  return set;
}

} // namespace w3d::gfx
