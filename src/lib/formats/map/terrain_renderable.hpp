#pragma once

#include "lib/gfx/buffer.hpp"
#include "lib/gfx/bounding_box.hpp"
#include "lib/gfx/pipeline.hpp"
#include "lib/gfx/renderable.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace map {

// Forward declarations
struct TerrainData;

// Terrain renderable - converts heightmap data to GPU mesh for rendering
class TerrainRenderable : public w3d::gfx::IRenderable {
public:
  TerrainRenderable() = default;
  ~TerrainRenderable();

  TerrainRenderable(const TerrainRenderable &) = delete;
  TerrainRenderable &operator=(const TerrainRenderable &) = delete;

  TerrainRenderable(TerrainRenderable &&other) noexcept;
  TerrainRenderable &operator=(TerrainRenderable &&other) noexcept;

  // Load terrain from parsed heightmap data
  bool load(w3d::gfx::VulkanContext &context, const TerrainData &data);

  // Free GPU resources
  void destroy();

  // IRenderable implementation
  void draw(vk::CommandBuffer cmd) override;
  const w3d::gfx::BoundingBox &bounds() const override;
  const char *typeName() const override { return "Terrain"; }
  bool isValid() const override { return vertexBuffer_.vertexCount() > 0; }

private:
  // Convert heightmap grid to vertices with world space coordinates
  std::vector<w3d::gfx::Vertex> generateVertices(const TerrainData &data) const;

  // Generate triangle indices for the heightmap grid
  std::vector<uint32_t> generateIndices(const TerrainData &data) const;

  // Calculate UV coordinates for a tile at (x, y)
  glm::vec2 calculateUV(int32_t x, int32_t y, int32_t width, int32_t height) const;

  // Compute bounding box from vertices
  w3d::gfx::BoundingBox computeBounds(const std::vector<w3d::gfx::Vertex> &vertices) const;

  w3d::gfx::VertexBuffer<w3d::gfx::Vertex> vertexBuffer_;
  w3d::gfx::IndexBuffer indexBuffer_;
  w3d::gfx::BoundingBox bounds_;
};

} // namespace map
