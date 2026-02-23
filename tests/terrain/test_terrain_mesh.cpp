#include "render/terrain/terrain_mesh.hpp"

#include <gtest/gtest.h>

using namespace w3d::terrain;
using namespace w3d;

namespace {

map::HeightMap createFlatHeightMap(int32_t width, int32_t height, uint8_t value = 128) {
  map::HeightMap hm;
  hm.width = width;
  hm.height = height;
  hm.borderSize = 0;
  hm.data.assign(static_cast<size_t>(width * height), value);
  return hm;
}

map::HeightMap createSlopedHeightMap(int32_t width, int32_t height) {
  map::HeightMap hm;
  hm.width = width;
  hm.height = height;
  hm.borderSize = 0;
  hm.data.resize(static_cast<size_t>(width * height));
  for (int32_t y = 0; y < height; ++y) {
    for (int32_t x = 0; x < width; ++x) {
      hm.data[y * width + x] = static_cast<uint8_t>(x + y);
    }
  }
  return hm;
}

} // namespace

class TerrainMeshTest : public ::testing::Test {
protected:
  map::HeightMap flat4x4_ = createFlatHeightMap(4, 4, 100);
  map::HeightMap sloped5x5_ = createSlopedHeightMap(5, 5);
};

TEST_F(TerrainMeshTest, HeightmapToWorldConvertsCorrectly) {
  auto pos = heightmapToWorld(flat4x4_, 2, 3);

  EXPECT_FLOAT_EQ(pos.x, 2.0f * map::MAP_XY_FACTOR);
  EXPECT_FLOAT_EQ(pos.y, 100.0f * map::MAP_HEIGHT_SCALE);
  EXPECT_FLOAT_EQ(pos.z, 3.0f * map::MAP_XY_FACTOR);
}

TEST_F(TerrainMeshTest, HeightmapToWorldOrigin) {
  auto pos = heightmapToWorld(flat4x4_, 0, 0);

  EXPECT_FLOAT_EQ(pos.x, 0.0f);
  EXPECT_FLOAT_EQ(pos.y, 100.0f * map::MAP_HEIGHT_SCALE);
  EXPECT_FLOAT_EQ(pos.z, 0.0f);
}

TEST_F(TerrainMeshTest, ComputeNormalFlatTerrain) {
  auto normal = computeNormal(flat4x4_, 2, 2);

  EXPECT_NEAR(normal.x, 0.0f, 0.01f);
  EXPECT_NEAR(normal.y, 1.0f, 0.01f);
  EXPECT_NEAR(normal.z, 0.0f, 0.01f);
}

TEST_F(TerrainMeshTest, ComputeNormalSlopedTerrain) {
  auto normal = computeNormal(sloped5x5_, 2, 2);

  EXPECT_GT(normal.y, 0.0f);
  float len = glm::length(normal);
  EXPECT_NEAR(len, 1.0f, 0.001f);
}

TEST_F(TerrainMeshTest, ComputeTexCoordCorners) {
  auto uv00 = computeTexCoord(flat4x4_, 0, 0);
  EXPECT_FLOAT_EQ(uv00.x, 0.0f);
  EXPECT_FLOAT_EQ(uv00.y, 0.0f);

  auto uv33 = computeTexCoord(flat4x4_, 3, 3);
  EXPECT_FLOAT_EQ(uv33.x, 1.0f);
  EXPECT_FLOAT_EQ(uv33.y, 1.0f);
}

TEST_F(TerrainMeshTest, ShouldFlipDiagonalFlatTerrain) {
  bool flip = shouldFlipDiagonal(flat4x4_, 1, 1);
  EXPECT_FALSE(flip);
}

TEST_F(TerrainMeshTest, GenerateChunkProducesCorrectVertexCount) {
  auto chunk = generateChunk(flat4x4_, 0, 0, 32);

  EXPECT_EQ(chunk.vertices.size(), 16u);
  EXPECT_EQ(chunk.chunkX, 0);
  EXPECT_EQ(chunk.chunkY, 0);
}

TEST_F(TerrainMeshTest, GenerateChunkProducesCorrectIndexCount) {
  auto chunk = generateChunk(flat4x4_, 0, 0, 32);

  EXPECT_EQ(chunk.indices.size(), 3u * 3u * 6u);
}

TEST_F(TerrainMeshTest, GenerateChunkBoundsAreValid) {
  auto chunk = generateChunk(flat4x4_, 0, 0, 32);

  EXPECT_TRUE(chunk.bounds.valid());
  EXPECT_FLOAT_EQ(chunk.bounds.min.x, 0.0f);
  EXPECT_FLOAT_EQ(chunk.bounds.min.z, 0.0f);
  EXPECT_FLOAT_EQ(chunk.bounds.max.x, 3.0f * map::MAP_XY_FACTOR);
  EXPECT_FLOAT_EQ(chunk.bounds.max.z, 3.0f * map::MAP_XY_FACTOR);
}

TEST_F(TerrainMeshTest, GenerateTerrainMeshSmallMap) {
  auto meshData = generateTerrainMesh(flat4x4_, 32);

  EXPECT_EQ(meshData.chunksX, 1);
  EXPECT_EQ(meshData.chunksY, 1);
  EXPECT_EQ(meshData.chunks.size(), 1u);
  EXPECT_TRUE(meshData.totalBounds.valid());
}

TEST_F(TerrainMeshTest, GenerateTerrainMeshMultipleChunks) {
  auto largeMap = createFlatHeightMap(66, 66, 128);
  auto meshData = generateTerrainMesh(largeMap, 32);

  EXPECT_EQ(meshData.chunksX, 3);
  EXPECT_EQ(meshData.chunksY, 3);
  EXPECT_EQ(meshData.chunks.size(), 9u);
}

TEST_F(TerrainMeshTest, GenerateTerrainMeshInvalidHeightMap) {
  map::HeightMap invalid;
  auto meshData = generateTerrainMesh(invalid, 32);

  EXPECT_EQ(meshData.chunks.size(), 0u);
  EXPECT_EQ(meshData.chunksX, 0);
  EXPECT_EQ(meshData.chunksY, 0);
}

TEST_F(TerrainMeshTest, GenerateTerrainMeshTooSmallHeightMap) {
  auto tinyMap = createFlatHeightMap(1, 1, 100);
  auto meshData = generateTerrainMesh(tinyMap, 32);

  EXPECT_EQ(meshData.chunks.size(), 0u);
}

TEST_F(TerrainMeshTest, GenerateTerrainMeshMinimalHeightMap) {
  auto minMap = createFlatHeightMap(2, 2, 100);
  auto meshData = generateTerrainMesh(minMap, 32);

  EXPECT_EQ(meshData.chunksX, 1);
  EXPECT_EQ(meshData.chunksY, 1);
  EXPECT_EQ(meshData.chunks.size(), 1u);

  auto &chunk = meshData.chunks[0];
  EXPECT_EQ(chunk.vertices.size(), 4u);
  EXPECT_EQ(chunk.indices.size(), 6u);
}

TEST_F(TerrainMeshTest, ChunkVertexPositionsMatchHeightmap) {
  auto chunk = generateChunk(flat4x4_, 0, 0, 32);

  auto &v0 = chunk.vertices[0];
  EXPECT_FLOAT_EQ(v0.position.x, 0.0f);
  EXPECT_FLOAT_EQ(v0.position.y, 100.0f * map::MAP_HEIGHT_SCALE);
  EXPECT_FLOAT_EQ(v0.position.z, 0.0f);

  auto &v3 = chunk.vertices[3];
  EXPECT_FLOAT_EQ(v3.position.x, 3.0f * map::MAP_XY_FACTOR);
  EXPECT_FLOAT_EQ(v3.position.z, 0.0f);
}

TEST_F(TerrainMeshTest, ChunkIndicesAreWithinBounds) {
  auto chunk = generateChunk(sloped5x5_, 0, 0, 32);

  uint32_t maxIndex = static_cast<uint32_t>(chunk.vertices.size());
  for (uint32_t idx : chunk.indices) {
    EXPECT_LT(idx, maxIndex);
  }
}

TEST_F(TerrainMeshTest, ChunkIndicesFormTriangles) {
  auto chunk = generateChunk(flat4x4_, 0, 0, 32);

  EXPECT_EQ(chunk.indices.size() % 3, 0u);
}

TEST_F(TerrainMeshTest, SlopedTerrainHasVaryingHeights) {
  auto chunk = generateChunk(sloped5x5_, 0, 0, 32);

  float minY = std::numeric_limits<float>::max();
  float maxY = std::numeric_limits<float>::lowest();
  for (const auto &v : chunk.vertices) {
    minY = std::min(minY, v.position.y);
    maxY = std::max(maxY, v.position.y);
  }

  EXPECT_GT(maxY - minY, 0.0f);
}

TEST_F(TerrainMeshTest, GenerateChunkWithExactChunkSize) {
  auto map33 = createFlatHeightMap(33, 33, 50);
  auto meshData = generateTerrainMesh(map33, 32);

  EXPECT_EQ(meshData.chunksX, 1);
  EXPECT_EQ(meshData.chunksY, 1);
  EXPECT_EQ(meshData.chunks.size(), 1u);

  auto &chunk = meshData.chunks[0];
  EXPECT_EQ(chunk.vertices.size(), 33u * 33u);
  EXPECT_EQ(chunk.indices.size(), 32u * 32u * 6u);
}

TEST_F(TerrainMeshTest, GenerateChunkSmallChunkSize) {
  auto meshData = generateTerrainMesh(sloped5x5_, 2);

  EXPECT_EQ(meshData.chunksX, 2);
  EXPECT_EQ(meshData.chunksY, 2);
  EXPECT_EQ(meshData.chunks.size(), 4u);

  for (const auto &chunk : meshData.chunks) {
    EXPECT_FALSE(chunk.vertices.empty());
    EXPECT_FALSE(chunk.indices.empty());
    EXPECT_TRUE(chunk.bounds.valid());
    EXPECT_EQ(chunk.indices.size() % 3, 0u);
  }
}

TEST_F(TerrainMeshTest, AllChunksCoverEntireHeightmap) {
  auto map10 = createFlatHeightMap(10, 10, 64);
  auto meshData = generateTerrainMesh(map10, 4);

  size_t totalIndices = 0;
  for (const auto &chunk : meshData.chunks) {
    totalIndices += chunk.indices.size();
  }

  EXPECT_EQ(totalIndices, 9u * 9u * 6u);
}

TEST_F(TerrainMeshTest, NormalsAreUnitLength) {
  auto chunk = generateChunk(sloped5x5_, 0, 0, 32);

  for (const auto &v : chunk.vertices) {
    float len = glm::length(v.normal);
    EXPECT_NEAR(len, 1.0f, 0.01f);
  }
}

TEST_F(TerrainMeshTest, TexCoordsAreInRange) {
  auto chunk = generateChunk(sloped5x5_, 0, 0, 32);

  for (const auto &v : chunk.vertices) {
    EXPECT_GE(v.texCoord.x, 0.0f);
    EXPECT_LE(v.texCoord.x, 1.0f);
    EXPECT_GE(v.texCoord.y, 0.0f);
    EXPECT_LE(v.texCoord.y, 1.0f);
  }
}

TEST_F(TerrainMeshTest, TotalBoundsContainAllChunkBounds) {
  auto map20 = createFlatHeightMap(20, 20, 80);
  auto meshData = generateTerrainMesh(map20, 8);

  for (const auto &chunk : meshData.chunks) {
    EXPECT_GE(chunk.bounds.min.x, meshData.totalBounds.min.x);
    EXPECT_GE(chunk.bounds.min.y, meshData.totalBounds.min.y);
    EXPECT_GE(chunk.bounds.min.z, meshData.totalBounds.min.z);
    EXPECT_LE(chunk.bounds.max.x, meshData.totalBounds.max.x);
    EXPECT_LE(chunk.bounds.max.y, meshData.totalBounds.max.y);
    EXPECT_LE(chunk.bounds.max.z, meshData.totalBounds.max.z);
  }
}
