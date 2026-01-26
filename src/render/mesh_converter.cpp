#include "mesh_converter.hpp"

#include <map>

#include "skeleton.hpp"

namespace w3d {

namespace {

// Helper to get texture name from mesh by texture ID
std::string getTextureName(const Mesh &mesh, uint32_t texId) {
  if (texId < mesh.textures.size()) {
    return mesh.textures[texId].name;
  }
  return "";
}

// Build a vertex from mesh data at given vertex and triangle indices
Vertex buildVertex(const Mesh &mesh, uint32_t vertIdx, size_t triIdx, int corner,
                   const std::vector<Vector2> *uvSource, const std::vector<uint32_t> *perFaceUVIds,
                   const std::function<glm::vec3(const Mesh &, uint32_t)> &getColor) {
  Vertex v;

  // Position
  if (vertIdx < mesh.vertices.size()) {
    v.position = {mesh.vertices[vertIdx].x, mesh.vertices[vertIdx].y, mesh.vertices[vertIdx].z};
  }

  // Normal
  if (vertIdx < mesh.normals.size()) {
    v.normal = {mesh.normals[vertIdx].x, mesh.normals[vertIdx].y, mesh.normals[vertIdx].z};
  } else {
    v.normal = {0.0f, 1.0f, 0.0f};
  }

  // UV - handle per-face UV indices if present
  if (perFaceUVIds && !perFaceUVIds->empty()) {
    size_t uvIdxPosition = triIdx * 3 + corner;
    if (uvIdxPosition < perFaceUVIds->size()) {
      uint32_t uvIdx = (*perFaceUVIds)[uvIdxPosition];
      if (uvSource && uvIdx < uvSource->size()) {
        v.texCoord = {(*uvSource)[uvIdx].u, (*uvSource)[uvIdx].v};
      }
    }
  } else if (uvSource && vertIdx < uvSource->size()) {
    v.texCoord = {(*uvSource)[vertIdx].u, (*uvSource)[vertIdx].v};
  } else {
    v.texCoord = {0.0f, 0.0f};
  }

  // Vertex color
  v.color = getColor(mesh, vertIdx);

  return v;
}

// Build a skinned vertex from mesh data
SkinnedVertex buildSkinnedVertex(const Mesh &mesh, uint32_t vertIdx, size_t triIdx, int corner,
                                 const std::vector<Vector2> *uvSource,
                                 const std::vector<uint32_t> *perFaceUVIds,
                                 const std::function<glm::vec3(const Mesh &, uint32_t)> &getColor,
                                 uint32_t fallbackBoneIndex) {
  SkinnedVertex v;

  // Position
  if (vertIdx < mesh.vertices.size()) {
    v.position = {mesh.vertices[vertIdx].x, mesh.vertices[vertIdx].y, mesh.vertices[vertIdx].z};
  }

  // Normal
  if (vertIdx < mesh.normals.size()) {
    v.normal = {mesh.normals[vertIdx].x, mesh.normals[vertIdx].y, mesh.normals[vertIdx].z};
  } else {
    v.normal = {0.0f, 1.0f, 0.0f};
  }

  // UV - handle per-face UV indices if present
  if (perFaceUVIds && !perFaceUVIds->empty()) {
    size_t uvIdxPosition = triIdx * 3 + corner;
    if (uvIdxPosition < perFaceUVIds->size()) {
      uint32_t uvIdx = (*perFaceUVIds)[uvIdxPosition];
      if (uvSource && uvIdx < uvSource->size()) {
        v.texCoord = {(*uvSource)[uvIdx].u, (*uvSource)[uvIdx].v};
      }
    }
  } else if (uvSource && vertIdx < uvSource->size()) {
    v.texCoord = {(*uvSource)[vertIdx].u, (*uvSource)[vertIdx].v};
  } else {
    v.texCoord = {0.0f, 0.0f};
  }

  // Vertex color
  v.color = getColor(mesh, vertIdx);

  // Bone index - use per-vertex influence if available, otherwise fallback
  if (vertIdx < mesh.vertexInfluences.size()) {
    v.boneIndex = mesh.vertexInfluences[vertIdx].boneIndex;
  } else {
    v.boneIndex = fallbackBoneIndex;
  }

  return v;
}

} // namespace

ConvertedMesh MeshConverter::convert(const Mesh &mesh) {
  ConvertedMesh result;
  result.name = mesh.header.meshName;

  size_t vertexCount = mesh.vertices.size();
  if (vertexCount == 0 || mesh.triangles.empty()) {
    return result;
  }

  // Find UV source and check for per-face UV indices
  const std::vector<Vector2> *uvSource = &mesh.texCoords;
  const std::vector<uint32_t> *perFaceUVIds = nullptr;

  // Get texture IDs (per-triangle or single)
  const std::vector<uint32_t> *textureIds = nullptr;

  if (!mesh.materialPasses.empty()) {
    for (const auto &pass : mesh.materialPasses) {
      for (const auto &stage : pass.textureStages) {
        // Get texture IDs
        if (!stage.textureIds.empty() && textureIds == nullptr) {
          textureIds = &stage.textureIds;
        }
        // Check for per-face UV indices
        if (!stage.perFaceTexCoordIds.empty() && perFaceUVIds == nullptr) {
          perFaceUVIds = &stage.perFaceTexCoordIds;
        }
        // Get UV source from stage if mesh-level UVs are empty
        if (!stage.texCoords.empty() && mesh.texCoords.empty()) {
          uvSource = &stage.texCoords;
        }
      }
    }
  }

  // Build per-triangle texture ID array
  size_t triCount = mesh.triangles.size();
  std::vector<uint32_t> triangleTextureIds(triCount, 0);

  if (textureIds) {
    if (textureIds->size() == 1) {
      // Single texture for all triangles
      std::fill(triangleTextureIds.begin(), triangleTextureIds.end(), (*textureIds)[0]);
    } else if (textureIds->size() >= triCount) {
      // Per-triangle texture assignment
      for (size_t i = 0; i < triCount; ++i) {
        triangleTextureIds[i] = (*textureIds)[i];
      }
    }
  }

  // Group triangles by texture ID
  std::map<uint32_t, std::vector<size_t>> textureToTriangles;
  for (size_t i = 0; i < triCount; ++i) {
    textureToTriangles[triangleTextureIds[i]].push_back(i);
  }

  // Create a sub-mesh for each texture group
  for (const auto &[texId, triangleIndices] : textureToTriangles) {
    ConvertedSubMesh subMesh;
    subMesh.textureName = getTextureName(mesh, texId);

    // Reserve space
    subMesh.vertices.reserve(triangleIndices.size() * 3);
    subMesh.indices.reserve(triangleIndices.size() * 3);

    // Check if we need to unroll (per-face UVs require unrolling, multi-texture also requires it)
    bool needsUnroll = (perFaceUVIds && !perFaceUVIds->empty()) || textureToTriangles.size() > 1;

    if (needsUnroll) {
      // Unrolled mesh: create separate vertices for each triangle corner
      for (size_t triIdx : triangleIndices) {
        const auto &tri = mesh.triangles[triIdx];

        for (int corner = 0; corner < 3; ++corner) {
          uint32_t vertIdx = tri.vertexIndices[corner];
          Vertex v =
              buildVertex(mesh, vertIdx, triIdx, corner, uvSource, perFaceUVIds, getVertexColor);

          subMesh.bounds.expand(v.position);
          subMesh.indices.push_back(static_cast<uint32_t>(subMesh.vertices.size()));
          subMesh.vertices.push_back(v);
        }
      }
    } else {
      // Standard indexed mesh (only when single texture and no per-face UVs)
      // Build vertex array
      subMesh.vertices.reserve(vertexCount);
      for (size_t i = 0; i < vertexCount; ++i) {
        Vertex v;
        v.position = {mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z};

        if (i < mesh.normals.size()) {
          v.normal = {mesh.normals[i].x, mesh.normals[i].y, mesh.normals[i].z};
        } else {
          v.normal = {0.0f, 1.0f, 0.0f};
        }

        if (uvSource && i < uvSource->size()) {
          v.texCoord = {(*uvSource)[i].u, (*uvSource)[i].v};
        } else {
          v.texCoord = {0.0f, 0.0f};
        }

        v.color = getVertexColor(mesh, static_cast<uint32_t>(i));

        subMesh.vertices.push_back(v);
        subMesh.bounds.expand(v.position);
      }

      // Build index array
      for (size_t triIdx : triangleIndices) {
        const auto &tri = mesh.triangles[triIdx];
        subMesh.indices.push_back(tri.vertexIndices[0]);
        subMesh.indices.push_back(tri.vertexIndices[1]);
        subMesh.indices.push_back(tri.vertexIndices[2]);
      }
    }

    result.combinedBounds.expand(subMesh.bounds);
    result.subMeshes.push_back(std::move(subMesh));
  }

  return result;
}

ConvertedSkinnedMesh MeshConverter::convertSkinned(const Mesh &mesh, int32_t fallbackBoneIndex) {
  ConvertedSkinnedMesh result;
  result.name = mesh.header.meshName;
  result.fallbackBoneIndex = fallbackBoneIndex;
  result.hasSkinning = !mesh.vertexInfluences.empty();

  size_t vertexCount = mesh.vertices.size();
  if (vertexCount == 0 || mesh.triangles.empty()) {
    return result;
  }

  // Determine the fallback bone index to use
  uint32_t fallbackBone = fallbackBoneIndex >= 0 ? static_cast<uint32_t>(fallbackBoneIndex) : 0;

  // Find UV source and check for per-face UV indices
  const std::vector<Vector2> *uvSource = &mesh.texCoords;
  const std::vector<uint32_t> *perFaceUVIds = nullptr;

  // Get texture IDs (per-triangle or single)
  const std::vector<uint32_t> *textureIds = nullptr;

  if (!mesh.materialPasses.empty()) {
    for (const auto &pass : mesh.materialPasses) {
      for (const auto &stage : pass.textureStages) {
        if (!stage.textureIds.empty() && textureIds == nullptr) {
          textureIds = &stage.textureIds;
        }
        if (!stage.perFaceTexCoordIds.empty() && perFaceUVIds == nullptr) {
          perFaceUVIds = &stage.perFaceTexCoordIds;
        }
        if (!stage.texCoords.empty() && mesh.texCoords.empty()) {
          uvSource = &stage.texCoords;
        }
      }
    }
  }

  // Build per-triangle texture ID array
  size_t triCount = mesh.triangles.size();
  std::vector<uint32_t> triangleTextureIds(triCount, 0);

  if (textureIds) {
    if (textureIds->size() == 1) {
      std::fill(triangleTextureIds.begin(), triangleTextureIds.end(), (*textureIds)[0]);
    } else if (textureIds->size() >= triCount) {
      for (size_t i = 0; i < triCount; ++i) {
        triangleTextureIds[i] = (*textureIds)[i];
      }
    }
  }

  // Group triangles by texture ID
  std::map<uint32_t, std::vector<size_t>> textureToTriangles;
  for (size_t i = 0; i < triCount; ++i) {
    textureToTriangles[triangleTextureIds[i]].push_back(i);
  }

  // Create a sub-mesh for each texture group
  for (const auto &[texId, triangleIndices] : textureToTriangles) {
    ConvertedSkinnedSubMesh subMesh;
    subMesh.textureName = getTextureName(mesh, texId);

    subMesh.vertices.reserve(triangleIndices.size() * 3);
    subMesh.indices.reserve(triangleIndices.size() * 3);

    // Check if we need to unroll
    bool needsUnroll = (perFaceUVIds && !perFaceUVIds->empty()) || textureToTriangles.size() > 1;

    if (needsUnroll) {
      // Unrolled mesh: create separate vertices for each triangle corner
      for (size_t triIdx : triangleIndices) {
        const auto &tri = mesh.triangles[triIdx];

        for (int corner = 0; corner < 3; ++corner) {
          uint32_t vertIdx = tri.vertexIndices[corner];
          SkinnedVertex v = buildSkinnedVertex(mesh, vertIdx, triIdx, corner, uvSource,
                                               perFaceUVIds, getVertexColor, fallbackBone);

          subMesh.bounds.expand(v.position);
          subMesh.indices.push_back(static_cast<uint32_t>(subMesh.vertices.size()));
          subMesh.vertices.push_back(v);
        }
      }
    } else {
      // Standard indexed mesh
      subMesh.vertices.reserve(vertexCount);
      for (size_t i = 0; i < vertexCount; ++i) {
        SkinnedVertex v;
        v.position = {mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z};

        if (i < mesh.normals.size()) {
          v.normal = {mesh.normals[i].x, mesh.normals[i].y, mesh.normals[i].z};
        } else {
          v.normal = {0.0f, 1.0f, 0.0f};
        }

        if (uvSource && i < uvSource->size()) {
          v.texCoord = {(*uvSource)[i].u, (*uvSource)[i].v};
        } else {
          v.texCoord = {0.0f, 0.0f};
        }

        v.color = getVertexColor(mesh, static_cast<uint32_t>(i));

        // Bone index
        if (i < mesh.vertexInfluences.size()) {
          v.boneIndex = mesh.vertexInfluences[i].boneIndex;
        } else {
          v.boneIndex = fallbackBone;
        }

        subMesh.vertices.push_back(v);
        subMesh.bounds.expand(v.position);
      }

      // Build index array
      for (size_t triIdx : triangleIndices) {
        const auto &tri = mesh.triangles[triIdx];
        subMesh.indices.push_back(tri.vertexIndices[0]);
        subMesh.indices.push_back(tri.vertexIndices[1]);
        subMesh.indices.push_back(tri.vertexIndices[2]);
      }
    }

    result.combinedBounds.expand(subMesh.bounds);
    result.subMeshes.push_back(std::move(subMesh));
  }

  return result;
}

std::vector<ConvertedSkinnedMesh> MeshConverter::convertAllSkinned(const W3DFile &file) {
  std::vector<ConvertedSkinnedMesh> result;
  result.reserve(file.meshes.size());

  // Build mesh name to bone index mapping from HLod data
  auto meshToBone = buildMeshToBoneMap(file);

  for (const auto &mesh : file.meshes) {
    // Find fallback bone index for this mesh
    int32_t fallbackBoneIndex = 0;
    std::string fullName = mesh.header.containerName + "." + mesh.header.meshName;
    auto it = meshToBone.find(fullName);
    if (it != meshToBone.end()) {
      fallbackBoneIndex = it->second;
    } else {
      it = meshToBone.find(mesh.header.meshName);
      if (it != meshToBone.end()) {
        fallbackBoneIndex = it->second;
      }
    }

    auto converted = convertSkinned(mesh, fallbackBoneIndex);
    if (!converted.subMeshes.empty()) {
      result.push_back(std::move(converted));
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
    if (!converted.subMeshes.empty()) {
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
  // Reset combined bounds since we're transforming vertices
  mesh.combinedBounds = BoundingBox{};

  // Transform each vertex position and normal
  glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

  for (auto &subMesh : mesh.subMeshes) {
    subMesh.bounds = BoundingBox{};

    for (auto &v : subMesh.vertices) {
      // Transform position
      glm::vec4 pos = transform * glm::vec4(v.position, 1.0f);
      v.position = glm::vec3(pos);

      // Transform normal (using normal matrix to handle non-uniform scaling)
      v.normal = glm::normalize(normalMatrix * v.normal);

      // Update bounds
      subMesh.bounds.expand(v.position);
    }

    mesh.combinedBounds.expand(subMesh.bounds);
  }
}

BoundingBox MeshConverter::combinedBounds(const std::vector<ConvertedMesh> &meshes) {
  BoundingBox combined;
  for (const auto &mesh : meshes) {
    combined.expand(mesh.combinedBounds);
  }
  return combined;
}

BoundingBox MeshConverter::combinedBounds(const std::vector<ConvertedSkinnedMesh> &meshes) {
  BoundingBox combined;
  for (const auto &mesh : meshes) {
    combined.expand(mesh.combinedBounds);
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
