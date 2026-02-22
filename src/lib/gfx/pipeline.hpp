#pragma once

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <array>
#include <string>
#include <vector>

namespace w3d::gfx {

class VulkanContext;

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
  glm::vec3 color;

  static vk::VertexInputBindingDescription getBindingDescription() {
    return vk::VertexInputBindingDescription{0, sizeof(Vertex), vk::VertexInputRate::eVertex};
  }

  static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions() {
    return {
        {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
         {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
         {2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)},
         {3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)}}
    };
  }
};

struct SkinnedVertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
  glm::vec3 color;
  uint32_t boneIndex;

  static vk::VertexInputBindingDescription getBindingDescription() {
    return vk::VertexInputBindingDescription{0, sizeof(SkinnedVertex),
                                             vk::VertexInputRate::eVertex};
  }

  static std::array<vk::VertexInputAttributeDescription, 5> getAttributeDescriptions() {
    return {
        {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(SkinnedVertex, position)},
         {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(SkinnedVertex, normal)},
         {2, 0, vk::Format::eR32G32Sfloat, offsetof(SkinnedVertex, texCoord)},
         {3, 0, vk::Format::eR32G32B32Sfloat, offsetof(SkinnedVertex, color)},
         {4, 0, vk::Format::eR32Uint, offsetof(SkinnedVertex, boneIndex)}}
    };
  }
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct MaterialPushConstant {
  alignas(16) glm::vec4 diffuseColor;
  alignas(16) glm::vec4 emissiveColor;
  alignas(16) glm::vec4 specularColor;
  alignas(16) glm::vec3 hoverTint;
  alignas(4) uint32_t flags;
  alignas(4) float alphaThreshold;
  alignas(4) uint32_t useTexture;
};

struct PipelineConfig {
  bool enableBlending = false;
  bool alphaBlend = false;
  bool depthWrite = true;
  bool twoSided = false;
};

struct VertexInputDescription {
  vk::VertexInputBindingDescription binding;
  std::vector<vk::VertexInputAttributeDescription> attributes;
};

struct PipelineCreateInfo {
  std::string vertShaderPath;
  std::string fragShaderPath;
  VertexInputDescription vertexInput;
  std::vector<vk::DescriptorSetLayoutBinding> descriptorBindings;
  std::vector<vk::PushConstantRange> pushConstants;
  vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
  PipelineConfig config;

  static PipelineCreateInfo standard() {
    PipelineCreateInfo info;
    info.vertShaderPath = "shaders/basic.vert.spv";
    info.fragShaderPath = "shaders/basic.frag.spv";

    auto bindingDesc = Vertex::getBindingDescription();
    auto attrDescs = Vertex::getAttributeDescriptions();
    info.vertexInput.binding = bindingDesc;
    info.vertexInput.attributes =
        std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end());

    info.descriptorBindings = {
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer,        1,
                                       vk::ShaderStageFlagBits::eVertex  },
        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eCombinedImageSampler, 1,
                                       vk::ShaderStageFlagBits::eFragment}
    };

    info.pushConstants = {
        vk::PushConstantRange{vk::ShaderStageFlagBits::eFragment, 0, sizeof(MaterialPushConstant)}
    };

    return info;
  }

  static PipelineCreateInfo skinned() {
    PipelineCreateInfo info;
    info.vertShaderPath = "shaders/skinned.vert.spv";
    info.fragShaderPath = "shaders/basic.frag.spv";

    auto bindingDesc = SkinnedVertex::getBindingDescription();
    auto attrDescs = SkinnedVertex::getAttributeDescriptions();
    info.vertexInput.binding = bindingDesc;
    info.vertexInput.attributes =
        std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end());

    info.descriptorBindings = {
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer,        1,
                                       vk::ShaderStageFlagBits::eVertex  },
        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eCombinedImageSampler, 1,
                                       vk::ShaderStageFlagBits::eFragment},
        vk::DescriptorSetLayoutBinding{2, vk::DescriptorType::eStorageBuffer,        1,
                                       vk::ShaderStageFlagBits::eVertex  }
    };

    info.pushConstants = {
        vk::PushConstantRange{vk::ShaderStageFlagBits::eFragment, 0, sizeof(MaterialPushConstant)}
    };

    return info;
  }
};

class Pipeline {
public:
  Pipeline() = default;
  ~Pipeline();

  Pipeline(const Pipeline &) = delete;
  Pipeline &operator=(const Pipeline &) = delete;

  void create(VulkanContext &context, const PipelineCreateInfo &createInfo);

  void createWithTexture(VulkanContext &context, const std::string &vertShaderPath,
                         const std::string &fragShaderPath, const PipelineConfig &config = {});

  void createSkinned(VulkanContext &context, const std::string &vertShaderPath,
                     const std::string &fragShaderPath, const PipelineConfig &config = {});

  void destroy();

  vk::Pipeline pipeline() const { return pipeline_; }
  vk::PipelineLayout layout() const { return pipelineLayout_; }
  vk::DescriptorSetLayout descriptorSetLayout() const { return descriptorSetLayout_; }

private:
  std::vector<char> readFile(const std::string &filename);
  vk::ShaderModule createShaderModule(const std::vector<char> &code);

  vk::Device device_;
  vk::Pipeline pipeline_;
  vk::PipelineLayout pipelineLayout_;
  vk::DescriptorSetLayout descriptorSetLayout_;
};

class DescriptorManager {
public:
  DescriptorManager() = default;
  ~DescriptorManager();

  void create(VulkanContext &context, vk::DescriptorSetLayout layout, uint32_t frameCount);

  void createWithTexture(VulkanContext &context, vk::DescriptorSetLayout layout,
                         uint32_t frameCount, uint32_t maxTextures = 64);
  void destroy();

  void updateUniformBuffer(uint32_t frameIndex, vk::Buffer buffer, vk::DeviceSize size);

  void updateTexture(uint32_t frameIndex, vk::ImageView imageView, vk::Sampler sampler);

  vk::DescriptorSet getTextureDescriptorSet(uint32_t frameIndex, uint32_t textureIndex,
                                            vk::ImageView imageView, vk::Sampler sampler);

  vk::DescriptorSet descriptorSet(uint32_t frameIndex) const { return descriptorSets_[frameIndex]; }

private:
  vk::Device device_;
  vk::DescriptorPool descriptorPool_;
  std::vector<vk::DescriptorSet> descriptorSets_;
  vk::DescriptorSetLayout layout_;
  uint32_t frameCount_ = 0;

  std::vector<vk::DescriptorSet> textureDescriptorSets_;
  std::vector<bool> textureDescriptorSetInitialized_;
  uint32_t maxTextures_ = 0;
};

class SkinnedDescriptorManager {
public:
  SkinnedDescriptorManager() = default;
  ~SkinnedDescriptorManager();

  void create(VulkanContext &context, vk::DescriptorSetLayout layout, uint32_t frameCount,
              uint32_t maxTextures = 64);
  void destroy();

  void updateUniformBuffer(uint32_t frameIndex, vk::Buffer buffer, vk::DeviceSize size);

  void updateBoneBuffer(uint32_t frameIndex, vk::Buffer buffer, vk::DeviceSize size);

  vk::DescriptorSet getDescriptorSet(uint32_t frameIndex, uint32_t textureIndex,
                                     vk::ImageView imageView, vk::Sampler sampler,
                                     vk::Buffer boneBuffer, vk::DeviceSize boneBufferSize);

  vk::DescriptorSet descriptorSet(uint32_t frameIndex) const { return descriptorSets_[frameIndex]; }

private:
  vk::Device device_;
  vk::DescriptorPool descriptorPool_;
  std::vector<vk::DescriptorSet> descriptorSets_;
  vk::DescriptorSetLayout layout_;
  uint32_t frameCount_ = 0;

  std::vector<vk::DescriptorSet> textureDescriptorSets_;
  std::vector<bool> textureDescriptorSetInitialized_;
  uint32_t maxTextures_ = 0;
};

} // namespace w3d::gfx
