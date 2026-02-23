#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

#include "lib/formats/map/types.hpp"
#include "lib/gfx/bounding_box.hpp"

namespace w3d::terrain {

struct TerrainVertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
};

struct TerrainChunk {
  std::vector<TerrainVertex> vertices;
  std::vector<uint32_t> indices;
  gfx::BoundingBox bounds;
  int32_t chunkX = 0;
  int32_t chunkY = 0;
};

struct TerrainMeshData {
  std::vector<TerrainChunk> chunks;
  gfx::BoundingBox totalBounds;
  int32_t chunksX = 0;
  int32_t chunksY = 0;
};

constexpr int32_t CHUNK_SIZE = 32;

[[nodiscard]] glm::vec3 heightmapToWorld(const map::HeightMap &heightMap, int32_t x, int32_t y);

[[nodiscard]] glm::vec3 computeNormal(const map::HeightMap &heightMap, int32_t x, int32_t y);

[[nodiscard]] bool shouldFlipDiagonal(const map::HeightMap &heightMap, int32_t cellX,
                                      int32_t cellY);

[[nodiscard]] glm::vec2 computeTexCoord(const map::HeightMap &heightMap, int32_t x, int32_t y);

[[nodiscard]] TerrainChunk generateChunk(const map::HeightMap &heightMap, int32_t chunkX,
                                         int32_t chunkY, int32_t chunkSize = CHUNK_SIZE);

[[nodiscard]] TerrainMeshData generateTerrainMesh(const map::HeightMap &heightMap,
                                                  int32_t chunkSize = CHUNK_SIZE);

} // namespace w3d::terrain
