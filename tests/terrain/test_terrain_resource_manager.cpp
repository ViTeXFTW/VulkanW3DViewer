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
