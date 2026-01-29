#include "renderable_mesh.hpp"

#include "core/vulkan_context.hpp"

#include "mesh_converter.hpp"

namespace w3d {

RenderableMesh::~RenderableMesh() {
  destroy();
}

void RenderableMesh::load(VulkanContext &context, const W3DFile &file) {
  loadWithPose(context, file, nullptr);
}

void RenderableMesh::loadWithPose(VulkanContext &context, const W3DFile &file,
                                  const SkeletonPose *pose) {
  destroy(); // Clean up any existing data

  auto converted = MeshConverter::convertAllWithPose(file, pose);
  bounds_ = MeshConverter::combinedBounds(converted);

  // Count total sub-meshes for reservation
  size_t totalSubMeshes = 0;
  for (const auto &cm : converted) {
    totalSubMeshes += cm.subMeshes.size();
  }
  meshes_.reserve(totalSubMeshes);

  for (const auto &cm : converted) {
    for (const auto &subMesh : cm.subMeshes) {
      if (subMesh.vertices.empty() || subMesh.indices.empty()) {
        continue;
      }

      MeshGPUData gpu;
      gpu.name = cm.name;
      gpu.boneIndex = cm.boneIndex;
      gpu.vertexBuffer.create(context, subMesh.vertices);
      gpu.indexBuffer.create(context, subMesh.indices);

      // Store CPU copies for ray-triangle intersection
      gpu.cpuVertices = subMesh.vertices;
      gpu.cpuIndices = subMesh.indices;

      meshes_.push_back(std::move(gpu));
    }
  }
}

bool RenderableMesh::getTriangle(size_t meshIndex, size_t triangleIndex, glm::vec3 &v0,
                                 glm::vec3 &v1, glm::vec3 &v2) const {
  if (meshIndex >= meshes_.size()) {
    return false;
  }

  const auto &mesh = meshes_[meshIndex];
  const size_t indexOffset = triangleIndex * 3;

  if (indexOffset + 2 >= mesh.cpuIndices.size()) {
    return false;
  }

  const uint32_t i0 = mesh.cpuIndices[indexOffset];
  const uint32_t i1 = mesh.cpuIndices[indexOffset + 1];
  const uint32_t i2 = mesh.cpuIndices[indexOffset + 2];

  if (i0 >= mesh.cpuVertices.size() || i1 >= mesh.cpuVertices.size() ||
      i2 >= mesh.cpuVertices.size()) {
    return false;
  }

  v0 = mesh.cpuVertices[i0].position;
  v1 = mesh.cpuVertices[i1].position;
  v2 = mesh.cpuVertices[i2].position;

  return true;
}

void RenderableMesh::draw(vk::CommandBuffer cmd) const {
  for (const auto &mesh : meshes_) {
    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }
}

void RenderableMesh::destroy() {
  for (auto &mesh : meshes_) {
    mesh.vertexBuffer.destroy();
    mesh.indexBuffer.destroy();
  }
  meshes_.clear();
  bounds_ = BoundingBox{};
}

} // namespace w3d
