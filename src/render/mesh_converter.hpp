#pragma once

#include "core/pipeline.hpp"

#include <string>
#include <unordered_map>
#include <vector>

#include "bounding_box.hpp"
#include "w3d/types.hpp"

namespace w3d {

class SkeletonPose;

// A sub-mesh that uses a single texture
struct ConvertedSubMesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  BoundingBox bounds;
  std::string textureName;
};

// Result of converting a mesh (may have multiple sub-meshes if per-triangle textures)
struct ConvertedMesh {
  std::string name;
  int32_t boneIndex = -1;                  // Index into hierarchy (-1 = no bone attachment)
  std::vector<ConvertedSubMesh> subMeshes; // One per unique texture
  BoundingBox combinedBounds;
};

class MeshConverter {
public:
  // Convert a single W3D mesh to GPU format
  static ConvertedMesh convert(const Mesh &mesh);

  // Convert all meshes in a W3D file (without bone transforms applied)
  static std::vector<ConvertedMesh> convertAll(const W3DFile &file);

  // Convert all meshes with bone transforms applied from skeleton pose
  static std::vector<ConvertedMesh> convertAllWithPose(const W3DFile &file,
                                                       const SkeletonPose *pose);

  // Apply bone transform to a converted mesh's vertices
  static void applyBoneTransform(ConvertedMesh &mesh, const glm::mat4 &transform);

  // Calculate combined bounds for all meshes
  static BoundingBox combinedBounds(const std::vector<ConvertedMesh> &meshes);

private:
  // Get vertex color with fallback to default
  static glm::vec3 getVertexColor(const Mesh &mesh, uint32_t vertexIndex);

  // Build mesh name to bone index mapping from HLod data
  static std::unordered_map<std::string, int32_t> buildMeshToBoneMap(const W3DFile &file);
};

} // namespace w3d
