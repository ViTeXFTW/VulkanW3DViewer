#pragma once

#include "core/buffer.hpp"

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <vector>

#include "skeleton.hpp"

namespace w3d {

class VulkanContext;
class Pipeline;

// Simple vertex for skeleton visualization (position + color)
struct SkeletonVertex {
  glm::vec3 position;
  glm::vec3 color;

  static vk::VertexInputBindingDescription getBindingDescription() {
    return vk::VertexInputBindingDescription{0, sizeof(SkeletonVertex),
                                             vk::VertexInputRate::eVertex};
  }

  static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
    return {
        {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(SkeletonVertex, position)},
         {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(SkeletonVertex, color)}}
    };
  }
};

// Renders a skeleton as lines and joint spheres
class SkeletonRenderer {
public:
  SkeletonRenderer() = default;
  ~SkeletonRenderer();

  SkeletonRenderer(const SkeletonRenderer &) = delete;
  SkeletonRenderer &operator=(const SkeletonRenderer &) = delete;

  // Create pipeline and resources
  void create(VulkanContext &context);

  // Update skeleton geometry from pose
  void updateFromPose(VulkanContext &context, const SkeletonPose &pose);

  // Free resources
  void destroy();

  // Check if skeleton is loaded
  bool hasData() const { return lineVertexCount_ > 0 || jointVertexCount_ > 0; }

  // Get pipeline for drawing
  vk::Pipeline linePipeline() const { return linePipeline_; }
  vk::Pipeline pointPipeline() const { return pointPipeline_; }
  vk::PipelineLayout pipelineLayout() const { return pipelineLayout_; }
  vk::DescriptorSetLayout descriptorSetLayout() const { return descriptorSetLayout_; }

  // Record draw commands (call after binding descriptor set)
  void draw(vk::CommandBuffer cmd) const;

  // Color configuration
  void setBoneColor(const glm::vec3 &color) { boneColor_ = color; }
  void setJointColor(const glm::vec3 &color) { jointColor_ = color; }
  void setRootColor(const glm::vec3 &color) { rootColor_ = color; }

private:
  void createPipeline(VulkanContext &context);
  void createDescriptorSetLayout(VulkanContext &context);

  // Generate joint sphere vertices (icosphere approximation)
  std::vector<SkeletonVertex> generateJointSphere(const glm::vec3 &center, float radius,
                                                  const glm::vec3 &color) const;

  vk::Device device_;

  // Pipeline resources
  vk::Pipeline linePipeline_;
  vk::Pipeline pointPipeline_;
  vk::PipelineLayout pipelineLayout_;
  vk::DescriptorSetLayout descriptorSetLayout_;

  // Geometry buffers
  VertexBuffer<SkeletonVertex> lineBuffer_;
  VertexBuffer<SkeletonVertex> jointBuffer_;
  uint32_t lineVertexCount_ = 0;
  uint32_t jointVertexCount_ = 0;

  // Colors
  glm::vec3 boneColor_{0.8f, 0.8f, 0.2f};  // Yellow for bones
  glm::vec3 jointColor_{0.2f, 0.8f, 0.2f}; // Green for joints
  glm::vec3 rootColor_{1.0f, 0.2f, 0.2f};  // Red for root joint

  // Joint sphere detail (number of subdivisions)
  static constexpr int kJointSphereDetail = 1;
  static constexpr float kJointSizeRatio = 0.02f; // Joint size relative to skeleton size
};

} // namespace w3d
