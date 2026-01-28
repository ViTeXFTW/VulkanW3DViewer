#pragma once

#include "core/buffer.hpp"
#include "core/pipeline.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

#include "bounding_box.hpp"
#include "skeleton.hpp"
#include "w3d/types.hpp"

namespace w3d {

class VulkanContext;

// GPU resources for a single mesh
struct MeshGPUData {
  VertexBuffer<Vertex> vertexBuffer;
  IndexBuffer indexBuffer;
  std::string name;
  int32_t boneIndex = -1; // Index into skeleton hierarchy (-1 = no bone)

  // CPU-side copies for ray-triangle intersection
  std::vector<Vertex> cpuVertices;
  std::vector<uint32_t> cpuIndices;
};

// Manages GPU resources for all meshes in a loaded file
class RenderableMesh {
public:
  RenderableMesh() = default;
  ~RenderableMesh();

  RenderableMesh(const RenderableMesh &) = delete;
  RenderableMesh &operator=(const RenderableMesh &) = delete;

  // Load meshes from W3D file (without bone transforms)
  void load(VulkanContext &context, const W3DFile &file);

  // Load meshes with bone transforms applied from skeleton pose
  void loadWithPose(VulkanContext &context, const W3DFile &file, const SkeletonPose *pose);

  // Free GPU resources
  void destroy();

  // Check if any meshes are loaded
  bool hasData() const { return !meshes_.empty(); }

  // Get bounds for camera positioning (optionally transformed by skeleton)
  const BoundingBox &bounds() const { return bounds_; }

  // Get mesh count
  size_t meshCount() const { return meshes_.size(); }

  // Access individual mesh GPU data
  const MeshGPUData &mesh(size_t index) const { return meshes_[index]; }

  // Get bone index for a mesh
  int32_t meshBoneIndex(size_t index) const { return meshes_[index].boneIndex; }

  // Get triangle count for a specific mesh
  size_t triangleCount(size_t meshIndex) const {
    if (meshIndex >= meshes_.size()) return 0;
    return meshes_[meshIndex].cpuIndices.size() / 3;
  }

  // Get triangle vertices for intersection testing
  // Returns false if meshIndex or triangleIndex is out of bounds
  bool getTriangle(size_t meshIndex, size_t triangleIndex,
                   glm::vec3 &v0, glm::vec3 &v1, glm::vec3 &v2) const;

  // Record draw commands for all meshes (simple version, no bone transforms)
  void draw(vk::CommandBuffer cmd) const;

  // Draw with hover highlighting on a specific mesh
  // hoverMeshIndex: Index of mesh to highlight (-1 for none)
  // tintColor: Color to multiply with hovered mesh
  // pushConstantCallback: Callback to fill and push material constants with tint applied
  template <typename PushConstantFunc>
  void drawWithHover(vk::CommandBuffer cmd, int hoverMeshIndex, const glm::vec3 &tintColor,
                     PushConstantFunc pushConstantCallback) const;

  // Record draw commands with per-mesh bone transforms
  // updateModelMatrix callback is called for each mesh with its bone transform
  template <typename UpdateModelMatrixFunc>
  void drawWithBoneTransforms(vk::CommandBuffer cmd, const SkeletonPose *pose,
                              UpdateModelMatrixFunc updateModelMatrix) const;

private:
  std::vector<MeshGPUData> meshes_;
  BoundingBox bounds_;
};

// Template implementation must be in header
template <typename UpdateModelMatrixFunc>
void RenderableMesh::drawWithBoneTransforms(vk::CommandBuffer cmd, const SkeletonPose *pose,
                                            UpdateModelMatrixFunc updateModelMatrix) const {
  for (const auto &mesh : meshes_) {
    // Get bone transform for this mesh
    glm::mat4 boneTransform(1.0f); // Identity by default
    if (pose && mesh.boneIndex >= 0 && static_cast<size_t>(mesh.boneIndex) < pose->boneCount()) {
      boneTransform = pose->boneTransform(static_cast<size_t>(mesh.boneIndex));
    }

    // Let caller update the model matrix (typically updates uniform buffer)
    updateModelMatrix(boneTransform);

    // Draw this mesh
    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }
}

template <typename PushConstantFunc>
void RenderableMesh::drawWithHover(vk::CommandBuffer cmd, int hoverMeshIndex,
                                    const glm::vec3 &tintColor,
                                    PushConstantFunc pushConstantCallback) const {
  for (size_t i = 0; i < meshes_.size(); ++i) {
    const auto &mesh = meshes_[i];

    // Determine tint for this mesh
    glm::vec3 meshTint = (static_cast<int>(i) == hoverMeshIndex) ? tintColor : glm::vec3(1.0f);

    // Let callback push material constants with the tint
    pushConstantCallback(i, meshTint);

    // Draw this mesh
    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }
}

} // namespace w3d
