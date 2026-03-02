#include "../../src/lib/formats/ini/terrain_types.hpp"

#include <gtest/gtest.h>

using namespace ini;

class TerrainTypesTest : public ::testing::Test {
protected:
  TerrainTypeCollection collection;
};

TEST_F(TerrainTypesTest, StartsEmpty) {
  EXPECT_TRUE(collection.empty());
  EXPECT_EQ(collection.size(), 0u);
}

TEST_F(TerrainTypesTest, ParsesSingleTerrainType) {
  const char *ini = R"(
Terrain TEDesert1
  Texture = TEDesert1.tga
  BlendEdges = No
  Class = DESERT_1
  RestrictConstruction = No
End
)";

  collection.loadFromINI(ini);

  EXPECT_EQ(collection.size(), 1u);

  auto *terrain = collection.findByName("TEDesert1");
  ASSERT_NE(terrain, nullptr);
  EXPECT_EQ(terrain->name, "TEDesert1");
  EXPECT_EQ(terrain->texture, "TEDesert1.tga");
  EXPECT_FALSE(terrain->blendEdgeTexture);
  EXPECT_EQ(terrain->terrainClass, TerrainClass::Desert1);
  EXPECT_FALSE(terrain->restrictConstruction);
}

TEST_F(TerrainTypesTest, ParsesMultipleTerrainTypes) {
  const char *ini = R"(
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

Terrain SnowHeavy
  Texture = SnowHeavy.tga
  BlendEdges = No
  Class = SNOW_1
  RestrictConstruction = Yes
End
)";

  collection.loadFromINI(ini);

  EXPECT_EQ(collection.size(), 3u);

  auto *desert = collection.findByName("TEDesert1");
  ASSERT_NE(desert, nullptr);
  EXPECT_EQ(desert->texture, "TEDesert1.tga");
  EXPECT_EQ(desert->terrainClass, TerrainClass::Desert1);

  auto *grass = collection.findByName("GrassLight");
  ASSERT_NE(grass, nullptr);
  EXPECT_EQ(grass->texture, "GrassLight.tga");
  EXPECT_TRUE(grass->blendEdgeTexture);
  EXPECT_EQ(grass->terrainClass, TerrainClass::Grass);

  auto *snow = collection.findByName("SnowHeavy");
  ASSERT_NE(snow, nullptr);
  EXPECT_EQ(snow->texture, "SnowHeavy.tga");
  EXPECT_TRUE(snow->restrictConstruction);
  EXPECT_EQ(snow->terrainClass, TerrainClass::Snow1);
}

TEST_F(TerrainTypesTest, FindByNameReturnsNullForUnknown) {
  const char *ini = R"(
Terrain TEDesert1
  Texture = TEDesert1.tga
  Class = DESERT_1
End
)";

  collection.loadFromINI(ini);

  EXPECT_EQ(collection.findByName("NonExistent"), nullptr);
}

TEST_F(TerrainTypesTest, ClearsCollection) {
  const char *ini = R"(
Terrain TEDesert1
  Texture = TEDesert1.tga
  Class = DESERT_1
End
)";

  collection.loadFromINI(ini);
  EXPECT_EQ(collection.size(), 1u);

  collection.clear();
  EXPECT_TRUE(collection.empty());
  EXPECT_EQ(collection.findByName("TEDesert1"), nullptr);
}

TEST_F(TerrainTypesTest, ParsesAllTerrainClasses) {
  const char *ini = R"(
Terrain Desert2
  Texture = Desert2.tga
  Class = DESERT_2
End

Terrain EasternEurope1
  Texture = EasternEurope1.tga
  Class = EASTERN_EUROPE_1
End

Terrain Swiss1
  Texture = Swiss1.tga
  Class = SWISS_1
End

Terrain Urban1
  Texture = Urban1.tga
  Class = URBAN
End

Terrain Concrete1
  Texture = Concrete1.tga
  Class = CONCRETE
End

Terrain Asphalt1
  Texture = Asphalt1.tga
  Class = ASPHALT
End
)";

  collection.loadFromINI(ini);
  EXPECT_EQ(collection.size(), 6u);

  EXPECT_EQ(collection.findByName("Desert2")->terrainClass, TerrainClass::Desert2);
  EXPECT_EQ(collection.findByName("EasternEurope1")->terrainClass, TerrainClass::EasternEurope1);
  EXPECT_EQ(collection.findByName("Swiss1")->terrainClass, TerrainClass::Swiss1);
  EXPECT_EQ(collection.findByName("Urban1")->terrainClass, TerrainClass::Urban);
  EXPECT_EQ(collection.findByName("Concrete1")->terrainClass, TerrainClass::Concrete);
  EXPECT_EQ(collection.findByName("Asphalt1")->terrainClass, TerrainClass::Asphalt);
}

TEST_F(TerrainTypesTest, HandlesCommentsInINI) {
  const char *ini = R"(
; This is a comment
Terrain TEDesert1
  ; Another comment
  Texture = TEDesert1.tga
  Class = DESERT_1  ; inline comment
End
)";

  collection.loadFromINI(ini);
  EXPECT_EQ(collection.size(), 1u);

  auto *terrain = collection.findByName("TEDesert1");
  ASSERT_NE(terrain, nullptr);
  EXPECT_EQ(terrain->texture, "TEDesert1.tga");
}

TEST_F(TerrainTypesTest, OverwritesDuplicateTerrainNames) {
  const char *ini = R"(
Terrain TEDesert1
  Texture = OldTexture.tga
  Class = DESERT_1
End

Terrain TEDesert1
  Texture = NewTexture.tga
  Class = DESERT_2
End
)";

  collection.loadFromINI(ini);
  EXPECT_EQ(collection.size(), 1u);

  auto *terrain = collection.findByName("TEDesert1");
  ASSERT_NE(terrain, nullptr);
  EXPECT_EQ(terrain->texture, "NewTexture.tga");
  EXPECT_EQ(terrain->terrainClass, TerrainClass::Desert2);
}

TEST_F(TerrainTypesTest, DefaultsForMissingFields) {
  const char *ini = R"(
Terrain Minimal
  Texture = Minimal.tga
End
)";

  collection.loadFromINI(ini);
  EXPECT_EQ(collection.size(), 1u);

  auto *terrain = collection.findByName("Minimal");
  ASSERT_NE(terrain, nullptr);
  EXPECT_EQ(terrain->texture, "Minimal.tga");
  EXPECT_FALSE(terrain->blendEdgeTexture);
  EXPECT_EQ(terrain->terrainClass, TerrainClass::None);
  EXPECT_FALSE(terrain->restrictConstruction);
}

TEST_F(TerrainTypesTest, TerrainTypesVectorAccessible) {
  const char *ini = R"(
Terrain A
  Texture = A.tga
  Class = DESERT_1
End

Terrain B
  Texture = B.tga
  Class = GRASS
End
)";

  collection.loadFromINI(ini);

  const auto &types = collection.terrainTypes();
  EXPECT_EQ(types.size(), 2u);

  bool foundA = false;
  bool foundB = false;
  for (const auto &t : types) {
    if (t.name == "A")
      foundA = true;
    if (t.name == "B")
      foundB = true;
  }
  EXPECT_TRUE(foundA);
  EXPECT_TRUE(foundB);
}

TEST_F(TerrainTypesTest, HandlesMixedContentWithOtherBlockTypes) {
  const char *ini = R"(
; File might contain other block types that we don't care about
SomeOtherBlock FooBar
  RandomField = 42
End

Terrain TEDesert1
  Texture = TEDesert1.tga
  Class = DESERT_1
End

AnotherBlock Baz
  Stuff = Things
End
)";

  collection.loadFromINI(ini);
  EXPECT_EQ(collection.size(), 1u);

  auto *terrain = collection.findByName("TEDesert1");
  ASSERT_NE(terrain, nullptr);
  EXPECT_EQ(terrain->texture, "TEDesert1.tga");
}
