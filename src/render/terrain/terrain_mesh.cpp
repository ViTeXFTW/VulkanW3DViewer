#include "render/terrain/terrain_mesh.hpp"

#include <algorithm>
#include <cmath>

namespace w3d::terrain {

glm::vec3 heightmapToWorld(const map::HeightMap &heightMap, int32_t x, int32_t y) {
  float worldX = static_cast<float>(x) * map::MAP_XY_FACTOR;
  float worldY = static_cast<float>(y) * map::MAP_XY_FACTOR;
  float worldZ = heightMap.getWorldHeight(x, y);
  return {worldX, worldZ, worldY};
}

glm::vec3 computeNormal(const map::HeightMap &heightMap, int32_t x, int32_t y) {
  float hL = heightMap.getWorldHeight(x - 1, y);
  float hR = heightMap.getWorldHeight(x + 1, y);
  float hD = heightMap.getWorldHeight(x, y - 1);
  float hU = heightMap.getWorldHeight(x, y + 1);

  glm::vec3 normal{(hL - hR) / (2.0f * map::MAP_XY_FACTOR), 1.0f,
                   (hD - hU) / (2.0f * map::MAP_XY_FACTOR)};

  return glm::normalize(normal);
}

bool shouldFlipDiagonal(const map::HeightMap &heightMap, int32_t cellX, int32_t cellY) {
  float h00 = heightMap.getWorldHeight(cellX, cellY);
  float h10 = heightMap.getWorldHeight(cellX + 1, cellY);
  float h01 = heightMap.getWorldHeight(cellX, cellY + 1);
  float h11 = heightMap.getWorldHeight(cellX + 1, cellY + 1);

  float diag1 = std::abs(h00 - h11);
  float diag2 = std::abs(h10 - h01);

  return diag2 < diag1;
}

glm::vec2 computeTexCoord(const map::HeightMap &heightMap, int32_t x, int32_t y) {
  float u = static_cast<float>(x) / static_cast<float>(heightMap.width - 1);
  float v = static_cast<float>(y) / static_cast<float>(heightMap.height - 1);
  return {u, v};
}

TerrainChunk generateChunk(const map::HeightMap &heightMap, int32_t chunkX, int32_t chunkY,
                           int32_t chunkSize) {
  TerrainChunk chunk;
  chunk.chunkX = chunkX;
  chunk.chunkY = chunkY;

  int32_t startX = chunkX * chunkSize;
  int32_t startY = chunkY * chunkSize;
  int32_t endX = std::min(startX + chunkSize, heightMap.width - 1);
  int32_t endY = std::min(startY + chunkSize, heightMap.height - 1);

  int32_t vertsX = endX - startX + 1;
  int32_t vertsY = endY - startY + 1;

  chunk.vertices.reserve(static_cast<size_t>(vertsX * vertsY));

  for (int32_t y = startY; y <= endY; ++y) {
    for (int32_t x = startX; x <= endX; ++x) {
      TerrainVertex vert;
      vert.position = heightmapToWorld(heightMap, x, y);
      vert.normal = computeNormal(heightMap, x, y);
      vert.texCoord = computeTexCoord(heightMap, x, y);

      chunk.bounds.expand(vert.position);
      chunk.vertices.push_back(vert);
    }
  }

  int32_t cellsX = vertsX - 1;
  int32_t cellsY = vertsY - 1;

  if (cellsX <= 0 || cellsY <= 0) {
    return chunk;
  }

  chunk.indices.reserve(static_cast<size_t>(cellsX * cellsY * 6));

  for (int32_t cy = 0; cy < cellsY; ++cy) {
    for (int32_t cx = 0; cx < cellsX; ++cx) {
      uint32_t topLeft = static_cast<uint32_t>(cy * vertsX + cx);
      uint32_t topRight = topLeft + 1;
      uint32_t bottomLeft = static_cast<uint32_t>((cy + 1) * vertsX + cx);
      uint32_t bottomRight = bottomLeft + 1;

      int32_t worldCellX = startX + cx;
      int32_t worldCellY = startY + cy;

      if (shouldFlipDiagonal(heightMap, worldCellX, worldCellY)) {
        chunk.indices.push_back(topLeft);
        chunk.indices.push_back(bottomLeft);
        chunk.indices.push_back(topRight);

        chunk.indices.push_back(topRight);
        chunk.indices.push_back(bottomLeft);
        chunk.indices.push_back(bottomRight);
      } else {
        chunk.indices.push_back(topLeft);
        chunk.indices.push_back(bottomLeft);
        chunk.indices.push_back(bottomRight);

        chunk.indices.push_back(topLeft);
        chunk.indices.push_back(bottomRight);
        chunk.indices.push_back(topRight);
      }
    }
  }

  return chunk;
}

TerrainMeshData generateTerrainMesh(const map::HeightMap &heightMap, int32_t chunkSize) {
  TerrainMeshData meshData;

  if (!heightMap.isValid() || heightMap.width < 2 || heightMap.height < 2) {
    return meshData;
  }

  int32_t cellsX = heightMap.width - 1;
  int32_t cellsY = heightMap.height - 1;

  meshData.chunksX = (cellsX + chunkSize - 1) / chunkSize;
  meshData.chunksY = (cellsY + chunkSize - 1) / chunkSize;

  meshData.chunks.reserve(static_cast<size_t>(meshData.chunksX * meshData.chunksY));

  for (int32_t cy = 0; cy < meshData.chunksY; ++cy) {
    for (int32_t cx = 0; cx < meshData.chunksX; ++cx) {
      auto chunk = generateChunk(heightMap, cx, cy, chunkSize);
      meshData.totalBounds.expand(chunk.bounds);
      meshData.chunks.push_back(std::move(chunk));
    }
  }

  return meshData;
}

} // namespace w3d::terrain
