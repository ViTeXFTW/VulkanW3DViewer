#pragma once

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <array>
#include <string>
#include <vector>

namespace w3d {

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

// Skinned vertex with bone index for GPU skinning
// W3D uses rigid skinning (one bone per vertex, no blend weights)
struct SkinnedVertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
  glm::vec3 color;
  uint32_t boneIndex; // Single bone per vertex (W3D rigid skinning)

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

// Material push constant for per-draw material data
struct MaterialPushConstant {
  alignas(16) glm::vec4 diffuseColor;  // RGB + alpha
  alignas(16) glm::vec4 emissiveColor; // RGB + intensity
  alignas(16) glm::vec4 specularColor; // RGB + shininess
  alignas(16) glm::vec3 hoverTint;     // RGB tint for hover highlighting (1,1,1 = no tint)
  alignas(4) uint32_t flags;           // Material flags
  alignas(4) float alphaThreshold;     // For alpha testing
  alignas(4) uint32_t useTexture;      // 1 = sample texture, 0 = use vertex color
};

// Pipeline configuration for different blend modes
struct PipelineConfig {
  bool enableBlending = false;
  bool alphaBlend = false; // true = alpha blend, false = additive
  bool depthWrite = true;
  bool twoSided = false;
};

class Pipeline {
public:
  Pipeline() = default;
  ~Pipeline();

  Pipeline(const Pipeline &) = delete;
  Pipeline &operator=(const Pipeline &) = delete;

  void create(VulkanContext &context, const std::string &vertShaderPath,
              const std::string &fragShaderPath);

  // Create pipeline with texture support
  void createWithTexture(VulkanContext &context, const std::string &vertShaderPath,
                         const std::string &fragShaderPath, const PipelineConfig &config = {});

  // Create skinned pipeline with bone SSBO support
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

  // Create with texture support
  void createWithTexture(VulkanContext &context, vk::DescriptorSetLayout layout,
                         uint32_t frameCount, uint32_t maxTextures = 64);
  void destroy();

  void updateUniformBuffer(uint32_t frameIndex, vk::Buffer buffer, vk::DeviceSize size);

  // Update texture binding (deprecated - use per-texture descriptor sets instead)
  void updateTexture(uint32_t frameIndex, vk::ImageView imageView, vk::Sampler sampler);

  // Create or get a per-texture descriptor set
  // Returns a descriptor set that has the UBO from the current frame and the specified texture
  vk::DescriptorSet getTextureDescriptorSet(uint32_t frameIndex, uint32_t textureIndex,
                                            vk::ImageView imageView, vk::Sampler sampler);

  vk::DescriptorSet descriptorSet(uint32_t frameIndex) const { return descriptorSets_[frameIndex]; }

private:
  vk::Device device_;
  vk::DescriptorPool descriptorPool_;
  std::vector<vk::DescriptorSet> descriptorSets_;
  vk::DescriptorSetLayout layout_;
  uint32_t frameCount_ = 0;

  // Per-texture descriptor sets: indexed by (frameIndex * maxTextures_ + textureIndex)
  std::vector<vk::DescriptorSet> textureDescriptorSets_;
  std::vector<bool> textureDescriptorSetInitialized_;
  uint32_t maxTextures_ = 0;
};

// Descriptor manager for skinned rendering with bone SSBO
class SkinnedDescriptorManager {
public:
  SkinnedDescriptorManager() = default;
  ~SkinnedDescriptorManager();

  // Create descriptor pool and sets for skinned rendering
  void create(VulkanContext &context, vk::DescriptorSetLayout layout, uint32_t frameCount,
              uint32_t maxTextures = 64);
  void destroy();

  // Update uniform buffer for a frame
  void updateUniformBuffer(uint32_t frameIndex, vk::Buffer buffer, vk::DeviceSize size);

  // Update bone SSBO for a frame
  void updateBoneBuffer(uint32_t frameIndex, vk::Buffer buffer, vk::DeviceSize size);

  // Get descriptor set with specific texture
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

  // Per-texture descriptor sets
  std::vector<vk::DescriptorSet> textureDescriptorSets_;
  std::vector<bool> textureDescriptorSetInitialized_;
  uint32_t maxTextures_ = 0;
};

} // namespace w3d
