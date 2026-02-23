#pragma once

#include "lib/gfx/buffer.hpp"
#include "lib/gfx/pipeline.hpp"

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

#include "lib/formats/map/types.hpp"
#include "lib/gfx/bounding_box.hpp"
#include "lib/gfx/frustum.hpp"
#include "lib/gfx/renderable.hpp"
#include "lib/gfx/texture.hpp"
#include "render/terrain/terrain_mesh.hpp"

namespace w3d::gfx {
class VulkanContext;
} // namespace w3d::gfx

namespace w3d::terrain {

struct GPUTerrainChunk {
  gfx::StagedBuffer vertexBuffer;
  gfx::StagedBuffer indexBuffer;
  uint32_t indexCount = 0;
  gfx::BoundingBox bounds;

  void destroy() {
    vertexBuffer.destroy();
    indexBuffer.destroy();
    indexCount = 0;
  }
};

class TerrainRenderable : public gfx::IRenderable {
public:
  TerrainRenderable() = default;
  ~TerrainRenderable() override;

  TerrainRenderable(const TerrainRenderable &) = delete;
  TerrainRenderable &operator=(const TerrainRenderable &) = delete;

  void load(gfx::VulkanContext &context, const map::HeightMap &heightMap,
            const map::GlobalLighting &lighting);

  void draw(vk::CommandBuffer cmd) override;

  const gfx::BoundingBox &bounds() const override { return bounds_; }

  const char *typeName() const override { return "Terrain"; }

  bool isValid() const override { return !gpuChunks_.empty(); }

  bool hasData() const { return !gpuChunks_.empty(); }

  void destroy();

  void setLighting(const map::GlobalLighting &lighting);

  gfx::Pipeline &pipeline() { return pipeline_; }
  gfx::DescriptorManager &descriptorManager() { return descriptorManager_; }

  void initPipeline(gfx::VulkanContext &context, gfx::TextureManager &textureManager,
                    uint32_t frameCount);

  void updateDescriptors(uint32_t frameIndex, vk::Buffer uniformBuffer, vk::DeviceSize uboSize);

  void drawWithPipeline(vk::CommandBuffer cmd, uint32_t frameIndex);

  void updateFrustum(const glm::mat4 &viewProjection);

  uint32_t visibleChunkCount() const { return visibleChunkCount_; }
  uint32_t totalChunkCount() const { return static_cast<uint32_t>(gpuChunks_.size()); }

private:
  void uploadChunks(gfx::VulkanContext &context, const TerrainMeshData &meshData);

  std::vector<GPUTerrainChunk> gpuChunks_;
  gfx::BoundingBox bounds_;

  gfx::Pipeline pipeline_;
  gfx::DescriptorManager descriptorManager_;

  gfx::TerrainPushConstant pushConstant_{};
  gfx::Frustum frustum_;
  uint32_t visibleChunkCount_ = 0;
  bool frustumValid_ = false;
};

} // namespace w3d::terrain
