#include <algorithm>
#include <cmath>

#include <gtest/gtest.h>

// Standalone implementation of mipmap level calculation for testing
uint32_t calculateMipLevels(uint32_t width, uint32_t height) {
  return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

TEST(MipmapCalculationTest, SquareTexture256x256) {
  EXPECT_EQ(calculateMipLevels(256, 256), 9);
}

TEST(MipmapCalculationTest, SquareTexture512x512) {
  EXPECT_EQ(calculateMipLevels(512, 512), 10);
}

TEST(MipmapCalculationTest, SquareTexture1024x1024) {
  EXPECT_EQ(calculateMipLevels(1024, 1024), 11);
}

TEST(MipmapCalculationTest, SquareTexture2048x2048) {
  EXPECT_EQ(calculateMipLevels(2048, 2048), 12);
}

TEST(MipmapCalculationTest, SquareTexture1x1) {
  EXPECT_EQ(calculateMipLevels(1, 1), 1);
}

TEST(MipmapCalculationTest, SquareTexture2x2) {
  EXPECT_EQ(calculateMipLevels(2, 2), 2);
}

TEST(MipmapCalculationTest, SquareTexture4x4) {
  EXPECT_EQ(calculateMipLevels(4, 4), 3);
}

TEST(MipmapCalculationTest, RectangularTexture1024x512) {
  EXPECT_EQ(calculateMipLevels(1024, 512), 11);
}

TEST(MipmapCalculationTest, RectangularTexture512x1024) {
  EXPECT_EQ(calculateMipLevels(512, 1024), 11);
}

TEST(MipmapCalculationTest, RectangularTexture256x128) {
  EXPECT_EQ(calculateMipLevels(256, 128), 9);
}

TEST(MipmapCalculationTest, NonPowerOfTwo1000x600) {
  uint32_t levels = calculateMipLevels(1000, 600);
  EXPECT_EQ(levels, 10);
}

TEST(MipmapCalculationTest, NonPowerOfTwo640x480) {
  uint32_t levels = calculateMipLevels(640, 480);
  EXPECT_EQ(levels, 10);
}

TEST(MipmapCalculationTest, SmallNonPowerOfTwo7x5) {
  uint32_t levels = calculateMipLevels(7, 5);
  EXPECT_EQ(levels, 3);
}

TEST(MipmapCalculationTest, VerifyMipChainDimensions) {
  uint32_t width = 1024;
  uint32_t height = 512;
  uint32_t levels = calculateMipLevels(width, height);

  EXPECT_EQ(levels, 11);

  uint32_t mipWidth = width;
  uint32_t mipHeight = height;

  for (uint32_t i = 0; i < levels; i++) {
    if (i == levels - 1) {
      EXPECT_TRUE(mipWidth == 1 || mipHeight == 1)
          << "Final mip level should have at least one dimension at 1";
    }

    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }
}
