#include "render/terrain/terrain_atlas.hpp"

#include <gtest/gtest.h>

using namespace w3d::terrain;
using namespace w3d;

class TerrainAtlasTest : public ::testing::Test {};

TEST_F(TerrainAtlasTest, DecodeTileIndexExtractsTop14Bits) {
  int16_t tileNdx = static_cast<int16_t>(0x0014);
  EXPECT_EQ(decodeTileIndex(tileNdx), 5);
}

TEST_F(TerrainAtlasTest, DecodeTileIndexZero) {
  EXPECT_EQ(decodeTileIndex(0), 0);
}

TEST_F(TerrainAtlasTest, DecodeQuadrantExtractsBottom2Bits) {
  int16_t tileNdx0 = static_cast<int16_t>(0x0000);
  int16_t tileNdx1 = static_cast<int16_t>(0x0001);
  int16_t tileNdx2 = static_cast<int16_t>(0x0002);
  int16_t tileNdx3 = static_cast<int16_t>(0x0003);

  EXPECT_EQ(decodeQuadrant(tileNdx0), 0);
  EXPECT_EQ(decodeQuadrant(tileNdx1), 1);
  EXPECT_EQ(decodeQuadrant(tileNdx2), 2);
  EXPECT_EQ(decodeQuadrant(tileNdx3), 3);
}

TEST_F(TerrainAtlasTest, DecodeTileIndexAndQuadrantCombined) {
  int16_t tileNdx = static_cast<int16_t>((3 << 2) | 2);
  EXPECT_EQ(decodeTileIndex(tileNdx), 3);
  EXPECT_EQ(decodeQuadrant(tileNdx), 2);
}

TEST_F(TerrainAtlasTest, ComputeQuadrantUVTopLeft) {
  TileUV tile{0.0f, 0.0f, 0.5f, 0.5f};
  auto uv = computeQuadrantUV(tile, 0);
  EXPECT_FLOAT_EQ(uv.u, 0.0f);
  EXPECT_FLOAT_EQ(uv.v, 0.0f);
  EXPECT_FLOAT_EQ(uv.uSize, 0.25f);
  EXPECT_FLOAT_EQ(uv.vSize, 0.25f);
}

TEST_F(TerrainAtlasTest, ComputeQuadrantUVTopRight) {
  TileUV tile{0.0f, 0.0f, 0.5f, 0.5f};
  auto uv = computeQuadrantUV(tile, 1);
  EXPECT_FLOAT_EQ(uv.u, 0.25f);
  EXPECT_FLOAT_EQ(uv.v, 0.0f);
  EXPECT_FLOAT_EQ(uv.uSize, 0.25f);
  EXPECT_FLOAT_EQ(uv.vSize, 0.25f);
}

TEST_F(TerrainAtlasTest, ComputeQuadrantUVBottomLeft) {
  TileUV tile{0.0f, 0.0f, 0.5f, 0.5f};
  auto uv = computeQuadrantUV(tile, 2);
  EXPECT_FLOAT_EQ(uv.u, 0.0f);
  EXPECT_FLOAT_EQ(uv.v, 0.25f);
}

TEST_F(TerrainAtlasTest, ComputeQuadrantUVBottomRight) {
  TileUV tile{0.0f, 0.0f, 0.5f, 0.5f};
  auto uv = computeQuadrantUV(tile, 3);
  EXPECT_FLOAT_EQ(uv.u, 0.25f);
  EXPECT_FLOAT_EQ(uv.v, 0.25f);
}

TEST_F(TerrainAtlasTest, ComputeTileUVTableEmptyTextureClasses) {
  auto uvs = computeTileUVTable({}, 2048, 64);
  EXPECT_TRUE(uvs.empty());
}

TEST_F(TerrainAtlasTest, ComputeTileUVTableSingleTileClass) {
  map::TextureClass tc;
  tc.firstTile = 0;
  tc.numTiles = 4;
  tc.width = 64;
  tc.name = "TestTerrain";

  auto uvs = computeTileUVTable({tc}, 2048, 64);
  EXPECT_EQ(uvs.size(), 4u);
}

TEST_F(TerrainAtlasTest, ComputeTileUVTableFirstTileIsAtOrigin) {
  map::TextureClass tc;
  tc.numTiles = 1;
  tc.width = 64;

  auto uvs = computeTileUVTable({tc}, 2048, 64);
  ASSERT_EQ(uvs.size(), 1u);
  EXPECT_FLOAT_EQ(uvs[0].u, 0.0f);
  EXPECT_FLOAT_EQ(uvs[0].v, 0.0f);
}

TEST_F(TerrainAtlasTest, ComputeTileUVTableTileUVsAreNormalized) {
  map::TextureClass tc;
  tc.numTiles = 10;

  auto uvs = computeTileUVTable({tc}, 2048, 64);
  for (const auto &uv : uvs) {
    EXPECT_GE(uv.u, 0.0f);
    EXPECT_LT(uv.u, 1.0f);
    EXPECT_GE(uv.v, 0.0f);
    EXPECT_LT(uv.u + uv.uSize, 1.01f);
    EXPECT_GE(uv.uSize, 0.0f);
    EXPECT_GE(uv.vSize, 0.0f);
  }
}

TEST_F(TerrainAtlasTest, ComputeTileUVTableSecondTileOffset) {
  map::TextureClass tc;
  tc.numTiles = 2;

  auto uvs = computeTileUVTable({tc}, 2048, 64);
  ASSERT_EQ(uvs.size(), 2u);

  float tileWidth = 64.0f / 2048.0f;
  EXPECT_FLOAT_EQ(uvs[0].u, 0.0f);
  EXPECT_NEAR(uvs[1].u, tileWidth, 1e-5f);
}

TEST_F(TerrainAtlasTest, ComputeTileUVTableMultipleClasses) {
  map::TextureClass tc1;
  tc1.numTiles = 3;

  map::TextureClass tc2;
  tc2.numTiles = 5;

  auto uvs = computeTileUVTable({tc1, tc2}, 2048, 64);
  EXPECT_EQ(uvs.size(), 8u);
}

TEST_F(TerrainAtlasTest, DecodeTileNdxUVReturnsTileUV) {
  std::vector<TileUV> tileUVs;
  tileUVs.push_back({0.0f, 0.0f, 0.5f, 0.5f});
  tileUVs.push_back({0.5f, 0.0f, 0.5f, 0.5f});

  int16_t tileNdx = static_cast<int16_t>(1 << 2);
  auto uv = decodeTileNdxUV(tileNdx, tileUVs);
  EXPECT_NEAR(uv.u, 0.5f, 1e-5f);
}

TEST_F(TerrainAtlasTest, DecodeTileNdxUVOutOfRangeReturnsZero) {
  std::vector<TileUV> tileUVs;
  tileUVs.push_back({0.1f, 0.2f, 0.3f, 0.4f});

  int16_t tileNdx = static_cast<int16_t>(100 << 2);
  auto uv = decodeTileNdxUV(tileNdx, tileUVs);
  EXPECT_FLOAT_EQ(uv.u, 0.0f);
  EXPECT_FLOAT_EQ(uv.v, 0.0f);
}

TEST_F(TerrainAtlasTest, BuildProceduralAtlasCreatesValidData) {
  auto atlas = buildProceduralAtlas(4, 256, 64);

  EXPECT_TRUE(atlas.isValid());
  EXPECT_EQ(atlas.atlasWidth, 256);
  EXPECT_GT(atlas.atlasHeight, 0);
  EXPECT_EQ(atlas.tilePixelSize, 64);
  EXPECT_EQ(atlas.tileUVs.size(), 4u);
}

TEST_F(TerrainAtlasTest, BuildProceduralAtlasPixelDataSize) {
  auto atlas = buildProceduralAtlas(2, 128, 64);

  size_t expectedSize = static_cast<size_t>(atlas.atlasWidth * atlas.atlasHeight * 4);
  EXPECT_EQ(atlas.pixels.size(), expectedSize);
}

TEST_F(TerrainAtlasTest, BuildProceduralAtlasUVsAreNormalized) {
  auto atlas = buildProceduralAtlas(8, 512, 64);

  for (const auto &uv : atlas.tileUVs) {
    EXPECT_GE(uv.u, 0.0f);
    EXPECT_LT(uv.u, 1.0f);
    EXPECT_GE(uv.v, 0.0f);
    EXPECT_LT(uv.u + uv.uSize, 1.01f);
  }
}

TEST_F(TerrainAtlasTest, BuildProceduralAtlasAlphaIsOpaque) {
  auto atlas = buildProceduralAtlas(1, 64, 64);

  for (size_t i = 3; i < atlas.pixels.size(); i += 4) {
    EXPECT_EQ(atlas.pixels[i], 255u);
  }
}

TEST_F(TerrainAtlasTest, BuildProceduralAtlasInvalidInputReturnsEmpty) {
  auto atlas = buildProceduralAtlas(0, 256, 64);
  EXPECT_FALSE(atlas.isValid());

  auto atlas2 = buildProceduralAtlas(4, 0, 64);
  EXPECT_FALSE(atlas2.isValid());
}

TEST_F(TerrainAtlasTest, BuildProceduralAtlasTilesHaveDistinctColors) {
  auto atlas = buildProceduralAtlas(3, 192, 64);
  ASSERT_TRUE(atlas.isValid());

  auto getPixel = [&](int32_t x, int32_t y) -> std::array<uint8_t, 3> {
    size_t idx = static_cast<size_t>((y * atlas.atlasWidth + x) * 4);
    return {atlas.pixels[idx], atlas.pixels[idx + 1], atlas.pixels[idx + 2]};
  };

  auto color0 = getPixel(32, 32);
  auto color1 = getPixel(64 + 32, 32);
  auto color2 = getPixel(128 + 32, 32);

  bool distinct01 = (color0 != color1);
  bool distinct02 = (color0 != color2);
  bool distinct12 = (color1 != color2);

  EXPECT_TRUE(distinct01 || distinct02 || distinct12);
}

TEST_F(TerrainAtlasTest, TileUVTableWrapsToNextRow) {
  map::TextureClass tc;
  tc.numTiles = 5;

  auto uvs = computeTileUVTable({tc}, 256, 64);
  ASSERT_EQ(uvs.size(), 5u);

  EXPECT_FLOAT_EQ(uvs[4].u, 0.0f);
  EXPECT_GT(uvs[4].v, 0.0f);
}
