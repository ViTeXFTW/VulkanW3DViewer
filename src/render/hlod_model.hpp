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
  std::string textureName;  // Primary texture name (from first texture stage)
  int32_t boneIndex = -1;
  size_t lodLevel = 0;      // Which LOD level this mesh belongs to
  bool isAggregate = false; // True if this is an always-rendered aggregate

  // CPU-side copies for ray-triangle intersection
  std::vector<Vertex> cpuVertices;
  std::vector<uint32_t> cpuIndices;

  // Sub-mesh metadata for hover display
  std::string baseName;      // Base mesh name without _subN suffix
  size_t subMeshIndex = 0;   // Which sub-mesh (0-indexed)
  size_t subMeshTotal = 1;   // Total sub-meshes for this base mesh
};

// GPU resources for a skinned mesh (with per-vertex bone indices)
struct HLodSkinnedMeshGPU {
  VertexBuffer<SkinnedVertex> vertexBuffer;
  IndexBuffer indexBuffer;
  std::string name;
  std::string textureName;
  int32_t fallbackBoneIndex = -1; // Default bone if vertex has no influence
  size_t lodLevel = 0;
  bool isAggregate = false;
  bool hasSkinning = false; // True if mesh has per-vertex bone indices

  // CPU-side copies for ray-triangle intersection
  std::vector<SkinnedVertex> cpuVertices;
  std::vector<uint32_t> cpuIndices;

  // Sub-mesh metadata for hover display
  std::string baseName;
  size_t subMeshIndex = 0;
  size_t subMeshTotal = 1;
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

  // Load HLod model with skinned meshes (per-vertex bone indices for GPU skinning)
  void loadSkinned(VulkanContext &context, const W3DFile &file);

  // Free GPU resources
  void destroy();

  // Check if model is loaded
  bool hasData() const { return !meshGPU_.empty() || !skinnedMeshGPU_.empty(); }

  // Check if model has skinned meshes
  bool hasSkinning() const { return !skinnedMeshGPU_.empty(); }

  // Get skinned mesh count
  size_t skinnedMeshCount() const { return skinnedMeshGPU_.size(); }

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

  // Draw with hover highlighting on a specific mesh
  // hoverMeshIndex: Index of mesh to highlight (-1 for none)
  // tintColor: Color to multiply with hovered mesh
  // beforeDraw: Callback receiving mesh index, texture name, and tint color
  template <typename BeforeDrawFunc>
  void drawWithHover(vk::CommandBuffer cmd, int hoverMeshIndex, const glm::vec3 &tintColor,
                     BeforeDrawFunc beforeDraw) const;

  // Get mesh GPU data (for external texture binding)
  const std::vector<HLodMeshGPU> &meshes() const { return meshGPU_; }

  // Triangle access for ray-casting (non-skinned meshes)
  size_t triangleCount(size_t meshIndex) const;
  bool getTriangle(size_t meshIndex, size_t triangleIndex, glm::vec3 &v0, glm::vec3 &v1,
                   glm::vec3 &v2) const;

  // Triangle access for skinned meshes
  size_t skinnedTriangleCount(size_t meshIndex) const;
  bool getSkinnedTriangle(size_t meshIndex, size_t triangleIndex, glm::vec3 &v0, glm::vec3 &v1,
                          glm::vec3 &v2) const;

  // Get mesh name by index
  const std::string &meshName(size_t index) const;
  const std::string &skinnedMeshName(size_t index) const;

  // Check if a mesh is visible at the current LOD level
  bool isMeshVisible(size_t meshIndex) const;
  bool isSkinnedMeshVisible(size_t meshIndex) const;

  // Get indices of all visible meshes (aggregates + current LOD)
  std::vector<size_t> visibleMeshIndices() const;
  std::vector<size_t> visibleSkinnedMeshIndices() const;

  // Draw with per-mesh bone transforms
  template <typename UpdateModelMatrixFunc>
  void drawWithBoneTransforms(vk::CommandBuffer cmd, const SkeletonPose *pose,
                              UpdateModelMatrixFunc updateModelMatrix) const;

  // Draw skinned meshes with texture binding callback
  // Uses GPU skinning with bone matrices from SSBO
  template <typename BindTextureFunc>
  void drawSkinnedWithTextures(vk::CommandBuffer cmd, BindTextureFunc bindTexture) const;

  // Draw skinned meshes with hover highlighting
  // hoverMeshIndex: Index of mesh to highlight (-1 for none)
  // tintColor: Color to multiply with hovered mesh
  // beforeDraw: Callback receiving mesh index, texture name, and tint color
  template <typename BeforeDrawFunc>
  void drawSkinnedWithHover(vk::CommandBuffer cmd, int hoverMeshIndex, const glm::vec3 &tintColor,
                            BeforeDrawFunc beforeDraw) const;

  // Get skinned mesh GPU data
  const std::vector<HLodSkinnedMeshGPU> &skinnedMeshes() const { return skinnedMeshGPU_; }

private:
  // Build mesh name to index mapping
  std::unordered_map<std::string, size_t> buildMeshNameMap(const W3DFile &file);

  // Find mesh index by name (tries full name and short name)
  std::optional<size_t> findMeshIndex(const std::unordered_map<std::string, size_t> &nameMap,
                                      const W3DFile &file, const std::string &name);

  // Calculate screen size from world-space bounding sphere
  float calculateScreenSize(float radius, float distance, float screenHeight, float fovY) const;

  // Unified mesh drawing helper - iterates meshes and calls beforeDraw callback
  // Template works with both HLodMeshGPU and HLodSkinnedMeshGPU
  template <typename MeshT, typename BeforeDrawFunc>
  void drawMeshesImpl(vk::CommandBuffer cmd, const std::vector<MeshT> &meshes,
                      size_t aggregateCount, BeforeDrawFunc beforeDraw) const;

  std::string name_;
  std::string hierarchyName_;

  std::vector<HLodLevelInfo> lodLevels_;           // LOD level information
  std::vector<HLodMeshGPU> meshGPU_;               // All GPU mesh data
  std::vector<HLodSkinnedMeshGPU> skinnedMeshGPU_; // Skinned GPU mesh data
  size_t aggregateCount_ = 0;        // Number of aggregate meshes (at start of meshGPU_)
  size_t skinnedAggregateCount_ = 0; // Number of skinned aggregate meshes

  LODSelectionMode selectionMode_ = LODSelectionMode::Auto;
  size_t currentLOD_ = 0;          // Current LOD level being rendered
  float currentScreenSize_ = 0.0f; // Current calculated screen size

  BoundingBox combinedBounds_;     // Combined bounds of all meshes
};

// Template implementation - unified mesh drawing helper
template <typename MeshT, typename BeforeDrawFunc>
void HLodModel::drawMeshesImpl(vk::CommandBuffer cmd, const std::vector<MeshT> &meshes,
                               size_t aggregateCount, BeforeDrawFunc beforeDraw) const {
  // Draw aggregates first (always rendered)
  for (size_t i = 0; i < aggregateCount; ++i) {
    const auto &mesh = meshes[i];
    beforeDraw(mesh);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }

  // Draw current LOD level meshes
  for (size_t i = aggregateCount; i < meshes.size(); ++i) {
    const auto &mesh = meshes[i];

    // Skip if not in current LOD level
    if (mesh.lodLevel != currentLOD_) {
      continue;
    }

    beforeDraw(mesh);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }
}

template <typename BindTextureFunc>
void HLodModel::drawWithTextures(vk::CommandBuffer cmd, BindTextureFunc bindTexture) const {
  drawMeshesImpl(cmd, meshGPU_, aggregateCount_,
                 [&](const HLodMeshGPU &mesh) { bindTexture(mesh.textureName); });
}

template <typename BeforeDrawFunc>
void HLodModel::drawWithHover(vk::CommandBuffer cmd, int hoverMeshIndex,
                              const glm::vec3 &tintColor, BeforeDrawFunc beforeDraw) const {
  // Draw aggregates first (always rendered)
  for (size_t i = 0; i < aggregateCount_; ++i) {
    const auto &mesh = meshGPU_[i];
    // Compare against actual array index (i), not a sequential counter
    glm::vec3 tint = (static_cast<int>(i) == hoverMeshIndex) ? tintColor : glm::vec3(1.0f);
    beforeDraw(i, mesh.textureName, tint);

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

    // Compare against actual array index (i), not a sequential counter
    glm::vec3 tint = (static_cast<int>(i) == hoverMeshIndex) ? tintColor : glm::vec3(1.0f);
    beforeDraw(i, mesh.textureName, tint);

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
  drawMeshesImpl(cmd, meshGPU_, aggregateCount_, [&](const HLodMeshGPU &mesh) {
    glm::mat4 boneTransform(1.0f);
    if (pose && mesh.boneIndex >= 0 && static_cast<size_t>(mesh.boneIndex) < pose->boneCount()) {
      boneTransform = pose->boneTransform(static_cast<size_t>(mesh.boneIndex));
    }
    updateModelMatrix(boneTransform);
  });
}

template <typename BindTextureFunc>
void HLodModel::drawSkinnedWithTextures(vk::CommandBuffer cmd, BindTextureFunc bindTexture) const {
  drawMeshesImpl(cmd, skinnedMeshGPU_, skinnedAggregateCount_,
                 [&](const HLodSkinnedMeshGPU &mesh) { bindTexture(mesh.textureName); });
}

template <typename BeforeDrawFunc>
void HLodModel::drawSkinnedWithHover(vk::CommandBuffer cmd, int hoverMeshIndex,
                                     const glm::vec3 &tintColor, BeforeDrawFunc beforeDraw) const {
  // Draw aggregates first (always rendered)
  for (size_t i = 0; i < skinnedAggregateCount_; ++i) {
    const auto &mesh = skinnedMeshGPU_[i];
    // Compare against actual array index (i), not a sequential counter
    glm::vec3 tint = (static_cast<int>(i) == hoverMeshIndex) ? tintColor : glm::vec3(1.0f);
    beforeDraw(i, mesh.textureName, tint);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }

  // Draw current LOD level meshes
  for (size_t i = skinnedAggregateCount_; i < skinnedMeshGPU_.size(); ++i) {
    const auto &mesh = skinnedMeshGPU_[i];

    // Skip if not in current LOD level
    if (mesh.lodLevel != currentLOD_) {
      continue;
    }

    // Compare against actual array index (i), not a sequential counter
    glm::vec3 tint = (static_cast<int>(i) == hoverMeshIndex) ? tintColor : glm::vec3(1.0f);
    beforeDraw(i, mesh.textureName, tint);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }
}

} // namespace w3d
