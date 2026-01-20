#pragma once

#include "bounding_box.hpp"
#include "core/buffer.hpp"
#include "core/pipeline.hpp"
#include "w3d/types.hpp"

#include <string>
#include <vector>

namespace w3d {

class VulkanContext;

// GPU resources for a single mesh
struct MeshGPUData {
  VertexBuffer<Vertex> vertexBuffer;
  IndexBuffer indexBuffer;
  std::string name;
};

// Manages GPU resources for all meshes in a loaded file
class RenderableMesh {
public:
  RenderableMesh() = default;
  ~RenderableMesh();

  RenderableMesh(const RenderableMesh&) = delete;
  RenderableMesh& operator=(const RenderableMesh&) = delete;

  // Load meshes from W3D file
  void load(VulkanContext& context, const W3DFile& file);

  // Free GPU resources
  void destroy();

  // Check if any meshes are loaded
  bool hasData() const { return !meshes_.empty(); }

  // Get bounds for camera positioning
  const BoundingBox& bounds() const { return bounds_; }

  // Get mesh count
  size_t meshCount() const { return meshes_.size(); }

  // Access individual mesh GPU data
  const MeshGPUData& mesh(size_t index) const { return meshes_[index]; }

  // Record draw commands for all meshes
  void draw(vk::CommandBuffer cmd) const;

private:
  std::vector<MeshGPUData> meshes_;
  BoundingBox bounds_;
};

} // namespace w3d
