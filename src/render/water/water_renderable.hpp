#pragma once

#include "lib/gfx/buffer.hpp"
#include "lib/gfx/pipeline.hpp"

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include "lib/formats/ini/water_settings.hpp"
#include "lib/formats/map/types.hpp"
#include "lib/gfx/bounding_box.hpp"
#include "lib/gfx/renderable.hpp"
#include "lib/gfx/texture.hpp"
#include "render/water/water_mesh.hpp"

namespace w3d::gfx {
class VulkanContext;
} // namespace w3d::gfx

namespace w3d::water {

// GPU representation of a single water polygon.
struct GPUWaterPolygon {
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

// Renders all water surfaces loaded from a map's PolygonTriggers.
//
// Usage:
//   1. Call load() with the map triggers.
//   2. Call initPipeline() with a VulkanContext.
//   3. Each frame: call update(deltaSeconds) then drawWithPipeline().
class WaterRenderable : public gfx::IRenderable {
public:
  WaterRenderable() = default;
  ~WaterRenderable() override;

  WaterRenderable(const WaterRenderable &) = delete;
  WaterRenderable &operator=(const WaterRenderable &) = delete;

  // Build GPU buffers from the polygon triggers in a map.
  void load(gfx::VulkanContext &context, const std::vector<map::PolygonTrigger> &triggers);

  // Create the Vulkan pipeline (must be called before drawWithPipeline).
  void initPipeline(gfx::VulkanContext &context, gfx::TextureManager &textureManager,
                    uint32_t frameCount);

  // Apply INI water appearance settings (scroll rates, color, opacity).
  void applyWaterSettings(const ini::WaterSettings &settings,
                          ini::TimeOfDay tod = ini::TimeOfDay::Morning);

  // Advance animation time by deltaSeconds.
  void update(float deltaSeconds);

  // Update per-frame UBO (call once per frame before drawWithPipeline).
  void updateDescriptors(uint32_t frameIndex, vk::Buffer uniformBuffer, vk::DeviceSize uboSize);

  // Bind pipeline + descriptors, then emit draw calls.
  void drawWithPipeline(vk::CommandBuffer cmd, uint32_t frameIndex);

  // IRenderable interface.
  void draw(vk::CommandBuffer cmd) override;
  const gfx::BoundingBox &bounds() const override { return bounds_; }
  const char *typeName() const override { return "Water"; }
  bool isValid() const override { return !gpuPolygons_.empty(); }

  bool hasData() const { return !gpuPolygons_.empty(); }
  uint32_t polygonCount() const { return static_cast<uint32_t>(gpuPolygons_.size()); }

  void destroy();

private:
  void uploadPolygons(gfx::VulkanContext &context, const WaterMeshData &meshData);

  std::vector<GPUWaterPolygon> gpuPolygons_;
  gfx::BoundingBox bounds_;

  gfx::Pipeline pipeline_;
  gfx::DescriptorManager descriptorManager_;

  gfx::WaterPushConstant pushConstant_{};

  uint32_t waterTextureIndex_ = ~0u;
};

} // namespace w3d::water
