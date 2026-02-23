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
      vert.atlasCoord = vert.texCoord;

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

namespace {

glm::vec2 cliffAtlasUV(const map::CliffInfo &cliff, int32_t cornerIdx,
                       const std::vector<TileUV> &tileUVs) {
  if (cliff.tileIndex < 0 || static_cast<size_t>(cliff.tileIndex) >= tileUVs.size()) {
    return {0.0f, 0.0f};
  }

  const TileUV &tile = tileUVs[static_cast<size_t>(cliff.tileIndex)];

  float localU = 0.0f;
  float localV = 0.0f;
  switch (cornerIdx) {
  case 0:
    localU = cliff.u0;
    localV = cliff.v0;
    break;
  case 1:
    localU = cliff.u1;
    localV = cliff.v1;
    break;
  case 2:
    localU = cliff.u2;
    localV = cliff.v2;
    break;
  case 3:
  default:
    localU = cliff.u3;
    localV = cliff.v3;
    break;
  }

  return {tile.u + localU * tile.uSize, tile.v + localV * tile.vSize};
}

} // namespace

TerrainChunk generateChunkFromBlendData(const map::HeightMap &heightMap,
                                        const map::BlendTileData &blendTileData,
                                        const std::vector<TileUV> &tileUVs, int32_t chunkX,
                                        int32_t chunkY, int32_t chunkSize) {
  TerrainChunk chunk;
  chunk.chunkX = chunkX;
  chunk.chunkY = chunkY;

  int32_t startX = chunkX * chunkSize;
  int32_t startY = chunkY * chunkSize;
  int32_t endCellX = std::min(startX + chunkSize, heightMap.width - 1);
  int32_t endCellY = std::min(startY + chunkSize, heightMap.height - 1);

  int32_t cellsX = endCellX - startX;
  int32_t cellsY = endCellY - startY;

  if (cellsX <= 0 || cellsY <= 0) {
    return chunk;
  }

  chunk.vertices.reserve(static_cast<size_t>(cellsX * cellsY * 4));
  chunk.indices.reserve(static_cast<size_t>(cellsX * cellsY * 6));

  for (int32_t cy = startY; cy < endCellY; ++cy) {
    for (int32_t cx = startX; cx < endCellX; ++cx) {
      int32_t cellIdx = cy * heightMap.width + cx;

      bool isCliff = false;
      int32_t cliffNdx = 0;
      if (!blendTileData.cliffInfoNdxes.empty() &&
          cellIdx < static_cast<int32_t>(blendTileData.cliffInfoNdxes.size())) {
        cliffNdx = static_cast<int32_t>(blendTileData.cliffInfoNdxes[static_cast<size_t>(cellIdx)]);
        isCliff =
            cliffNdx > 0 && (cliffNdx - 1) < static_cast<int32_t>(blendTileData.cliffInfos.size());
      }

      TileUV cellTileUV{};
      if (!tileUVs.empty() && !blendTileData.tileNdxes.empty() &&
          cellIdx < static_cast<int32_t>(blendTileData.tileNdxes.size())) {
        int16_t tileNdx = blendTileData.tileNdxes[static_cast<size_t>(cellIdx)];
        cellTileUV = decodeTileNdxUV(tileNdx, tileUVs);
      }

      uint32_t baseIdx = static_cast<uint32_t>(chunk.vertices.size());

      auto makeVert = [&](int32_t vx, int32_t vy, int32_t corner) {
        TerrainVertex vert;
        vert.position = heightmapToWorld(heightMap, vx, vy);
        vert.normal = computeNormal(heightMap, vx, vy);
        vert.texCoord = computeTexCoord(heightMap, vx, vy);

        if (isCliff) {
          const auto &cliff = blendTileData.cliffInfos[static_cast<size_t>(cliffNdx - 1)];
          vert.atlasCoord = cliffAtlasUV(cliff, corner, tileUVs);
        } else {
          float localU = static_cast<float>(vx - cx) * cellTileUV.uSize;
          float localV = static_cast<float>(vy - cy) * cellTileUV.vSize;
          vert.atlasCoord = {cellTileUV.u + localU, cellTileUV.v + localV};
        }

        chunk.bounds.expand(vert.position);
        chunk.vertices.push_back(vert);
      };

      makeVert(cx, cy, 0);
      makeVert(cx + 1, cy, 1);
      makeVert(cx, cy + 1, 2);
      makeVert(cx + 1, cy + 1, 3);

      if (shouldFlipDiagonal(heightMap, cx, cy)) {
        chunk.indices.push_back(baseIdx + 0);
        chunk.indices.push_back(baseIdx + 2);
        chunk.indices.push_back(baseIdx + 1);

        chunk.indices.push_back(baseIdx + 1);
        chunk.indices.push_back(baseIdx + 2);
        chunk.indices.push_back(baseIdx + 3);
      } else {
        chunk.indices.push_back(baseIdx + 0);
        chunk.indices.push_back(baseIdx + 2);
        chunk.indices.push_back(baseIdx + 3);

        chunk.indices.push_back(baseIdx + 0);
        chunk.indices.push_back(baseIdx + 3);
        chunk.indices.push_back(baseIdx + 1);
      }
    }
  }

  return chunk;
}

TerrainMeshData generateTerrainMeshFromBlendData(const map::HeightMap &heightMap,
                                                 const map::BlendTileData &blendTileData,
                                                 const std::vector<TileUV> &tileUVs,
                                                 int32_t chunkSize) {
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
      auto chunk = generateChunkFromBlendData(heightMap, blendTileData, tileUVs, cx, cy, chunkSize);
      meshData.totalBounds.expand(chunk.bounds);
      meshData.chunks.push_back(std::move(chunk));
    }
  }

  return meshData;
}

} // namespace w3d::terrain
