#include "render/terrain/terrain_blend_data.hpp"

#include <gtest/gtest.h>

using namespace w3d::terrain;
using namespace w3d;

class TerrainBlendDataTest : public ::testing::Test {
protected:
  map::BlendTileData makeSimpleBlendTileData(int32_t cellCount) {
    map::BlendTileData btd;
    btd.dataSize = cellCount;
    btd.tileNdxes.resize(cellCount, 0);
    btd.blendTileNdxes.resize(cellCount, 0);
    btd.extraBlendTileNdxes.resize(cellCount, 0);
    btd.cliffInfoNdxes.resize(cellCount, 0);
    btd.cellCliffState.resize(cellCount, 0);
    return btd;
  }
};

TEST_F(TerrainBlendDataTest, EmptyBlendTileDataProducesEmptyBuffer) {
  map::BlendTileData btd;
  auto cells = buildCellBlendBuffer(btd);
  EXPECT_TRUE(cells.empty());
}

TEST_F(TerrainBlendDataTest, BufferSizeMatchesCellCount) {
  auto btd = makeSimpleBlendTileData(16);
  auto cells = buildCellBlendBuffer(btd);
  EXPECT_EQ(cells.size(), 16u);
}

TEST_F(TerrainBlendDataTest, NoBlendsProducesZeroBlendIndices) {
  auto btd = makeSimpleBlendTileData(4);
  auto cells = buildCellBlendBuffer(btd);
  for (const auto &cell : cells) {
    EXPECT_EQ(cell.blendTileIndex, 0u);
    EXPECT_EQ(cell.extraTileIndex, 0u);
  }
}

TEST_F(TerrainBlendDataTest, BaseTileIndexDecodedCorrectly) {
  auto btd = makeSimpleBlendTileData(1);
  btd.tileNdxes[0] = static_cast<int16_t>(3 << 2);
  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].baseTileIndex, 3u);
}

TEST_F(TerrainBlendDataTest, BaseQuadrantDecodedCorrectly) {
  auto btd = makeSimpleBlendTileData(4);
  for (int16_t q = 0; q < 4; ++q) {
    btd.tileNdxes[q] = static_cast<int16_t>((2 << 2) | q);
  }
  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 4u);
  for (uint16_t q = 0; q < 4; ++q) {
    EXPECT_EQ(cells[q].baseQuadrant, q);
  }
}

TEST_F(TerrainBlendDataTest, BlendTileIndexResolvedFromBlendTileInfo) {
  auto btd = makeSimpleBlendTileData(1);

  map::BlendTileInfo info;
  info.blendNdx = static_cast<int32_t>(5 << 2);
  info.horiz = 1;
  info.inverted = 0;
  btd.blendTileInfos.push_back(info);

  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendTileIndex, 5u);
}

TEST_F(TerrainBlendDataTest, ExtraBlendTileIndexResolvedCorrectly) {
  auto btd = makeSimpleBlendTileData(1);

  map::BlendTileInfo blendInfo;
  blendInfo.blendNdx = static_cast<int32_t>(2 << 2);
  blendInfo.horiz = 1;
  btd.blendTileInfos.push_back(blendInfo);

  map::BlendTileInfo extraInfo;
  extraInfo.blendNdx = static_cast<int32_t>(7 << 2);
  extraInfo.vert = 1;
  btd.blendTileInfos.push_back(extraInfo);

  btd.blendTileNdxes[0] = 1;
  btd.extraBlendTileNdxes[0] = 2;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendTileIndex, 2u);
  EXPECT_EQ(cells[0].extraTileIndex, 7u);
}

TEST_F(TerrainBlendDataTest, BlendDirectionEncodedForHoriz) {
  auto btd = makeSimpleBlendTileData(1);

  map::BlendTileInfo info;
  info.blendNdx = 0;
  info.horiz = 1;
  info.inverted = 0;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::Horizontal));
}

TEST_F(TerrainBlendDataTest, BlendDirectionEncodedForVertInv) {
  auto btd = makeSimpleBlendTileData(1);

  map::BlendTileInfo info;
  info.blendNdx = 0;
  info.vert = 1;
  info.inverted = map::INVERTED_MASK;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::VerticalInv));
}

TEST_F(TerrainBlendDataTest, CliffFlagSetWhenCliffInfoNdxNonZero) {
  auto btd = makeSimpleBlendTileData(2);
  btd.cliffInfoNdxes[0] = 0;
  btd.cliffInfoNdxes[1] = 1;

  map::CliffInfo cliffInfo;
  cliffInfo.tileIndex = 10;
  btd.cliffInfos.push_back(cliffInfo);

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 2u);
  EXPECT_EQ(cells[0].flags & CellBlendFlags::IsCliff, 0u);
  EXPECT_NE(cells[1].flags & CellBlendFlags::IsCliff, 0u);
}

TEST_F(TerrainBlendDataTest, BlendQuadrantDecodedCorrectly) {
  auto btd = makeSimpleBlendTileData(1);

  map::BlendTileInfo info;
  info.blendNdx = static_cast<int32_t>((4 << 2) | 3);
  info.rightDiagonal = 1;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendTileIndex, 4u);
  EXPECT_EQ(cells[0].blendQuadrant, 3u);
}

TEST_F(TerrainBlendDataTest, NoBlendsHasNoneDirection) {
  auto btd = makeSimpleBlendTileData(1);
  btd.blendTileNdxes[0] = 0;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::None));
}

TEST_F(TerrainBlendDataTest, LargeMapBufferSizeCorrect) {
  constexpr int32_t LARGE = 1024 * 1024;
  map::BlendTileData btd;
  btd.dataSize = LARGE;
  btd.tileNdxes.resize(LARGE, 0);
  btd.blendTileNdxes.resize(LARGE, 0);
  btd.extraBlendTileNdxes.resize(LARGE, 0);
  btd.cliffInfoNdxes.resize(LARGE, 0);
  btd.cellCliffState.resize(LARGE, 0);

  auto cells = buildCellBlendBuffer(btd);
  EXPECT_EQ(cells.size(), static_cast<size_t>(LARGE));
}

TEST_F(TerrainBlendDataTest, ZeroBlendTileNdxMeansNoBlend) {
  auto btd = makeSimpleBlendTileData(1);
  btd.blendTileNdxes[0] = 0;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendTileIndex, 0u);
  EXPECT_EQ(cells[0].blendQuadrant, 0u);
}

TEST_F(TerrainBlendDataTest, CustomEdgeDirectionEncodedWhenCustomBlendEdgeClassSet) {
  auto btd = makeSimpleBlendTileData(1);

  map::TextureClass edgeClass;
  edgeClass.name = "EdgeTex";
  edgeClass.firstTile = 5;
  edgeClass.numTiles = 1;
  edgeClass.width = 1;
  btd.edgeTextureClasses.push_back(edgeClass);

  map::BlendTileInfo info;
  info.blendNdx = static_cast<int32_t>(2 << 2);
  info.horiz = 1;
  info.customBlendEdgeClass = 0;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  constexpr uint32_t edgeLayerBase = 10;
  auto cells = buildCellBlendBuffer(btd, edgeLayerBase);
  ASSERT_EQ(cells.size(), 1u);

  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::CustomEdge));
  EXPECT_EQ(cells[0].blendQuadrant, edgeLayerBase + 5u);
}

TEST_F(TerrainBlendDataTest, CustomEdgeLayerIndexComputedFromEdgeClassFirstTile) {
  auto btd = makeSimpleBlendTileData(1);

  map::TextureClass edgeClass0;
  edgeClass0.name = "EdgeA";
  edgeClass0.firstTile = 0;
  edgeClass0.numTiles = 2;
  edgeClass0.width = 1;
  btd.edgeTextureClasses.push_back(edgeClass0);

  map::TextureClass edgeClass1;
  edgeClass1.name = "EdgeB";
  edgeClass1.firstTile = 2;
  edgeClass1.numTiles = 1;
  edgeClass1.width = 1;
  btd.edgeTextureClasses.push_back(edgeClass1);

  map::BlendTileInfo info;
  info.blendNdx = 0;
  info.horiz = 1;
  info.customBlendEdgeClass = 1;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  constexpr uint32_t edgeLayerBase = 20;
  auto cells = buildCellBlendBuffer(btd, edgeLayerBase);
  ASSERT_EQ(cells.size(), 1u);

  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::CustomEdge));
  EXPECT_EQ(cells[0].blendQuadrant, edgeLayerBase + 2u);
}

TEST_F(TerrainBlendDataTest, CustomEdgeNotSetWhenEdgeClassIndexOutOfBounds) {
  auto btd = makeSimpleBlendTileData(1);

  map::BlendTileInfo info;
  info.blendNdx = static_cast<int32_t>(3 << 2);
  info.horiz = 1;
  info.customBlendEdgeClass = 99;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd, 0);
  ASSERT_EQ(cells.size(), 1u);

  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::Horizontal));
}

TEST_F(TerrainBlendDataTest, NoCustomEdgeWhenEdgeClassIsMinusOne) {
  auto btd = makeSimpleBlendTileData(1);

  map::BlendTileInfo info;
  info.blendNdx = static_cast<int32_t>(2 << 2);
  info.vert = 1;
  info.customBlendEdgeClass = -1;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd, 0);
  ASSERT_EQ(cells.size(), 1u);

  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::Vertical));
}

TEST_F(TerrainBlendDataTest, ExtraBlendAlsoSupportsCustomEdge) {
  auto btd = makeSimpleBlendTileData(1);

  map::TextureClass edgeClass;
  edgeClass.name = "EdgeTex";
  edgeClass.firstTile = 3;
  edgeClass.numTiles = 1;
  edgeClass.width = 1;
  btd.edgeTextureClasses.push_back(edgeClass);

  map::BlendTileInfo blendInfo;
  blendInfo.blendNdx = 0;
  blendInfo.horiz = 1;
  blendInfo.customBlendEdgeClass = -1;
  btd.blendTileInfos.push_back(blendInfo);

  map::BlendTileInfo extraInfo;
  extraInfo.blendNdx = 0;
  extraInfo.vert = 1;
  extraInfo.customBlendEdgeClass = 0;
  btd.blendTileInfos.push_back(extraInfo);

  btd.blendTileNdxes[0] = 1;
  btd.extraBlendTileNdxes[0] = 2;

  constexpr uint32_t edgeLayerBase = 15;
  auto cells = buildCellBlendBuffer(btd, edgeLayerBase);
  ASSERT_EQ(cells.size(), 1u);

  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::Horizontal));
  EXPECT_EQ(cells[0].extraDirection, static_cast<uint8_t>(BlendDirectionEncoding::CustomEdge));
  EXPECT_EQ(cells[0].extraQuadrant, edgeLayerBase + 3u);
}

TEST_F(TerrainBlendDataTest, DefaultEdgeTileLayerBaseIsZero) {
  auto btd = makeSimpleBlendTileData(1);

  map::TextureClass edgeClass;
  edgeClass.name = "EdgeTex";
  edgeClass.firstTile = 7;
  edgeClass.numTiles = 1;
  edgeClass.width = 1;
  btd.edgeTextureClasses.push_back(edgeClass);

  map::BlendTileInfo info;
  info.blendNdx = 0;
  info.horiz = 1;
  info.customBlendEdgeClass = 0;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::CustomEdge));
  EXPECT_EQ(cells[0].blendQuadrant, 0u + 7u);
}

TEST_F(TerrainBlendDataTest, AllDirectionsEncodedDistinctly) {
  auto btd = makeSimpleBlendTileData(12);

  map::BlendTileInfo infos[12];

  // Horizontal / HorizontalInv
  infos[0].horiz = 1;
  infos[0].inverted = 0;
  infos[1].horiz = 1;
  infos[1].inverted = map::INVERTED_MASK;
  // Vertical / VerticalInv
  infos[2].vert = 1;
  infos[2].inverted = 0;
  infos[3].vert = 1;
  infos[3].inverted = map::INVERTED_MASK;
  // DiagonalRight / DiagonalRightInv  (rightDiagonal=1, longDiagonal=0)
  infos[4].rightDiagonal = 1;
  infos[4].inverted = 0;
  infos[5].rightDiagonal = 1;
  infos[5].inverted = map::INVERTED_MASK;
  // DiagonalLeft / DiagonalLeftInv  (leftDiagonal=1, longDiagonal=0)
  infos[6].leftDiagonal = 1;
  infos[6].inverted = 0;
  infos[7].leftDiagonal = 1;
  infos[7].inverted = map::INVERTED_MASK;
  // LongDiagonalRight / LongDiagonalRightInv  (rightDiagonal=1, longDiagonal=1)
  infos[8].rightDiagonal = 1;
  infos[8].longDiagonal = 1;
  infos[8].inverted = 0;
  infos[9].rightDiagonal = 1;
  infos[9].longDiagonal = 1;
  infos[9].inverted = map::INVERTED_MASK;
  // LongDiagonalLeft / LongDiagonalLeftInv  (leftDiagonal=1, longDiagonal=1)
  infos[10].leftDiagonal = 1;
  infos[10].longDiagonal = 1;
  infos[10].inverted = 0;
  infos[11].leftDiagonal = 1;
  infos[11].longDiagonal = 1;
  infos[11].inverted = map::INVERTED_MASK;

  for (int32_t i = 0; i < 12; ++i) {
    infos[i].blendNdx = 0;
    btd.blendTileInfos.push_back(infos[i]);
    btd.blendTileNdxes[i] = static_cast<int16_t>(i + 1);
  }

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 12u);

  uint8_t dirs[12];
  for (int32_t i = 0; i < 12; ++i) {
    dirs[i] = cells[i].blendDirection;
  }

  for (int32_t i = 0; i < 12; ++i) {
    for (int32_t j = i + 1; j < 12; ++j) {
      EXPECT_NE(dirs[i], dirs[j]) << "Directions " << i << " and " << j << " are the same";
    }
  }
}

// --- encodeBlendDirection edge cases ---

TEST_F(TerrainBlendDataTest, LongDiagonalRightEncoded) {
  auto btd = makeSimpleBlendTileData(1);
  map::BlendTileInfo info;
  info.rightDiagonal = 1;
  info.longDiagonal = 1;
  info.inverted = 0;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection,
            static_cast<uint8_t>(BlendDirectionEncoding::LongDiagonalRight));
}

TEST_F(TerrainBlendDataTest, LongDiagonalRightInvEncoded) {
  auto btd = makeSimpleBlendTileData(1);
  map::BlendTileInfo info;
  info.rightDiagonal = 1;
  info.longDiagonal = 1;
  info.inverted = map::INVERTED_MASK;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection,
            static_cast<uint8_t>(BlendDirectionEncoding::LongDiagonalRightInv));
}

TEST_F(TerrainBlendDataTest, LongDiagonalLeftEncoded) {
  auto btd = makeSimpleBlendTileData(1);
  map::BlendTileInfo info;
  info.leftDiagonal = 1;
  info.longDiagonal = 1;
  info.inverted = 0;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection,
            static_cast<uint8_t>(BlendDirectionEncoding::LongDiagonalLeft));
}

TEST_F(TerrainBlendDataTest, LongDiagonalLeftInvEncoded) {
  auto btd = makeSimpleBlendTileData(1);
  map::BlendTileInfo info;
  info.leftDiagonal = 1;
  info.longDiagonal = 1;
  info.inverted = map::INVERTED_MASK;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection,
            static_cast<uint8_t>(BlendDirectionEncoding::LongDiagonalLeftInv));
}

// longDiagonal alone (no rightDiagonal/leftDiagonal) is meaningless in the original engine
// and must fall through to None, matching getRGBAlphaDataForWidth() behaviour.
TEST_F(TerrainBlendDataTest, LongDiagonalAloneProducesNone) {
  auto btd = makeSimpleBlendTileData(1);
  map::BlendTileInfo info;
  info.longDiagonal = 1;
  info.inverted = 0;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::None));
}

// FLIPPED_MASK (bit 1 of inverted) is a 3-way blend marker; it is not read by
// encodeBlendDirection, which only tests INVERTED_MASK (bit 0).  Setting FLIPPED_MASK
// alone must therefore produce the non-inverted encoding.
TEST_F(TerrainBlendDataTest, FlippedMaskAloneNotTreatedAsInverted) {
  auto btd = makeSimpleBlendTileData(1);
  map::BlendTileInfo info;
  info.horiz = 1;
  info.inverted = map::FLIPPED_MASK;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::Horizontal));
}

// FLIPPED_MASK | INVERTED_MASK: the INVERTED_MASK bit is set, so encoding must be inverted.
TEST_F(TerrainBlendDataTest, FlippedMaskPlusInvertedMaskTreatedAsInverted) {
  auto btd = makeSimpleBlendTileData(1);
  map::BlendTileInfo info;
  info.horiz = 1;
  info.inverted = static_cast<int8_t>(map::INVERTED_MASK | map::FLIPPED_MASK);
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::HorizontalInv));
}

// longDiagonal set alongside horiz must not change the direction: the original engine
// stores longDiagonal in the struct for horiz cells but never reads it in that path.
TEST_F(TerrainBlendDataTest, LongDiagonalWithHorizIgnored) {
  auto btd = makeSimpleBlendTileData(1);
  map::BlendTileInfo info;
  info.horiz = 1;
  info.longDiagonal = 1;
  info.inverted = 0;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::Horizontal));
}

// longDiagonal set alongside vert must also be ignored.
TEST_F(TerrainBlendDataTest, LongDiagonalWithVertIgnored) {
  auto btd = makeSimpleBlendTileData(1);
  map::BlendTileInfo info;
  info.vert = 1;
  info.longDiagonal = 1;
  info.inverted = map::INVERTED_MASK;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::VerticalInv));
}

// horiz takes priority over vert when both are set (mirrors if/else if chain in
// getRGBAlphaDataForWidth).
TEST_F(TerrainBlendDataTest, HorizTakesPriorityOverVert) {
  auto btd = makeSimpleBlendTileData(1);
  map::BlendTileInfo info;
  info.horiz = 1;
  info.vert = 1;
  info.inverted = 0;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::Horizontal));
}

// rightDiagonal takes priority over leftDiagonal when both are set (mirrors if/else if chain).
TEST_F(TerrainBlendDataTest, RightDiagonalTakesPriorityOverLeft) {
  auto btd = makeSimpleBlendTileData(1);
  map::BlendTileInfo info;
  info.rightDiagonal = 1;
  info.leftDiagonal = 1;
  info.longDiagonal = 0;
  info.inverted = 0;
  btd.blendTileInfos.push_back(info);
  btd.blendTileNdxes[0] = 1;

  auto cells = buildCellBlendBuffer(btd);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].blendDirection, static_cast<uint8_t>(BlendDirectionEncoding::DiagonalRight));
}
