#include "terrain_renderable.hpp"

#include "lib/gfx/vulkan_context.hpp"

#include <glm/glm.hpp>

#include <algorithm>

#include "lib/formats/map/terrain_types.hpp"

namespace map {

TerrainRenderable::~TerrainRenderable() {
  destroy();
}

TerrainRenderable::TerrainRenderable(TerrainRenderable &&other) noexcept
    : vertexBuffer_(std::move(other.vertexBuffer_)), indexBuffer_(std::move(other.indexBuffer_)),
      bounds_(other.bounds_) {}

TerrainRenderable &TerrainRenderable::operator=(TerrainRenderable &&other) noexcept {
  if (this != &other) {
    destroy();
    vertexBuffer_ = std::move(other.vertexBuffer_);
    indexBuffer_ = std::move(other.indexBuffer_);
    bounds_ = other.bounds_;
  }
  return *this;
}

bool TerrainRenderable::load(w3d::gfx::VulkanContext &context, const TerrainData &data) {
  // Clean up any existing data
  destroy();

  // Validate input
  if (!data.isValid()) {
    return false;
  }

  const auto &heightmap = data.heightmap;
  if (heightmap.width < 2 || heightmap.height < 2) {
    return false;
  }

  // Generate vertices from heightmap
  std::vector<w3d::gfx::Vertex> vertices = generateVertices(data);

  // Generate triangle indices
  std::vector<uint32_t> indices = generateIndices(data);

  // Compute bounding box
  bounds_ = computeBounds(vertices);

  // Create GPU buffers
  vertexBuffer_.create(context, vertices);
  indexBuffer_.create(context, indices);

  return true;
}

void TerrainRenderable::destroy() {
  vertexBuffer_.destroy();
  indexBuffer_.destroy();
  bounds_ = w3d::gfx::BoundingBox{};
}

void TerrainRenderable::draw(vk::CommandBuffer cmd) {
  if (!isValid()) {
    return;
  }

  vk::Buffer vertexBuffers[] = {vertexBuffer_.buffer()};
  vk::DeviceSize offsets[] = {0};
  cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
  cmd.bindIndexBuffer(indexBuffer_.buffer(), 0, vk::IndexType::eUint32);
  cmd.drawIndexed(indexBuffer_.indexCount(), 1, 0, 0, 0);
}

const w3d::gfx::BoundingBox &TerrainRenderable::bounds() const {
  return bounds_;
}

std::vector<w3d::gfx::Vertex> TerrainRenderable::generateVertices(const TerrainData &data) const {
  const auto &heightmap = data.heightmap;
  const uint32_t width = heightmap.width;
  const uint32_t height = heightmap.height;

  std::vector<w3d::gfx::Vertex> vertices;
  vertices.reserve(width * height);

  // Calculate normal for terrain (pointing up)
  const glm::vec3 normal(0.0f, 1.0f, 0.0f);

  // Create a vertex for each heightmap cell
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      w3d::gfx::Vertex vertex{};

      // Position: convert heightmap grid to world space
      // x = x * MAP_XY_FACTOR
      // y = height * MAP_HEIGHT_SCALE (height value is 0-255)
      // z = y * MAP_XY_FACTOR
      uint32_t heightIndex = y * width + x;
      float h = static_cast<float>(heightmap.heights[heightIndex]);

      vertex.position.x = static_cast<float>(x) * MAP_XY_FACTOR;
      vertex.position.y = h * MAP_HEIGHT_SCALE;
      vertex.position.z = static_cast<float>(y) * MAP_XY_FACTOR;

      // Normal: pointing up for terrain
      vertex.normal = normal;

      // Texture coordinates: based on tile position
      vertex.texCoord = calculateUV(static_cast<int32_t>(x), static_cast<int32_t>(y),
                                    static_cast<int32_t>(width), static_cast<int32_t>(height));

      // Color: white (can be tinted by material)
      vertex.color = glm::vec3(1.0f);

      vertices.push_back(vertex);
    }
  }

  return vertices;
}

std::vector<uint32_t> TerrainRenderable::generateIndices(const TerrainData &data) const {
  const auto &heightmap = data.heightmap;
  const uint32_t width = heightmap.width;
  const uint32_t height = heightmap.height;

  // Each cell in the heightmap grid is a quad made of 2 triangles
  // We have (width-1) * (height-1) quads
  const uint32_t quadCountX = width - 1;
  const uint32_t quadCountY = height - 1;
  const uint32_t quadCount = quadCountX * quadCountY;

  // 6 indices per quad (2 triangles)
  std::vector<uint32_t> indices;
  indices.reserve(quadCount * 6);

  for (uint32_t y = 0; y < quadCountY; ++y) {
    for (uint32_t x = 0; x < quadCountX; ++x) {
      // Vertex indices for this quad
      uint32_t topLeft = y * width + x;
      uint32_t topRight = topLeft + 1;
      uint32_t bottomLeft = (y + 1) * width + x;
      uint32_t bottomRight = bottomLeft + 1;

      // First triangle: top-left, bottom-left, top-right
      indices.push_back(topLeft);
      indices.push_back(bottomLeft);
      indices.push_back(topRight);

      // Second triangle: top-right, bottom-left, bottom-right
      indices.push_back(topRight);
      indices.push_back(bottomLeft);
      indices.push_back(bottomRight);
    }
  }

  return indices;
}

glm::vec2 TerrainRenderable::calculateUV(int32_t x, int32_t y, int32_t width,
                                         int32_t height) const {
  // Simple UV mapping based on grid position
  // UV coordinates range from 0 to 1 across the entire terrain
  float u = static_cast<float>(x) / static_cast<float>(width - 1);
  float v = static_cast<float>(y) / static_cast<float>(height - 1);

  return glm::vec2(u, v);
}

w3d::gfx::BoundingBox
TerrainRenderable::computeBounds(const std::vector<w3d::gfx::Vertex> &vertices) const {
  if (vertices.empty()) {
    return {};
  }

  glm::vec3 min = vertices[0].position;
  glm::vec3 max = vertices[0].position;

  for (const auto &vertex : vertices) {
    min.x = std::min(min.x, vertex.position.x);
    min.y = std::min(min.y, vertex.position.y);
    min.z = std::min(min.z, vertex.position.z);

    max.x = std::max(max.x, vertex.position.x);
    max.y = std::max(max.y, vertex.position.y);
    max.z = std::max(max.z, vertex.position.z);
  }

  w3d::gfx::BoundingBox bounds{};
  bounds.min = min;
  bounds.max = max;
  return bounds;
}

} // namespace map
