#include <cmath>

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

// ---------------------------------------------------------------------------
// Phase 3.5 – Cliff UV override tests
// Verify that cliff cells store raw tile-local UV in atlasCoord (not atlas-
// scaled), so the splatmap shader can use fragAtlasCoord directly with the
// texture array layer index from the SSBO.
// ---------------------------------------------------------------------------

namespace {

map::BlendTileData makeBlendTileDataWithCliff(int32_t width, int32_t height, int32_t cliffCellX,
                                              int32_t cliffCellY, float u0, float v0, float u1,
                                              float v1, float u2, float v2, float u3, float v3) {
  int32_t cellCount = width * height;
  map::BlendTileData btd;
  btd.dataSize = cellCount;
  btd.tileNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.blendTileNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.extraBlendTileNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.cliffInfoNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.cellCliffState.resize(static_cast<size_t>(cellCount), 0);

  map::CliffInfo cliff;
  cliff.tileIndex = 0;
  cliff.u0 = u0;
  cliff.v0 = v0;
  cliff.u1 = u1;
  cliff.v1 = v1;
  cliff.u2 = u2;
  cliff.v2 = v2;
  cliff.u3 = u3;
  cliff.v3 = v3;
  btd.cliffInfos.push_back(cliff);

  int32_t cellIdx = cliffCellY * width + cliffCellX;
  btd.cliffInfoNdxes[static_cast<size_t>(cellIdx)] = 1;

  return btd;
}

} // namespace

TEST_F(TerrainMeshTest, CliffCellAtlasCoordsAreRawTileLocalUV) {
  auto hm = createFlatHeightMap(3, 3, 100);

  const float cu0 = 0.1f, cv0 = 0.2f;
  const float cu1 = 0.9f, cv1 = 0.2f;
  const float cu2 = 0.1f, cv2 = 0.8f;
  const float cu3 = 0.9f, cv3 = 0.8f;

  auto btd = makeBlendTileDataWithCliff(2, 2, 0, 0, cu0, cv0, cu1, cv1, cu2, cv2, cu3, cv3);

  std::vector<TileUV> tileUVs;
  tileUVs.push_back({0.5f, 0.25f, 0.0625f, 0.0625f});

  auto meshData = generateTerrainMeshFromBlendData(hm, btd, tileUVs, 32);
  ASSERT_FALSE(meshData.chunks.empty());

  const auto &chunk = meshData.chunks[0];
  ASSERT_GE(chunk.vertices.size(), 4u);

  EXPECT_NEAR(chunk.vertices[0].atlasCoord.x, cu0, 1e-5f);
  EXPECT_NEAR(chunk.vertices[0].atlasCoord.y, cv0, 1e-5f);

  EXPECT_NEAR(chunk.vertices[1].atlasCoord.x, cu1, 1e-5f);
  EXPECT_NEAR(chunk.vertices[1].atlasCoord.y, cv1, 1e-5f);

  EXPECT_NEAR(chunk.vertices[2].atlasCoord.x, cu2, 1e-5f);
  EXPECT_NEAR(chunk.vertices[2].atlasCoord.y, cv2, 1e-5f);

  EXPECT_NEAR(chunk.vertices[3].atlasCoord.x, cu3, 1e-5f);
  EXPECT_NEAR(chunk.vertices[3].atlasCoord.y, cv3, 1e-5f);
}

TEST_F(TerrainMeshTest, CliffCellAtlasCoordsAreNotAtlasScaled) {
  auto hm = createFlatHeightMap(3, 3, 100);

  const float cu0 = 0.3f, cv0 = 0.4f;
  auto btd = makeBlendTileDataWithCliff(2, 2, 0, 0, cu0, cv0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

  std::vector<TileUV> tileUVs;
  tileUVs.push_back({0.5f, 0.25f, 0.0625f, 0.0625f});

  auto meshData = generateTerrainMeshFromBlendData(hm, btd, tileUVs, 32);
  ASSERT_FALSE(meshData.chunks.empty());

  const auto &chunk = meshData.chunks[0];
  ASSERT_GE(chunk.vertices.size(), 1u);

  EXPECT_NEAR(chunk.vertices[0].atlasCoord.x, cu0, 1e-5f);
  EXPECT_NEAR(chunk.vertices[0].atlasCoord.y, cv0, 1e-5f);

  float atlasScaledU = 0.5f + cu0 * 0.0625f;
  float atlasScaledV = 0.25f + cv0 * 0.0625f;
  EXPECT_GT(std::abs(chunk.vertices[0].atlasCoord.x - atlasScaledU), 1e-5f);
  EXPECT_GT(std::abs(chunk.vertices[0].atlasCoord.y - atlasScaledV), 1e-5f);
}

TEST_F(TerrainMeshTest, NonCliffCellAtlasCoordsAreNotRawCliffUV) {
  auto hm = createFlatHeightMap(3, 3, 100);

  int32_t cellCount = 2 * 2;
  map::BlendTileData btd;
  btd.dataSize = cellCount;
  btd.tileNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.blendTileNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.extraBlendTileNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.cliffInfoNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.cellCliffState.resize(static_cast<size_t>(cellCount), 0);

  std::vector<TileUV> tileUVs;
  tileUVs.push_back({0.0f, 0.0f, 0.5f, 0.5f});

  auto meshData = generateTerrainMeshFromBlendData(hm, btd, tileUVs, 32);
  ASSERT_FALSE(meshData.chunks.empty());

  const auto &chunk = meshData.chunks[0];
  ASSERT_GE(chunk.vertices.size(), 4u);

  EXPECT_FLOAT_EQ(chunk.vertices[0].atlasCoord.x, 0.0f);
  EXPECT_FLOAT_EQ(chunk.vertices[0].atlasCoord.y, 0.0f);
}

TEST_F(TerrainMeshTest, CliffCellDoesNotAffectNeighbouringCells) {
  auto hm = createFlatHeightMap(4, 4, 100);

  int32_t cellCount = 3 * 3;
  map::BlendTileData btd;
  btd.dataSize = cellCount;
  btd.tileNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.blendTileNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.extraBlendTileNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.cliffInfoNdxes.resize(static_cast<size_t>(cellCount), 0);
  btd.cellCliffState.resize(static_cast<size_t>(cellCount), 0);

  map::CliffInfo cliff;
  cliff.tileIndex = 0;
  cliff.u0 = 0.7f;
  cliff.v0 = 0.8f;
  cliff.u1 = 0.7f;
  cliff.v1 = 0.8f;
  cliff.u2 = 0.7f;
  cliff.v2 = 0.8f;
  cliff.u3 = 0.7f;
  cliff.v3 = 0.8f;
  btd.cliffInfos.push_back(cliff);
  btd.cliffInfoNdxes[0] = 1;

  std::vector<TileUV> tileUVs;
  tileUVs.push_back({0.0f, 0.0f, 1.0f, 1.0f});

  auto meshData = generateTerrainMeshFromBlendData(hm, btd, tileUVs, 32);
  ASSERT_FALSE(meshData.chunks.empty());

  const auto &chunk = meshData.chunks[0];
  ASSERT_GE(chunk.vertices.size(), 16u);

  for (size_t i = 4; i < chunk.vertices.size(); ++i) {
    bool isCliffUV = (std::abs(chunk.vertices[i].atlasCoord.x - 0.7f) < 1e-5f &&
                      std::abs(chunk.vertices[i].atlasCoord.y - 0.8f) < 1e-5f);
    EXPECT_FALSE(isCliffUV) << "Vertex " << i << " should not have cliff UV";
  }
}
