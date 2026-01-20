#pragma once

#include "core/pipeline.hpp"

#include <string>
#include <vector>

#include "bounding_box.hpp"
#include "w3d/types.hpp"

namespace w3d {

struct ConvertedMesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  BoundingBox bounds;
  std::string name;
};

class MeshConverter {
public:
  // Convert a single W3D mesh to GPU format
  static ConvertedMesh convert(const Mesh &mesh);

  // Convert all meshes in a W3D file
  static std::vector<ConvertedMesh> convertAll(const W3DFile &file);

  // Calculate combined bounds for all meshes
  static BoundingBox combinedBounds(const std::vector<ConvertedMesh> &meshes);

private:
  // Get vertex color with fallback to default
  static glm::vec3 getVertexColor(const Mesh &mesh, uint32_t vertexIndex);
};

} // namespace w3d
