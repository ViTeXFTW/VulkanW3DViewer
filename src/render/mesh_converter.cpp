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

  // Find UV source and check for per-face UV indices
  const std::vector<Vector2> *uvSource = &mesh.texCoords;
  const std::vector<uint32_t> *perFaceUVIds = nullptr;

  if (!mesh.materialPasses.empty()) {
    for (const auto &pass : mesh.materialPasses) {
      for (const auto &stage : pass.textureStages) {
        // Check for per-face UV indices first
        if (!stage.perFaceTexCoordIds.empty()) {
          perFaceUVIds = &stage.perFaceTexCoordIds;
        }
        // Get UV source from stage if mesh-level UVs are empty
        if (!stage.texCoords.empty() && mesh.texCoords.empty()) {
          uvSource = &stage.texCoords;
        }
        // Only use first stage with data
        if (perFaceUVIds || uvSource != &mesh.texCoords) {
          break;
        }
      }
      if (perFaceUVIds || uvSource != &mesh.texCoords)
        break;
    }
  }

  // If we have per-face UV indices, we need to unroll the mesh
  // (create separate vertices for each triangle corner since UVs vary per-face)
  if (perFaceUVIds && !perFaceUVIds->empty() && !uvSource->empty()) {
    size_t triCount = mesh.triangles.size();
    result.vertices.reserve(triCount * 3);
    result.indices.reserve(triCount * 3);

    for (size_t triIdx = 0; triIdx < triCount; ++triIdx) {
      const auto &tri = mesh.triangles[triIdx];

      for (int corner = 0; corner < 3; ++corner) {
        uint32_t vertIdx = tri.vertexIndices[corner];
        Vertex v;

        // Position
        if (vertIdx < mesh.vertices.size()) {
          v.position = {mesh.vertices[vertIdx].x, mesh.vertices[vertIdx].y,
                        mesh.vertices[vertIdx].z};
        }

        // Normal
        if (vertIdx < mesh.normals.size()) {
          v.normal = {mesh.normals[vertIdx].x, mesh.normals[vertIdx].y, mesh.normals[vertIdx].z};
        } else {
          v.normal = {0.0f, 1.0f, 0.0f};
        }

        // UV from per-face indices
        size_t uvIdxPosition = triIdx * 3 + corner;
        if (uvIdxPosition < perFaceUVIds->size()) {
          uint32_t uvIdx = (*perFaceUVIds)[uvIdxPosition];
          if (uvIdx < uvSource->size()) {
            v.texCoord = {(*uvSource)[uvIdx].u, (*uvSource)[uvIdx].v};
          }
        }

        // Vertex color
        v.color = getVertexColor(mesh, vertIdx);

        result.bounds.expand(v.position);
        result.indices.push_back(static_cast<uint32_t>(result.vertices.size()));
        result.vertices.push_back(v);
      }
    }
  } else {
    // Standard per-vertex UV mapping
    result.vertices.reserve(vertexCount);

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
      if (i < uvSource->size()) {
        v.texCoord = {(*uvSource)[i].u, (*uvSource)[i].v};
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
