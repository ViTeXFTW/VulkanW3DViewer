#include "renderable_mesh.hpp"

#include "core/vulkan_context.hpp"

#include "mesh_converter.hpp"

namespace w3d {

RenderableMesh::~RenderableMesh() {
  destroy();
}

void RenderableMesh::load(VulkanContext &context, const W3DFile &file) {
  destroy(); // Clean up any existing data

  auto converted = MeshConverter::convertAll(file);
  bounds_ = MeshConverter::combinedBounds(converted);

  meshes_.reserve(converted.size());
  for (auto &cm : converted) {
    if (cm.vertices.empty() || cm.indices.empty()) {
      continue;
    }

    MeshGPUData gpu;
    gpu.name = cm.name;
    gpu.vertexBuffer.create(context, cm.vertices);
    gpu.indexBuffer.create(context, cm.indices);
    meshes_.push_back(std::move(gpu));
  }
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
