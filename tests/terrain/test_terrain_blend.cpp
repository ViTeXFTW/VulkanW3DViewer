#include "render/terrain/terrain_blend.hpp"

#include <gtest/gtest.h>

using namespace w3d::terrain;
using namespace w3d;

class TerrainBlendTest : public ::testing::Test {};

TEST_F(TerrainBlendTest, GenerateBlendPatternHasCorrectSize) {
  auto pattern = generateBlendPattern(BlendDirection::Horizontal);
  EXPECT_EQ(pattern.size, BLEND_PATTERN_SIZE);
  EXPECT_EQ(static_cast<int32_t>(pattern.alpha.size()), BLEND_PATTERN_SIZE * BLEND_PATTERN_SIZE);
}

TEST_F(TerrainBlendTest, GenerateBlendPatternAlphaValuesInRange) {
  for (int32_t i = 0; i < NUM_BLEND_PATTERNS; ++i) {
    auto pattern = generateBlendPattern(static_cast<BlendDirection>(i));
    for (uint8_t val : pattern.alpha) {
      EXPECT_GE(val, 0);
      EXPECT_LE(val, 255);
    }
  }
}

TEST_F(TerrainBlendTest, HorizontalPatternIncreasesLeftToRight) {
  auto pattern = generateBlendPattern(BlendDirection::Horizontal);

  int leftCol = 0;
  int rightCol = BLEND_PATTERN_SIZE - 1;
  int midRow = BLEND_PATTERN_SIZE / 2;

  uint8_t leftVal = pattern.alpha[static_cast<size_t>(midRow * BLEND_PATTERN_SIZE + leftCol)];
  uint8_t rightVal = pattern.alpha[static_cast<size_t>(midRow * BLEND_PATTERN_SIZE + rightCol)];

  EXPECT_LT(leftVal, rightVal);
}

TEST_F(TerrainBlendTest, HorizontalInvPatternDecreasesLeftToRight) {
  auto pattern = generateBlendPattern(BlendDirection::HorizontalInv);

  int midRow = BLEND_PATTERN_SIZE / 2;
  uint8_t leftVal = pattern.alpha[static_cast<size_t>(midRow * BLEND_PATTERN_SIZE + 0)];
  uint8_t rightVal =
      pattern.alpha[static_cast<size_t>(midRow * BLEND_PATTERN_SIZE + BLEND_PATTERN_SIZE - 1)];

  EXPECT_GT(leftVal, rightVal);
}

TEST_F(TerrainBlendTest, VerticalPatternIncreasesTopToBottom) {
  auto pattern = generateBlendPattern(BlendDirection::Vertical);

  int midCol = BLEND_PATTERN_SIZE / 2;
  uint8_t topVal = pattern.alpha[static_cast<size_t>(0 * BLEND_PATTERN_SIZE + midCol)];
  uint8_t bottomVal =
      pattern.alpha[static_cast<size_t>((BLEND_PATTERN_SIZE - 1) * BLEND_PATTERN_SIZE + midCol)];

  EXPECT_LT(topVal, bottomVal);
}

TEST_F(TerrainBlendTest, VerticalInvPatternDecreasesTopToBottom) {
  auto pattern = generateBlendPattern(BlendDirection::VerticalInv);

  int midCol = BLEND_PATTERN_SIZE / 2;
  uint8_t topVal = pattern.alpha[static_cast<size_t>(0 * BLEND_PATTERN_SIZE + midCol)];
  uint8_t bottomVal =
      pattern.alpha[static_cast<size_t>((BLEND_PATTERN_SIZE - 1) * BLEND_PATTERN_SIZE + midCol)];

  EXPECT_GT(topVal, bottomVal);
}

TEST_F(TerrainBlendTest, HorizontalAndInvAreComplementary) {
  auto horiz = generateBlendPattern(BlendDirection::Horizontal);
  auto horizInv = generateBlendPattern(BlendDirection::HorizontalInv);

  int midRow = BLEND_PATTERN_SIZE / 2;
  int midCol = BLEND_PATTERN_SIZE / 2;
  size_t idx = static_cast<size_t>(midRow * BLEND_PATTERN_SIZE + midCol);

  int sum = static_cast<int>(horiz.alpha[idx]) + static_cast<int>(horizInv.alpha[idx]);
  EXPECT_NEAR(sum, 255, 2);
}

TEST_F(TerrainBlendTest, VerticalAndInvAreComplementary) {
  auto vert = generateBlendPattern(BlendDirection::Vertical);
  auto vertInv = generateBlendPattern(BlendDirection::VerticalInv);

  int midRow = BLEND_PATTERN_SIZE / 2;
  int midCol = BLEND_PATTERN_SIZE / 2;
  size_t idx = static_cast<size_t>(midRow * BLEND_PATTERN_SIZE + midCol);

  int sum = static_cast<int>(vert.alpha[idx]) + static_cast<int>(vertInv.alpha[idx]);
  EXPECT_NEAR(sum, 255, 2);
}

TEST_F(TerrainBlendTest, GenerateAllBlendPatternsReturnsCorrectCount) {
  auto patterns = generateAllBlendPatterns();
  EXPECT_EQ(static_cast<int32_t>(patterns.size()), NUM_BLEND_PATTERNS);
}

TEST_F(TerrainBlendTest, AllPatternsHaveCorrectDimensions) {
  auto patterns = generateAllBlendPatterns();
  for (const auto &pattern : patterns) {
    EXPECT_EQ(pattern.size, BLEND_PATTERN_SIZE);
    EXPECT_EQ(static_cast<int32_t>(pattern.alpha.size()), BLEND_PATTERN_SIZE * BLEND_PATTERN_SIZE);
  }
}

TEST_F(TerrainBlendTest, BlendDirectionFromInfoHoriz) {
  map::BlendTileInfo info;
  info.horiz = 1;
  info.inverted = 0;

  auto dir = blendDirectionFromInfo(info);
  EXPECT_EQ(dir, BlendDirection::Horizontal);
}

TEST_F(TerrainBlendTest, BlendDirectionFromInfoHorizInv) {
  map::BlendTileInfo info;
  info.horiz = 1;
  info.inverted = map::INVERTED_MASK;

  auto dir = blendDirectionFromInfo(info);
  EXPECT_EQ(dir, BlendDirection::HorizontalInv);
}

TEST_F(TerrainBlendTest, BlendDirectionFromInfoVert) {
  map::BlendTileInfo info;
  info.vert = 1;
  info.inverted = 0;

  auto dir = blendDirectionFromInfo(info);
  EXPECT_EQ(dir, BlendDirection::Vertical);
}

TEST_F(TerrainBlendTest, BlendDirectionFromInfoVertInv) {
  map::BlendTileInfo info;
  info.vert = 1;
  info.inverted = map::INVERTED_MASK;

  auto dir = blendDirectionFromInfo(info);
  EXPECT_EQ(dir, BlendDirection::VerticalInv);
}

TEST_F(TerrainBlendTest, BlendDirectionFromInfoDiagonalRight) {
  map::BlendTileInfo info;
  info.rightDiagonal = 1;
  info.inverted = 0;

  auto dir = blendDirectionFromInfo(info);
  EXPECT_EQ(dir, BlendDirection::DiagonalRight);
}

TEST_F(TerrainBlendTest, BlendDirectionFromInfoLongDiagonal) {
  map::BlendTileInfo info;
  info.leftDiagonal = 1;
  info.longDiagonal = 1;
  info.inverted = 0;

  auto dir = blendDirectionFromInfo(info);
  EXPECT_EQ(dir, BlendDirection::LongDiagonalLeft);
}

TEST_F(TerrainBlendTest, CellHasBlendReturnsTrueForHoriz) {
  map::BlendTileInfo info;
  info.horiz = 1;
  EXPECT_TRUE(cellHasBlend(info));
}

TEST_F(TerrainBlendTest, CellHasBlendReturnsFalseForEmpty) {
  map::BlendTileInfo info{};
  EXPECT_FALSE(cellHasBlend(info));
}

TEST_F(TerrainBlendTest, CellHasBlendReturnsTrueForVert) {
  map::BlendTileInfo info;
  info.vert = 1;
  EXPECT_TRUE(cellHasBlend(info));
}

TEST_F(TerrainBlendTest, CellHasBlendReturnsTrueForDiagonal) {
  map::BlendTileInfo info;
  info.leftDiagonal = 1;
  EXPECT_TRUE(cellHasBlend(info));
}

TEST_F(TerrainBlendTest, DiagonalRightPatternHasCorrectGradient) {
  auto pattern = generateBlendPattern(BlendDirection::DiagonalRight);

  uint8_t topLeft = pattern.alpha[0];
  uint8_t bottomRight = pattern.alpha[static_cast<size_t>(
      (BLEND_PATTERN_SIZE - 1) * BLEND_PATTERN_SIZE + BLEND_PATTERN_SIZE - 1)];

  EXPECT_LT(topLeft, bottomRight);
}

TEST_F(TerrainBlendTest, PatternsAreNotAllSame) {
  auto horiz = generateBlendPattern(BlendDirection::Horizontal);
  auto vert = generateBlendPattern(BlendDirection::Vertical);

  bool different = false;
  for (size_t i = 0; i < horiz.alpha.size(); ++i) {
    if (horiz.alpha[i] != vert.alpha[i]) {
      different = true;
      break;
    }
  }
  EXPECT_TRUE(different);
}
