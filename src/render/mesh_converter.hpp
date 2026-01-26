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

// A skinned sub-mesh with per-vertex bone indices
struct ConvertedSkinnedSubMesh {
  std::vector<SkinnedVertex> vertices;
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

// Result of converting a skinned mesh
struct ConvertedSkinnedMesh {
  std::string name;
  int32_t fallbackBoneIndex = -1;                    // Default bone if no per-vertex influences
  std::vector<ConvertedSkinnedSubMesh> subMeshes;    // One per unique texture
  BoundingBox combinedBounds;
  bool hasSkinning = false;                          // True if mesh has per-vertex bone indices
};

class MeshConverter {
public:
  // Convert a single W3D mesh to GPU format
  static ConvertedMesh convert(const Mesh &mesh);

  // Convert a single W3D mesh to skinned GPU format (with per-vertex bone indices)
  static ConvertedSkinnedMesh convertSkinned(const Mesh &mesh, int32_t fallbackBoneIndex);

  // Convert all meshes in a W3D file (without bone transforms applied)
  static std::vector<ConvertedMesh> convertAll(const W3DFile &file);

  // Convert all meshes with bone transforms applied from skeleton pose
  static std::vector<ConvertedMesh> convertAllWithPose(const W3DFile &file,
                                                       const SkeletonPose *pose);

  // Convert all meshes to skinned format with per-vertex bone indices
  static std::vector<ConvertedSkinnedMesh> convertAllSkinned(const W3DFile &file);

  // Apply bone transform to a converted mesh's vertices
  static void applyBoneTransform(ConvertedMesh &mesh, const glm::mat4 &transform);

  // Calculate combined bounds for all meshes
  static BoundingBox combinedBounds(const std::vector<ConvertedMesh> &meshes);

  // Calculate combined bounds for skinned meshes
  static BoundingBox combinedBounds(const std::vector<ConvertedSkinnedMesh> &meshes);

private:
  // Get vertex color with fallback to default
  static glm::vec3 getVertexColor(const Mesh &mesh, uint32_t vertexIndex);

  // Build mesh name to bone index mapping from HLod data
  static std::unordered_map<std::string, int32_t> buildMeshToBoneMap(const W3DFile &file);
};

} // namespace w3d
