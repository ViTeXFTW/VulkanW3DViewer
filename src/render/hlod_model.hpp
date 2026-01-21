#pragma once

#include "core/buffer.hpp"
#include "core/pipeline.hpp"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <vector>

#include "bounding_box.hpp"
#include "skeleton.hpp"
#include "w3d/types.hpp"

namespace w3d {

class VulkanContext;

// Information about a single mesh within a LOD level
struct HLodMeshInfo {
  size_t meshIndex;   // Index into W3DFile::meshes
  uint32_t boneIndex; // Bone this mesh is attached to
  std::string name;   // Mesh identifier (for debugging)
};

// Information about a single LOD level
struct HLodLevelInfo {
  float maxScreenSize;              // Maximum screen size for this LOD (0 = highest detail)
  std::vector<HLodMeshInfo> meshes; // Meshes to render at this LOD level
  BoundingBox bounds;               // Combined bounds for this LOD level
};

// GPU resources for a mesh in the HLod model
struct HLodMeshGPU {
  VertexBuffer<Vertex> vertexBuffer;
  IndexBuffer indexBuffer;
  std::string name;
  std::string textureName;   // Primary texture name (from first texture stage)
  int32_t boneIndex = -1;
  size_t lodLevel = 0;      // Which LOD level this mesh belongs to
  bool isAggregate = false; // True if this is an always-rendered aggregate
};

// LOD selection mode
enum class LODSelectionMode {
  Auto,  // Automatically select LOD based on screen size
  Manual // Manual LOD level selection
};

// Complete HLod model with LOD management
class HLodModel {
public:
  HLodModel() = default;
  ~HLodModel();

  HLodModel(const HLodModel &) = delete;
  HLodModel &operator=(const HLodModel &) = delete;

  // Load HLod model from W3D file
  // Uses the first HLod definition in the file
  void load(VulkanContext &context, const W3DFile &file, const SkeletonPose *pose);

  // Free GPU resources
  void destroy();

  // Check if model is loaded
  bool hasData() const { return !meshGPU_.empty(); }

  // Get the HLod name
  const std::string &name() const { return name_; }

  // Get the hierarchy name this HLod references
  const std::string &hierarchyName() const { return hierarchyName_; }

  // Get total LOD level count
  size_t lodCount() const { return lodLevels_.size(); }

  // Get information about a specific LOD level
  const HLodLevelInfo &lodLevel(size_t index) const { return lodLevels_[index]; }

  // Get/set LOD selection mode
  LODSelectionMode selectionMode() const { return selectionMode_; }
  void setSelectionMode(LODSelectionMode mode) { selectionMode_ = mode; }

  // Get/set current LOD level (for manual mode)
  size_t currentLOD() const { return currentLOD_; }
  void setCurrentLOD(size_t level);

  // Update LOD selection based on camera parameters
  // screenHeight: viewport height in pixels
  // fovY: vertical field of view in radians
  // cameraDistance: distance from camera to model center
  void updateLOD(float screenHeight, float fovY, float cameraDistance);

  // Get the screen size value for the current view (for UI display)
  float currentScreenSize() const { return currentScreenSize_; }

  // Get combined bounds for all meshes (or current LOD)
  const BoundingBox &bounds() const { return combinedBounds_; }

  // Get aggregate mesh count
  size_t aggregateCount() const { return aggregateCount_; }

  // Get total mesh count on GPU
  size_t totalMeshCount() const { return meshGPU_.size(); }

  // Draw current LOD level (plus aggregates)
  void draw(vk::CommandBuffer cmd) const;

  // Draw with texture binding callback
  // bindTexture: called with texture name before drawing each mesh
  template <typename BindTextureFunc>
  void drawWithTextures(vk::CommandBuffer cmd, BindTextureFunc bindTexture) const;

  // Get mesh GPU data (for external texture binding)
  const std::vector<HLodMeshGPU> &meshes() const { return meshGPU_; }

  // Draw with per-mesh bone transforms
  template <typename UpdateModelMatrixFunc>
  void drawWithBoneTransforms(vk::CommandBuffer cmd, const SkeletonPose *pose,
                              UpdateModelMatrixFunc updateModelMatrix) const;

private:
  // Build mesh name to index mapping
  std::unordered_map<std::string, size_t> buildMeshNameMap(const W3DFile &file);

  // Find mesh index by name (tries full name and short name)
  std::optional<size_t> findMeshIndex(const std::unordered_map<std::string, size_t> &nameMap,
                                      const W3DFile &file, const std::string &name);

  // Calculate screen size from world-space bounding sphere
  float calculateScreenSize(float radius, float distance, float screenHeight, float fovY) const;

  std::string name_;
  std::string hierarchyName_;

  std::vector<HLodLevelInfo> lodLevels_; // LOD level information
  std::vector<HLodMeshGPU> meshGPU_;     // All GPU mesh data
  size_t aggregateCount_ = 0;            // Number of aggregate meshes (at start of meshGPU_)

  LODSelectionMode selectionMode_ = LODSelectionMode::Auto;
  size_t currentLOD_ = 0;          // Current LOD level being rendered
  float currentScreenSize_ = 0.0f; // Current calculated screen size

  BoundingBox combinedBounds_;     // Combined bounds of all meshes
};

// Template implementation
template <typename BindTextureFunc>
void HLodModel::drawWithTextures(vk::CommandBuffer cmd, BindTextureFunc bindTexture) const {
  // Draw aggregates first (always rendered)
  for (size_t i = 0; i < aggregateCount_; ++i) {
    const auto &mesh = meshGPU_[i];

    bindTexture(mesh.textureName);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }

  // Draw current LOD level meshes
  for (size_t i = aggregateCount_; i < meshGPU_.size(); ++i) {
    const auto &mesh = meshGPU_[i];

    // Skip if not in current LOD level
    if (mesh.lodLevel != currentLOD_) {
      continue;
    }

    bindTexture(mesh.textureName);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }
}

template <typename UpdateModelMatrixFunc>
void HLodModel::drawWithBoneTransforms(vk::CommandBuffer cmd, const SkeletonPose *pose,
                                       UpdateModelMatrixFunc updateModelMatrix) const {
  // Draw aggregates first (always rendered)
  for (size_t i = 0; i < aggregateCount_; ++i) {
    const auto &mesh = meshGPU_[i];

    glm::mat4 boneTransform(1.0f);
    if (pose && mesh.boneIndex >= 0 && static_cast<size_t>(mesh.boneIndex) < pose->boneCount()) {
      boneTransform = pose->boneTransform(static_cast<size_t>(mesh.boneIndex));
    }

    updateModelMatrix(boneTransform);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }

  // Draw current LOD level meshes
  for (size_t i = aggregateCount_; i < meshGPU_.size(); ++i) {
    const auto &mesh = meshGPU_[i];

    // Skip if not in current LOD level
    if (mesh.lodLevel != currentLOD_) {
      continue;
    }

    glm::mat4 boneTransform(1.0f);
    if (pose && mesh.boneIndex >= 0 && static_cast<size_t>(mesh.boneIndex) < pose->boneCount()) {
      boneTransform = pose->boneTransform(static_cast<size_t>(mesh.boneIndex));
    }

    updateModelMatrix(boneTransform);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }
}

} // namespace w3d
