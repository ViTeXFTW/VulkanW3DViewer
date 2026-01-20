#include "mesh_converter.hpp"

#include "skeleton.hpp"

namespace w3d {

ConvertedMesh MeshConverter::convert(const Mesh &mesh) {
  ConvertedMesh result;
  result.name = mesh.header.meshName;

  size_t vertexCount = mesh.vertices.size();
  if (vertexCount == 0) {
    return result;
  }

  result.vertices.reserve(vertexCount);

  // Convert vertices
  for (size_t i = 0; i < vertexCount; ++i) {
    Vertex v;

    // Position
    v.position = {mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z};

    // Normal (with fallback to up vector)
    if (i < mesh.normals.size()) {
      v.normal = {mesh.normals[i].x, mesh.normals[i].y, mesh.normals[i].z};
    } else {
      v.normal = {0.0f, 1.0f, 0.0f};
    }

    // Texture coordinates (with fallback)
    if (i < mesh.texCoords.size()) {
      v.texCoord = {mesh.texCoords[i].u, mesh.texCoords[i].v};
    } else {
      v.texCoord = {0.0f, 0.0f};
    }

    // Vertex color
    v.color = getVertexColor(mesh, static_cast<uint32_t>(i));

    result.vertices.push_back(v);
    result.bounds.expand(v.position);
  }

  // Convert triangles to flat index array
  result.indices.reserve(mesh.triangles.size() * 3);
  for (const auto &tri : mesh.triangles) {
    result.indices.push_back(tri.vertexIndices[0]);
    result.indices.push_back(tri.vertexIndices[1]);
    result.indices.push_back(tri.vertexIndices[2]);
  }

  return result;
}

std::unordered_map<std::string, int32_t> MeshConverter::buildMeshToBoneMap(const W3DFile &file) {
  std::unordered_map<std::string, int32_t> meshToBone;

  for (const auto &hlod : file.hlods) {
    // Process all LOD arrays
    for (const auto &lodArray : hlod.lodArrays) {
      for (const auto &subObj : lodArray.subObjects) {
        meshToBone[subObj.name] = static_cast<int32_t>(subObj.boneIndex);
      }
    }
    // Also process aggregates
    for (const auto &subObj : hlod.aggregates) {
      meshToBone[subObj.name] = static_cast<int32_t>(subObj.boneIndex);
    }
  }

  return meshToBone;
}

std::vector<ConvertedMesh> MeshConverter::convertAll(const W3DFile &file) {
  return convertAllWithPose(file, nullptr);
}

std::vector<ConvertedMesh> MeshConverter::convertAllWithPose(const W3DFile &file,
                                                             const SkeletonPose *pose) {
  std::vector<ConvertedMesh> result;
  result.reserve(file.meshes.size());

  // Build mesh name to bone index mapping from HLod data
  auto meshToBone = buildMeshToBoneMap(file);

  for (const auto &mesh : file.meshes) {
    auto converted = convert(mesh);
    if (!converted.vertices.empty() && !converted.indices.empty()) {
      // Try to find bone index for this mesh
      // First try full name (containerName.meshName)
      std::string fullName = mesh.header.containerName + "." + mesh.header.meshName;
      auto it = meshToBone.find(fullName);
      if (it != meshToBone.end()) {
        converted.boneIndex = it->second;
      } else {
        // Try just mesh name
        it = meshToBone.find(mesh.header.meshName);
        if (it != meshToBone.end()) {
          converted.boneIndex = it->second;
        }
      }

      // Apply bone transform if skeleton pose is provided
      if (pose && converted.boneIndex >= 0 &&
          static_cast<size_t>(converted.boneIndex) < pose->boneCount()) {
        glm::mat4 boneTransform = pose->boneTransform(static_cast<size_t>(converted.boneIndex));
        applyBoneTransform(converted, boneTransform);
      }

      result.push_back(std::move(converted));
    }
  }

  return result;
}

void MeshConverter::applyBoneTransform(ConvertedMesh &mesh, const glm::mat4 &transform) {
  // Reset bounds since we're transforming vertices
  mesh.bounds = BoundingBox{};

  // Transform each vertex position and normal
  glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

  for (auto &v : mesh.vertices) {
    // Transform position
    glm::vec4 pos = transform * glm::vec4(v.position, 1.0f);
    v.position = glm::vec3(pos);

    // Transform normal (using normal matrix to handle non-uniform scaling)
    v.normal = glm::normalize(normalMatrix * v.normal);

    // Update bounds
    mesh.bounds.expand(v.position);
  }
}

BoundingBox MeshConverter::combinedBounds(const std::vector<ConvertedMesh> &meshes) {
  BoundingBox combined;
  for (const auto &mesh : meshes) {
    combined.expand(mesh.bounds);
  }
  return combined;
}

glm::vec3 MeshConverter::getVertexColor(const Mesh &mesh, uint32_t idx) {
  // Priority 1: Per-vertex colors
  if (idx < mesh.vertexColors.size()) {
    const auto &c = mesh.vertexColors[idx];
    return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f};
  }

  // Priority 2: Material pass DCG (diffuse color per vertex)
  if (!mesh.materialPasses.empty() && idx < mesh.materialPasses[0].dcg.size()) {
    const auto &c = mesh.materialPasses[0].dcg[idx];
    return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f};
  }

  // Priority 3: First vertex material diffuse color
  if (!mesh.vertexMaterials.empty()) {
    const auto &c = mesh.vertexMaterials[0].diffuse;
    return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f};
  }

  // Fallback: Light gray
  return {0.8f, 0.8f, 0.8f};
}

} // namespace w3d
