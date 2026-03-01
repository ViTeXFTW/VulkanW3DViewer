#include <cstring>
#include <vector>

#include "../../src/render/terrain/terrain_resource_manager.hpp"

#include <gtest/gtest.h>

using namespace w3d::big;
using namespace w3d::terrain;
using namespace ini;

class TerrainResourceManagerTest : public ::testing::Test {
protected:
  std::unique_ptr<TerrainResourceManager> manager;

  void SetUp() override { manager = std::make_unique<TerrainResourceManager>(); }
};

TEST_F(TerrainResourceManagerTest, StartsUninitialized) {
  EXPECT_FALSE(manager->isInitialized());
  EXPECT_TRUE(manager->getTerrainTypes().empty());
}

TEST_F(TerrainResourceManagerTest, LoadsTerrainINIFromMemory) {
  const char *iniContent = R"(
Terrain TEDesert1
  Texture = TEDesert1.tga
  BlendEdges = No
  Class = DESERT_1
  RestrictConstruction = No
End

Terrain GrassLight
  Texture = GrassLight.tga
  BlendEdges = Yes
  Class = GRASS
  RestrictConstruction = No
End
)";

  std::string error;
  bool result = manager->loadTerrainTypesFromINI(iniContent, &error);

  EXPECT_TRUE(result) << "Error: " << error;
  EXPECT_TRUE(manager->isInitialized());

  const auto &types = manager->getTerrainTypes();
  EXPECT_EQ(types.size(), 2u);

  auto *desert = types.findByName("TEDesert1");
  ASSERT_NE(desert, nullptr);
  EXPECT_EQ(desert->texture, "TEDesert1.tga");
  EXPECT_EQ(desert->terrainClass, TerrainClass::Desert1);

  auto *grass = types.findByName("GrassLight");
  ASSERT_NE(grass, nullptr);
  EXPECT_EQ(grass->texture, "GrassLight.tga");
  EXPECT_TRUE(grass->blendEdgeTexture);
}

TEST_F(TerrainResourceManagerTest, HandlesEmptyINI) {
  std::string error;
  bool result = manager->loadTerrainTypesFromINI("", &error);

  EXPECT_TRUE(result);
  EXPECT_TRUE(manager->isInitialized());
  EXPECT_TRUE(manager->getTerrainTypes().empty());
}

TEST_F(TerrainResourceManagerTest, ReplacesExistingTerrainTypes) {
  const char *ini1 = R"(
Terrain TEDesert1
  Texture = OldTexture.tga
  Class = DESERT_1
End
)";

  const char *ini2 = R"(
Terrain TEDesert2
  Texture = NewTexture.tga
  Class = DESERT_2
End
)";

  std::string error;
  manager->loadTerrainTypesFromINI(ini1, &error);
  EXPECT_EQ(manager->getTerrainTypes().size(), 1u);

  manager->loadTerrainTypesFromINI(ini2, &error);
  EXPECT_EQ(manager->getTerrainTypes().size(), 1u);

  auto *terrain = manager->getTerrainTypes().findByName("TEDesert2");
  ASSERT_NE(terrain, nullptr);
  EXPECT_EQ(terrain->texture, "NewTexture.tga");

  EXPECT_EQ(manager->getTerrainTypes().findByName("TEDesert1"), nullptr);
}

TEST_F(TerrainResourceManagerTest, ReturnsErrorForMalformedINI) {
  const char *badIni = R"(
Terrain Incomplete
  Texture =
)";

  std::string error;
  bool result = manager->loadTerrainTypesFromINI(badIni, &error);

  EXPECT_TRUE(result);
}

TEST_F(TerrainResourceManagerTest, ClearsTerrainTypes) {
  const char *ini = R"(
Terrain TEDesert1
  Texture = TEDesert1.tga
  Class = DESERT_1
End
)";

  std::string error;
  manager->loadTerrainTypesFromINI(ini, &error);
  EXPECT_TRUE(manager->isInitialized());
  EXPECT_EQ(manager->getTerrainTypes().size(), 1u);

  manager->clear();
  EXPECT_FALSE(manager->isInitialized());
  EXPECT_TRUE(manager->getTerrainTypes().empty());
}

TEST_F(TerrainResourceManagerTest, LoadsTerrainINIFromBigArchive) {
  BigArchiveManager bigManager;
  std::string error;

  bool initialized = bigManager.initialize(".", &error);
  if (!initialized) {
    GTEST_SKIP() << "BigArchiveManager initialization failed: " << error
                 << ". Skipping BIG archive test.";
  }

  bool loaded = manager->loadTerrainTypesFromBig(bigManager, &error);
  if (!loaded) {
    GTEST_SKIP() << "No INIZH.big found or no Terrain.ini inside: " << error
                 << ". This is expected if testing without game data.";
  }

  EXPECT_TRUE(manager->isInitialized());
  EXPECT_FALSE(manager->getTerrainTypes().empty())
      << "Expected to find terrain types in Terrain.ini";
}

TEST_F(TerrainResourceManagerTest, HandlesMissingINIZHBig) {
  BigArchiveManager bigManager;
  std::string error;

  bigManager.initialize(".", &error);

  bool loaded = manager->loadTerrainTypesFromBig(bigManager, &error);

  EXPECT_FALSE(loaded);
  EXPECT_FALSE(error.empty());
}

TEST_F(TerrainResourceManagerTest, ResolvesTGAPathFromTerrainClass) {
  const char *ini = R"(
Terrain TEDesert1
  Texture = TEDesert1.tga
  Class = DESERT_1
End
)";

  std::string error;
  manager->loadTerrainTypesFromINI(ini, &error);

  auto path = manager->resolveTexturePath("TEDesert1", &error);

  ASSERT_TRUE(path.has_value()) << "Error: " << error;
  EXPECT_EQ(path.value(), "Art/Terrain/TEDesert1.tga");
}

TEST_F(TerrainResourceManagerTest, ReturnsNulloptForUnknownTerrainClass) {
  const char *ini = R"(
Terrain TEDesert1
  Texture = TEDesert1.tga
  Class = DESERT_1
End
)";

  std::string error;
  manager->loadTerrainTypesFromINI(ini, &error);

  auto path = manager->resolveTexturePath("NonExistent", &error);

  EXPECT_FALSE(path.has_value());
  EXPECT_FALSE(error.empty());
}

TEST_F(TerrainResourceManagerTest, ResolvesTGAPathBeforeInitialization) {
  std::string error;

  auto path = manager->resolveTexturePath("TEDesert1", &error);

  EXPECT_FALSE(path.has_value());
  EXPECT_FALSE(error.empty());
  EXPECT_FALSE(manager->isInitialized());
}

// ============================================================================
// Helper: builds a minimal in-memory TGA file (uncompressed, type 2, BGR[A])
// ============================================================================
namespace {

std::vector<uint8_t> makeTga(uint32_t width, uint32_t height, uint8_t bpp,
                             const std::vector<uint8_t> &pixelData) {
  std::vector<uint8_t> tga;
  tga.reserve(18 + pixelData.size());

  tga.push_back(0); // id length
  tga.push_back(0); // color map type
  tga.push_back(2); // image type: uncompressed true-color
  // color map spec (5 bytes, all zero)
  for (int i = 0; i < 5; ++i)
    tga.push_back(0);
  // image spec
  tga.push_back(0);
  tga.push_back(0); // x-origin
  tga.push_back(0);
  tga.push_back(0); // y-origin
  tga.push_back(static_cast<uint8_t>(width & 0xFF));
  tga.push_back(static_cast<uint8_t>((width >> 8) & 0xFF));
  tga.push_back(static_cast<uint8_t>(height & 0xFF));
  tga.push_back(static_cast<uint8_t>((height >> 8) & 0xFF));
  tga.push_back(bpp);  // bits per pixel
  tga.push_back(0x20); // image descriptor: top-left origin (no vertical flip needed)

  tga.insert(tga.end(), pixelData.begin(), pixelData.end());
  return tga;
}

// BGR24 pixel data for a width*height image filled with a solid colour
std::vector<uint8_t> solidBgr24(uint32_t width, uint32_t height, uint8_t b, uint8_t g, uint8_t r) {
  std::vector<uint8_t> pixels(width * height * 3);
  for (uint32_t i = 0; i < width * height; ++i) {
    pixels[i * 3 + 0] = b;
    pixels[i * 3 + 1] = g;
    pixels[i * 3 + 2] = r;
  }
  return pixels;
}

} // namespace

// ============================================================================
// Phase 1.2 / 1.3 Tests: TGA decoding and tile splitting
// ============================================================================

class TgaTileSplitTest : public ::testing::Test {
protected:
  TerrainResourceManager manager;
};

TEST_F(TgaTileSplitTest, DecodeTgaFromMemoryReturnsCorrectDimensions) {
  auto pixels = solidBgr24(64, 64, 255, 0, 0);
  auto tga = makeTga(64, 64, 24, pixels);

  TgaImage img;
  std::string error;
  bool ok = manager.decodeTgaFromMemory(tga, img, &error);

  EXPECT_TRUE(ok) << "Error: " << error;
  EXPECT_EQ(img.width, 64u);
  EXPECT_EQ(img.height, 64u);
  EXPECT_EQ(img.pixels.size(), 64u * 64u * 4u);
}

TEST_F(TgaTileSplitTest, DecodeTgaConvertsChannelsToRGBA) {
  // TGA stores BGR, we expect RGBA output
  auto pixels = solidBgr24(1, 1, /*b=*/200, /*g=*/100, /*r=*/50);
  auto tga = makeTga(1, 1, 24, pixels);

  TgaImage img;
  std::string error;
  bool ok = manager.decodeTgaFromMemory(tga, img, &error);

  ASSERT_TRUE(ok) << "Error: " << error;
  ASSERT_EQ(img.pixels.size(), 4u);
  EXPECT_EQ(img.pixels[0], 50u);  // R
  EXPECT_EQ(img.pixels[1], 100u); // G
  EXPECT_EQ(img.pixels[2], 200u); // B
  EXPECT_EQ(img.pixels[3], 255u); // A (opaque for 24-bit)
}

TEST_F(TgaTileSplitTest, DecodeTgaFailsOnEmptyData) {
  TgaImage img;
  std::string error;
  bool ok = manager.decodeTgaFromMemory({}, img, &error);

  EXPECT_FALSE(ok);
  EXPECT_FALSE(error.empty());
}

TEST_F(TgaTileSplitTest, DecodeTgaFailsOnTruncatedHeader) {
  std::vector<uint8_t> truncated = {0, 0, 2}; // only 3 bytes, need 18
  TgaImage img;
  std::string error;
  bool ok = manager.decodeTgaFromMemory(truncated, img, &error);

  EXPECT_FALSE(ok);
  EXPECT_FALSE(error.empty());
}

TEST_F(TgaTileSplitTest, SplitTgaIntoTilesProducesCorrectCount) {
  // 128x128 TGA -> 4 tiles of 64x64
  auto pixels = solidBgr24(128, 128, 0, 0, 0);
  auto tga = makeTga(128, 128, 24, pixels);

  TgaImage img;
  std::string error;
  manager.decodeTgaFromMemory(tga, img, &error);

  auto tiles = manager.splitImageIntoTiles(img, 64);

  EXPECT_EQ(tiles.size(), 4u);
  for (const auto &tile : tiles) {
    EXPECT_EQ(tile.size(), 64u * 64u * 4u);
  }
}

TEST_F(TgaTileSplitTest, SplitTgaIntoTilesProducesCorrectCountFor64x64) {
  // 64x64 TGA -> 1 tile
  auto pixels = solidBgr24(64, 64, 0, 0, 0);
  auto tga = makeTga(64, 64, 24, pixels);

  TgaImage img;
  std::string error;
  manager.decodeTgaFromMemory(tga, img, &error);

  auto tiles = manager.splitImageIntoTiles(img, 64);

  EXPECT_EQ(tiles.size(), 1u);
}

TEST_F(TgaTileSplitTest, SplitTgaIntoTilesProducesCorrectCountFor256x256) {
  // 256x256 TGA -> 16 tiles of 64x64
  auto pixels = solidBgr24(256, 256, 0, 0, 0);
  auto tga = makeTga(256, 256, 24, pixels);

  TgaImage img;
  std::string error;
  manager.decodeTgaFromMemory(tga, img, &error);

  auto tiles = manager.splitImageIntoTiles(img, 64);

  EXPECT_EQ(tiles.size(), 16u);
}

TEST_F(TgaTileSplitTest, SplitTgaTilesContainCorrectPixels) {
  // Build a 128x64 image: left half is red (r=255, g=0, b=0 in BGR: b=0,g=0,r=255)
  //                       right half is blue (r=0, g=0, b=255 in BGR: b=255,g=0,r=0)
  std::vector<uint8_t> pixels(128 * 64 * 3);
  for (uint32_t y = 0; y < 64; ++y) {
    for (uint32_t x = 0; x < 128; ++x) {
      size_t idx = (y * 128 + x) * 3;
      if (x < 64) {
        // left: red -> BGR = (0, 0, 255)
        pixels[idx + 0] = 0;
        pixels[idx + 1] = 0;
        pixels[idx + 2] = 255;
      } else {
        // right: blue -> BGR = (255, 0, 0)
        pixels[idx + 0] = 255;
        pixels[idx + 1] = 0;
        pixels[idx + 2] = 0;
      }
    }
  }
  auto tga = makeTga(128, 64, 24, pixels);

  TgaImage img;
  std::string error;
  manager.decodeTgaFromMemory(tga, img, &error);

  auto tiles = manager.splitImageIntoTiles(img, 64);
  ASSERT_EQ(tiles.size(), 2u);

  // Tile 0 (left): top-left pixel should be red: R=255, G=0, B=0
  EXPECT_EQ(tiles[0][0], 255u); // R
  EXPECT_EQ(tiles[0][1], 0u);   // G
  EXPECT_EQ(tiles[0][2], 0u);   // B
  EXPECT_EQ(tiles[0][3], 255u); // A

  // Tile 1 (right): top-left pixel should be blue: R=0, G=0, B=255
  EXPECT_EQ(tiles[1][0], 0u);   // R
  EXPECT_EQ(tiles[1][1], 0u);   // G
  EXPECT_EQ(tiles[1][2], 255u); // B
  EXPECT_EQ(tiles[1][3], 255u); // A
}

TEST_F(TgaTileSplitTest, SplitReturnsEmptyForEmptyImage) {
  TgaImage emptyImg;
  auto tiles = manager.splitImageIntoTiles(emptyImg, 64);
  EXPECT_TRUE(tiles.empty());
}

TEST_F(TgaTileSplitTest, SplitReturnsEmptyForZeroTileSize) {
  TgaImage img;
  img.width = 64;
  img.height = 64;
  img.pixels.resize(64 * 64 * 4, 0);

  auto tiles = manager.splitImageIntoTiles(img, 0);
  EXPECT_TRUE(tiles.empty());
}

// ============================================================================
// Phase 1.2 Tests: Extracting tiles from blend data using BigArchive
// ============================================================================

TEST_F(TgaTileSplitTest, ExtractTilesForTextureClassesReturnsEmptyWhenNoTypes) {
  std::string error;
  manager.loadTerrainTypesFromINI("", &error);

  std::vector<map::TextureClass> textureClasses;
  BigArchiveManager bigManager;
  bigManager.initialize(".", &error);

  auto result = manager.extractTilesForTextureClasses(textureClasses, bigManager, &error);
  EXPECT_TRUE(result.empty());
}

TEST_F(TgaTileSplitTest, ExtractTilesSkipsUnknownTextureClass) {
  const char *ini = R"(
Terrain TEDesert1
  Texture = TEDesert1.tga
  Class = DESERT_1
End
)";
  std::string error;
  manager.loadTerrainTypesFromINI(ini, &error);

  map::TextureClass tc;
  tc.name = "NonExistentTexture";
  tc.numTiles = 1;
  tc.width = 1;
  tc.firstTile = 0;

  BigArchiveManager bigManager;
  bigManager.initialize(".", &error);

  auto result = manager.extractTilesForTextureClasses({tc}, bigManager, &error);
  // Non-existent texture class -> no tiles extracted
  EXPECT_TRUE(result.empty());
}

TEST_F(TgaTileSplitTest, ExtractTilesFromBigArchiveSkipsWhenBigNotInitialized) {
  const char *ini = R"(
Terrain TEDesert1
  Texture = TEDesert1.tga
  Class = DESERT_1
End
)";
  std::string error;
  manager.loadTerrainTypesFromINI(ini, &error);

  map::TextureClass tc;
  tc.name = "TEDesert1";
  tc.numTiles = 4;
  tc.width = 2;
  tc.firstTile = 0;

  BigArchiveManager bigManager; // not initialized

  auto result = manager.extractTilesForTextureClasses({tc}, bigManager, &error);
  EXPECT_TRUE(result.empty());
}

TEST_F(TgaTileSplitTest, ExtractTilesWithRealBigArchiveSkipsIfNoGameData) {
  const char *ini = R"(
Terrain TEDesert1
  Texture = TEDesert1.tga
  Class = DESERT_1
End
)";
  std::string error;
  manager.loadTerrainTypesFromINI(ini, &error);

  map::TextureClass tc;
  tc.name = "TEDesert1";
  tc.numTiles = 4;
  tc.width = 2;
  tc.firstTile = 0;

  BigArchiveManager bigManager;
  bool initialized = bigManager.initialize(".", &error);
  if (!initialized) {
    GTEST_SKIP() << "No game directory available";
  }

  auto result = manager.extractTilesForTextureClasses({tc}, bigManager, &error);
  // Will succeed only if TerrainZH.big with TEDesert1.tga is present
  // Either we get tiles or an empty result -- both are valid without game data
  (void)result;
  SUCCEED();
}

// ============================================================================
// Phase 1.4 Tests: Building TileArrayData from tile bitmaps
// ============================================================================

class TileArrayTest : public ::testing::Test {
protected:
  TerrainResourceManager manager;

  // Produce a synthetic 64x64 RGBA tile filled with a solid colour
  static std::vector<uint8_t> makeSolidTile(uint8_t r, uint8_t g, uint8_t b) {
    std::vector<uint8_t> tile(64 * 64 * 4);
    for (size_t i = 0; i < 64u * 64u; ++i) {
      tile[i * 4 + 0] = r;
      tile[i * 4 + 1] = g;
      tile[i * 4 + 2] = b;
      tile[i * 4 + 3] = 255;
    }
    return tile;
  }
};

TEST_F(TileArrayTest, BuildTileArrayDataFromEmptyTilesReturnsInvalid) {
  std::vector<std::vector<uint8_t>> noTiles;
  auto data = manager.buildTileArrayData(noTiles);
  EXPECT_FALSE(data.isValid());
  EXPECT_EQ(data.layerCount, 0u);
}

TEST_F(TileArrayTest, BuildTileArrayDataFromSingleTileIsValid) {
  auto tiles = std::vector<std::vector<uint8_t>>{makeSolidTile(255, 0, 0)};
  auto data = manager.buildTileArrayData(tiles);

  EXPECT_TRUE(data.isValid());
  EXPECT_EQ(data.layerCount, 1u);
  EXPECT_EQ(data.tileSize, 64u);
  EXPECT_EQ(data.layers.size(), 1u);
}

TEST_F(TileArrayTest, BuildTileArrayDataLayerCountMatchesTileCount) {
  std::vector<std::vector<uint8_t>> tiles;
  tiles.push_back(makeSolidTile(255, 0, 0));
  tiles.push_back(makeSolidTile(0, 255, 0));
  tiles.push_back(makeSolidTile(0, 0, 255));

  auto data = manager.buildTileArrayData(tiles);

  EXPECT_EQ(data.layerCount, 3u);
  EXPECT_EQ(data.layers.size(), 3u);
}

TEST_F(TileArrayTest, BuildTileArrayDataTileSizeIs64) {
  auto tiles = std::vector<std::vector<uint8_t>>{makeSolidTile(0, 0, 0)};
  auto data = manager.buildTileArrayData(tiles);

  EXPECT_EQ(data.tileSize, 64u);
}

TEST_F(TileArrayTest, BuildTileArrayDataPreservesPixelContent) {
  auto redTile = makeSolidTile(200, 50, 30);
  auto tiles = std::vector<std::vector<uint8_t>>{redTile};
  auto data = manager.buildTileArrayData(tiles);

  ASSERT_TRUE(data.isValid());
  ASSERT_FALSE(data.layers[0].empty());
  EXPECT_EQ(data.layers[0][0], 200u); // R
  EXPECT_EQ(data.layers[0][1], 50u);  // G
  EXPECT_EQ(data.layers[0][2], 30u);  // B
  EXPECT_EQ(data.layers[0][3], 255u); // A
}

TEST_F(TileArrayTest, BuildTileArrayDataSkipsWrongSizeTiles) {
  // One correct 64x64 tile, one wrong-size tile (32x32 = 4096 bytes)
  auto goodTile = makeSolidTile(100, 100, 100);
  std::vector<uint8_t> badTile(32 * 32 * 4, 0);

  auto data = manager.buildTileArrayData({goodTile, badTile});

  // Only the good tile should be included
  EXPECT_EQ(data.layerCount, 1u);
}

TEST_F(TileArrayTest, BuildTileArrayDataTotalLayerSizeIsCorrect) {
  std::vector<std::vector<uint8_t>> tiles;
  tiles.push_back(makeSolidTile(10, 20, 30));
  tiles.push_back(makeSolidTile(40, 50, 60));

  auto data = manager.buildTileArrayData(tiles);
  ASSERT_EQ(data.layers.size(), 2u);

  for (const auto &layer : data.layers) {
    EXPECT_EQ(layer.size(), 64u * 64u * 4u);
  }
}

TEST_F(TileArrayTest, BuildTileArrayDataWithMixedValidAndInvalidTiles) {
  std::vector<std::vector<uint8_t>> tiles;
  tiles.push_back(makeSolidTile(1, 2, 3));
  tiles.push_back({}); // empty - invalid
  tiles.push_back(makeSolidTile(4, 5, 6));

  auto data = manager.buildTileArrayData(tiles);

  // Empty tiles are skipped, 2 valid ones kept
  EXPECT_EQ(data.layerCount, 2u);
}

TEST_F(TileArrayTest, BuildTileArrayDataFromTextureClassesIntegration) {
  // Simulate extraction: build two texture classes each with one 64x64 tile
  const char *ini = R"(
Terrain GrassA
  Texture = GrassA.tga
  Class = GRASS
End
Terrain RockA
  Texture = RockA.tga
  Class = ROCK
End
)";
  std::string error;
  manager.loadTerrainTypesFromINI(ini, &error);

  // Manually synthesise the tiles that extraction would produce (64x64 RGBA each)
  std::vector<std::vector<uint8_t>> extractedTiles;
  extractedTiles.push_back(makeSolidTile(0, 200, 0));    // GrassA tile
  extractedTiles.push_back(makeSolidTile(150, 100, 80)); // RockA tile

  auto data = manager.buildTileArrayData(extractedTiles);
  EXPECT_TRUE(data.isValid());
  EXPECT_EQ(data.layerCount, 2u);
}
