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

  // Draw with optional hover tint (applies to all skeleton elements)
  void drawWithHover(vk::CommandBuffer cmd, const glm::vec3 &tintColor) const;

  // Color configuration
  void setBoneColor(const glm::vec3 &color) { boneColor_ = color; }
  void setJointColor(const glm::vec3 &color) { jointColor_ = color; }
  void setRootColor(const glm::vec3 &color) { rootColor_ = color; }

  // Geometry access for hover detection
  size_t boneCount() const { return bonePositions_.size(); }
  size_t jointCount() const { return bonePositions_.size(); }

  // Get bone segment for ray intersection (returns false if index out of bounds or no parent)
  bool getBoneSegment(size_t boneIndex, glm::vec3 &start, glm::vec3 &end) const;

  // Get joint sphere for ray intersection (returns false if index out of bounds)
  bool getJointSphere(size_t jointIndex, glm::vec3 &center, float &radius) const;

  // Get bone/joint name by index
  const std::string &boneName(size_t index) const {
    if (index < boneNames_.size()) {
      return boneNames_[index];
    }
    static const std::string empty;
    return empty;
  }

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

  // Current pose data for hover detection
  std::vector<glm::vec3> bonePositions_;
  std::vector<int> parentIndices_;
  std::vector<std::string> boneNames_;
  float jointRadius_ = 0.01f;

  // Joint sphere detail (number of subdivisions)
  static constexpr int kJointSphereDetail = 1;
  static constexpr float kJointSizeRatio = 0.02f; // Joint size relative to skeleton size
};

} // namespace w3d
