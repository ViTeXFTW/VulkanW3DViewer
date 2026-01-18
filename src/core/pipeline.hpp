#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <array>

namespace w3d {

class VulkanContext;

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
  glm::vec3 color;

  static vk::VertexInputBindingDescription getBindingDescription() {
    return vk::VertexInputBindingDescription{
      0,
      sizeof(Vertex),
      vk::VertexInputRate::eVertex
    };
  }

  static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions() {
    return {{
      { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position) },
      { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) },
      { 2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord) },
      { 3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) }
    }};
  }
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

class Pipeline {
public:
  Pipeline() = default;
  ~Pipeline();

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;

  void create(VulkanContext& context,
              const std::string& vertShaderPath,
              const std::string& fragShaderPath);
  void destroy();

  vk::Pipeline pipeline() const { return pipeline_; }
  vk::PipelineLayout layout() const { return pipelineLayout_; }
  vk::DescriptorSetLayout descriptorSetLayout() const { return descriptorSetLayout_; }

private:
  std::vector<char> readFile(const std::string& filename);
  vk::ShaderModule createShaderModule(const std::vector<char>& code);

  vk::Device device_;
  vk::Pipeline pipeline_;
  vk::PipelineLayout pipelineLayout_;
  vk::DescriptorSetLayout descriptorSetLayout_;
};

class DescriptorManager {
public:
  DescriptorManager() = default;
  ~DescriptorManager();

  void create(VulkanContext& context, vk::DescriptorSetLayout layout, uint32_t frameCount);
  void destroy();

  void updateUniformBuffer(uint32_t frameIndex, vk::Buffer buffer, vk::DeviceSize size);

  vk::DescriptorSet descriptorSet(uint32_t frameIndex) const {
    return descriptorSets_[frameIndex];
  }

private:
  vk::Device device_;
  vk::DescriptorPool descriptorPool_;
  std::vector<vk::DescriptorSet> descriptorSets_;
};

} // namespace w3d
