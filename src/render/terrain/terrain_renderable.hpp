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
#include "render/lighting_state.hpp"
#include "render/terrain/terrain_atlas.hpp"
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

  void loadWithBlendData(gfx::VulkanContext &context, const map::HeightMap &heightMap,
                         const map::BlendTileData &blendTileData,
                         const std::vector<TileUV> &tileUVs, const map::GlobalLighting &lighting);

  void draw(vk::CommandBuffer cmd) override;

  const gfx::BoundingBox &bounds() const override { return bounds_; }

  const char *typeName() const override { return "Terrain"; }

  bool isValid() const override { return !gpuChunks_.empty(); }

  bool hasData() const { return !gpuChunks_.empty(); }

  void destroy();

  /** Apply lighting from a parsed GlobalLighting chunk (legacy helper). */
  void setLighting(const map::GlobalLighting &lighting);

  /**
   * Apply lighting from a LightingState (Phase 6.1/6.2/6.3).
   * The LightingState handles time-of-day selection, shadow colour, and cloud
   * animation – so prefer this over setLighting() when a LightingState is
   * available.
   */
  void applyLightingState(const LightingState &lightingState);

  /**
   * Advance the cloud shadow animation by deltaSeconds (Phase 6.3).
   * Must be called once per frame when cloud shadows are active.
   */
  void update(float deltaSeconds);

  gfx::Pipeline &pipeline() { return pipeline_; }
  gfx::DescriptorManager &descriptorManager() { return descriptorManager_; }

  void initPipeline(gfx::VulkanContext &context, gfx::TextureManager &textureManager,
                    uint32_t frameCount);

  void initPipelineWithAtlas(gfx::VulkanContext &context, gfx::TextureManager &textureManager,
                             const TerrainAtlasData &atlasData, uint32_t frameCount);

  void updateDescriptors(uint32_t frameIndex, vk::Buffer uniformBuffer, vk::DeviceSize uboSize);

  void drawWithPipeline(vk::CommandBuffer cmd, uint32_t frameIndex);

  void updateFrustum(const glm::mat4 &viewProjection);

  uint32_t visibleChunkCount() const { return visibleChunkCount_; }
  uint32_t totalChunkCount() const { return static_cast<uint32_t>(gpuChunks_.size()); }

  bool hasAtlas() const { return atlasTextureIndex_ != ~0u; }

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

  uint32_t atlasTextureIndex_ = ~0u;
};

} // namespace w3d::terrain
