#include "mesh_converter.hpp"

namespace w3d {

ConvertedMesh MeshConverter::convert(const Mesh& mesh) {
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
  for (const auto& tri : mesh.triangles) {
    result.indices.push_back(tri.vertexIndices[0]);
    result.indices.push_back(tri.vertexIndices[1]);
    result.indices.push_back(tri.vertexIndices[2]);
  }

  return result;
}

std::vector<ConvertedMesh> MeshConverter::convertAll(const W3DFile& file) {
  std::vector<ConvertedMesh> result;
  result.reserve(file.meshes.size());

  for (const auto& mesh : file.meshes) {
    auto converted = convert(mesh);
    if (!converted.vertices.empty() && !converted.indices.empty()) {
      result.push_back(std::move(converted));
    }
  }

  return result;
}

BoundingBox MeshConverter::combinedBounds(const std::vector<ConvertedMesh>& meshes) {
  BoundingBox combined;
  for (const auto& mesh : meshes) {
    combined.expand(mesh.bounds);
  }
  return combined;
}

glm::vec3 MeshConverter::getVertexColor(const Mesh& mesh, uint32_t idx) {
  // Priority 1: Per-vertex colors
  if (idx < mesh.vertexColors.size()) {
    const auto& c = mesh.vertexColors[idx];
    return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f};
  }

  // Priority 2: Material pass DCG (diffuse color per vertex)
  if (!mesh.materialPasses.empty() && idx < mesh.materialPasses[0].dcg.size()) {
    const auto& c = mesh.materialPasses[0].dcg[idx];
    return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f};
  }

  // Priority 3: First vertex material diffuse color
  if (!mesh.vertexMaterials.empty()) {
    const auto& c = mesh.vertexMaterials[0].diffuse;
    return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f};
  }

  // Fallback: Light gray
  return {0.8f, 0.8f, 0.8f};
}

} // namespace w3d
