// tests/terrain/test_terrain_minimap.cpp
// Unit tests for MinimapGenerator (Phase 6.4).
//
// No Vulkan dependency – minimap generation is purely CPU-side.

#include "lib/formats/map/types.hpp"
#include "render/terrain/terrain_minimap.hpp"

#include <gtest/gtest.h>

using namespace w3d::terrain;
using namespace map;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static HeightMap makeFlat(int32_t w, int32_t h, uint8_t heightValue = 128) {
  HeightMap hm;
  hm.width = w;
  hm.height = h;
  hm.borderSize = 0;
  hm.data.assign(static_cast<size_t>(w * h), heightValue);
  return hm;
}

static HeightMap makeGradient(int32_t w, int32_t h) {
  HeightMap hm;
  hm.width = w;
  hm.height = h;
  hm.borderSize = 0;
  hm.data.resize(static_cast<size_t>(w * h));
  for (int32_t y = 0; y < h; ++y) {
    for (int32_t x = 0; x < w; ++x) {
      // height increases left to right, bottom to top
      hm.data[static_cast<size_t>(y * w + x)] = static_cast<uint8_t>((x + y) * 255 / (w + h - 2));
    }
  }
  return hm;
}

// ---------------------------------------------------------------------------
// Empty / invalid heightmap
// ---------------------------------------------------------------------------

TEST(MinimapGenerator, EmptyHeightmapProducesEmptyImage) {
  HeightMap hm;
  auto img = MinimapGenerator::generate(hm);
  EXPECT_FALSE(img.isValid());
}

TEST(MinimapGenerator, ZeroWidthProducesEmptyImage) {
  HeightMap hm;
  hm.width = 0;
  hm.height = 10;
  hm.data.assign(10, 128);
  auto img = MinimapGenerator::generate(hm);
  EXPECT_FALSE(img.isValid());
}

TEST(MinimapGenerator, ZeroHeightProducesEmptyImage) {
  HeightMap hm;
  hm.width = 10;
  hm.height = 0;
  auto img = MinimapGenerator::generate(hm);
  EXPECT_FALSE(img.isValid());
}

// ---------------------------------------------------------------------------
// Image dimensions
// ---------------------------------------------------------------------------

TEST(MinimapGenerator, OutputDimensionsMatchHeightmap) {
  auto hm = makeFlat(64, 64);
  auto img = MinimapGenerator::generate(hm);
  ASSERT_TRUE(img.isValid());
  EXPECT_EQ(img.width, 64u);
  EXPECT_EQ(img.height, 64u);
}

TEST(MinimapGenerator, NonSquareDimensionsPreserved) {
  auto hm = makeFlat(128, 64);
  auto img = MinimapGenerator::generate(hm);
  ASSERT_TRUE(img.isValid());
  EXPECT_EQ(img.width, 128u);
  EXPECT_EQ(img.height, 64u);
}

TEST(MinimapGenerator, PixelCountIsWidthTimesHeightTimes4) {
  auto hm = makeFlat(32, 48);
  auto img = MinimapGenerator::generate(hm);
  ASSERT_TRUE(img.isValid());
  EXPECT_EQ(img.pixels.size(), 32u * 48u * 4u); // RGBA
}

// ---------------------------------------------------------------------------
// Flat terrain (uniform height)
// ---------------------------------------------------------------------------

TEST(MinimapGenerator, FlatTerrainProducesUniformColor) {
  auto hm = makeFlat(8, 8, 128); // mid height
  auto img = MinimapGenerator::generate(hm);
  ASSERT_TRUE(img.isValid());

  // All pixels should have the same color
  uint8_t r0 = img.pixels[0];
  uint8_t g0 = img.pixels[1];
  uint8_t b0 = img.pixels[2];
  for (size_t i = 0; i + 3 < img.pixels.size(); i += 4) {
    EXPECT_EQ(img.pixels[i + 0], r0) << "R differs at pixel " << i / 4;
    EXPECT_EQ(img.pixels[i + 1], g0) << "G differs at pixel " << i / 4;
    EXPECT_EQ(img.pixels[i + 2], b0) << "B differs at pixel " << i / 4;
  }
}

TEST(MinimapGenerator, AllPixelsFullyOpaque) {
  auto hm = makeFlat(16, 16);
  auto img = MinimapGenerator::generate(hm);
  ASSERT_TRUE(img.isValid());
  for (size_t i = 3; i < img.pixels.size(); i += 4) {
    EXPECT_EQ(img.pixels[i], 255u) << "Alpha not 255 at pixel " << i / 4;
  }
}

// ---------------------------------------------------------------------------
// Gradient terrain (varying height)
// ---------------------------------------------------------------------------

TEST(MinimapGenerator, GradientTerrainProducesVaryingColors) {
  auto hm = makeGradient(16, 16);
  auto img = MinimapGenerator::generate(hm);
  ASSERT_TRUE(img.isValid());

  // The darkest pixel (bottom-left, height ~0) should be darker than the
  // brightest pixel (top-right, height ~255).
  auto brightness = [&](size_t pixelIndex) {
    return static_cast<int>(img.pixels[pixelIndex * 4 + 0]) +
           static_cast<int>(img.pixels[pixelIndex * 4 + 1]) +
           static_cast<int>(img.pixels[pixelIndex * 4 + 2]);
  };

  size_t topRight = static_cast<size_t>((15) * 16 + 15); // y=15, x=15 → highest
  size_t bottomLeft = 0;                                 // y=0,  x=0  → lowest

  EXPECT_GT(brightness(topRight), brightness(bottomLeft));
}

TEST(MinimapGenerator, ZeroHeightPixelIsDarkest) {
  auto hm = makeFlat(4, 4, 0); // all-zero height
  auto img = MinimapGenerator::generate(hm);
  ASSERT_TRUE(img.isValid());

  auto hm2 = makeFlat(4, 4, 255); // all-max height
  auto img2 = MinimapGenerator::generate(hm2);
  ASSERT_TRUE(img2.isValid());

  int brightness0 = static_cast<int>(img.pixels[0]) + static_cast<int>(img.pixels[1]) +
                    static_cast<int>(img.pixels[2]);
  int brightness255 = static_cast<int>(img2.pixels[0]) + static_cast<int>(img2.pixels[1]) +
                      static_cast<int>(img2.pixels[2]);

  EXPECT_LT(brightness0, brightness255);
}

// ---------------------------------------------------------------------------
// Large map size
// ---------------------------------------------------------------------------

TEST(MinimapGenerator, LargeMapGeneratesWithoutError) {
  auto hm = makeGradient(256, 256);
  auto img = MinimapGenerator::generate(hm);
  ASSERT_TRUE(img.isValid());
  EXPECT_EQ(img.width, 256u);
  EXPECT_EQ(img.height, 256u);
  EXPECT_EQ(img.pixels.size(), 256u * 256u * 4u);
}

// ---------------------------------------------------------------------------
// Downscaled variant
// ---------------------------------------------------------------------------

TEST(MinimapGenerator, GenerateScaledReducesDimensions) {
  auto hm = makeGradient(256, 256);
  auto img = MinimapGenerator::generateScaled(hm, 128, 128);
  ASSERT_TRUE(img.isValid());
  EXPECT_EQ(img.width, 128u);
  EXPECT_EQ(img.height, 128u);
  EXPECT_EQ(img.pixels.size(), 128u * 128u * 4u);
}

TEST(MinimapGenerator, GenerateScaledSinglePixel) {
  auto hm = makeFlat(32, 32, 200);
  auto img = MinimapGenerator::generateScaled(hm, 1, 1);
  ASSERT_TRUE(img.isValid());
  EXPECT_EQ(img.width, 1u);
  EXPECT_EQ(img.height, 1u);
  EXPECT_EQ(img.pixels.size(), 4u);
}

TEST(MinimapGenerator, GenerateScaledLargerThanSourceClampsToSource) {
  // Requesting a larger minimap than source should clamp to source dimensions.
  auto hm = makeFlat(32, 32);
  auto img = MinimapGenerator::generateScaled(hm, 256, 256);
  ASSERT_TRUE(img.isValid());
  EXPECT_LE(img.width, 256u);
  EXPECT_LE(img.height, 256u);
}
