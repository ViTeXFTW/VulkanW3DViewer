#include "render/water/water_mesh.hpp"

#include <gtest/gtest.h>

#include <glm/glm.hpp>

#include <cmath>

using namespace w3d::water;
using namespace w3d;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

map::PolygonTrigger makeWaterTrigger(const std::string &name,
                                     const std::vector<glm::ivec3> &points,
                                     bool isWater = true) {
  map::PolygonTrigger t;
  t.name        = name;
  t.id          = 1;
  t.isWaterArea = isWater;
  t.isRiver     = false;
  t.riverStart  = 0;
  t.points      = points;
  return t;
}

// Square water area (4 corners, CCW winding when viewed from above).
map::PolygonTrigger makeSquareTrigger(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                                      int32_t z = 10) {
  return makeWaterTrigger("Square", {
                                        {x0, y0, z},
                                        {x1, y0, z},
                                        {x1, y1, z},
                                        {x0, y1, z},
                                    });
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// triggerPointToWorld
// ─────────────────────────────────────────────────────────────────────────────

TEST(TriggerPointToWorld, OriginMapsToOrigin) {
  auto world = triggerPointToWorld({0, 0, 0});
  EXPECT_FLOAT_EQ(world.x, 0.0f);
  EXPECT_FLOAT_EQ(world.y, 0.0f);
  EXPECT_FLOAT_EQ(world.z, 0.0f);
}

TEST(TriggerPointToWorld, XYScaledByMapXYFactor) {
  auto world = triggerPointToWorld({1, 0, 0});
  EXPECT_FLOAT_EQ(world.x, map::MAP_XY_FACTOR);

  auto world2 = triggerPointToWorld({0, 1, 0});
  EXPECT_FLOAT_EQ(world2.z, map::MAP_XY_FACTOR);
}

TEST(TriggerPointToWorld, ZMapsToWorldHeight) {
  auto world = triggerPointToWorld({0, 0, 16});
  // 16 * MAP_HEIGHT_SCALE = 16 * (MAP_XY_FACTOR / 16) = MAP_XY_FACTOR
  EXPECT_FLOAT_EQ(world.y, map::MAP_XY_FACTOR);
}

TEST(TriggerPointToWorld, MapYMapsToWorldZ) {
  // map Y (north/south) → world Z
  auto world = triggerPointToWorld({0, 5, 0});
  EXPECT_FLOAT_EQ(world.x, 0.0f);
  EXPECT_FLOAT_EQ(world.z, 5.0f * map::MAP_XY_FACTOR);
  EXPECT_FLOAT_EQ(world.y, 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// earClipTriangulate – convex polygons
// ─────────────────────────────────────────────────────────────────────────────

TEST(EarClipTriangulate, TriangleProducesSingleTriangle) {
  std::vector<glm::vec2> tri = {{0, 0}, {10, 0}, {5, 10}};
  auto indices               = earClipTriangulate(tri);
  ASSERT_EQ(indices.size(), 3u);
  // All indices must reference valid vertices.
  for (uint32_t idx : indices) {
    EXPECT_LT(idx, static_cast<uint32_t>(tri.size()));
  }
}

TEST(EarClipTriangulate, SquareProducesTwoTriangles) {
  // CCW square
  std::vector<glm::vec2> sq = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};
  auto indices               = earClipTriangulate(sq);
  EXPECT_EQ(indices.size(), 6u); // 2 triangles × 3 indices
  EXPECT_EQ(indices.size() % 3, 0u);
}

TEST(EarClipTriangulate, PentagonProducesThreeTriangles) {
  std::vector<glm::vec2> pentagon;
  for (int i = 0; i < 5; ++i) {
    float angle = static_cast<float>(i) * 2.0f * 3.14159f / 5.0f;
    pentagon.emplace_back(std::cos(angle), std::sin(angle));
  }
  auto indices = earClipTriangulate(pentagon);
  EXPECT_EQ(indices.size(), 9u); // 3 triangles
  EXPECT_EQ(indices.size() % 3, 0u);
}

TEST(EarClipTriangulate, TooFewVerticesReturnsEmpty) {
  EXPECT_TRUE(earClipTriangulate({}).empty());
  EXPECT_TRUE(earClipTriangulate({{0, 0}}).empty());
  EXPECT_TRUE(earClipTriangulate({{0, 0}, {1, 0}}).empty());
}

TEST(EarClipTriangulate, CWPolygonIsHandled) {
  // CW square – should still produce valid triangles.
  std::vector<glm::vec2> cw = {{0, 0}, {0, 10}, {10, 10}, {10, 0}};
  auto indices               = earClipTriangulate(cw);
  EXPECT_EQ(indices.size(), 6u);
  for (uint32_t idx : indices) {
    EXPECT_LT(idx, static_cast<uint32_t>(cw.size()));
  }
}

TEST(EarClipTriangulate, AllIndicesInBounds) {
  std::vector<glm::vec2> hex;
  for (int i = 0; i < 6; ++i) {
    float angle = static_cast<float>(i) * 2.0f * 3.14159f / 6.0f;
    hex.emplace_back(std::cos(angle), std::sin(angle));
  }
  auto indices = earClipTriangulate(hex);
  ASSERT_EQ(indices.size(), 12u); // 4 triangles
  for (uint32_t idx : indices) {
    EXPECT_LT(idx, static_cast<uint32_t>(hex.size()));
  }
}

TEST(EarClipTriangulate, TriangleCountIsNMinus2) {
  for (int n = 3; n <= 8; ++n) {
    std::vector<glm::vec2> poly;
    for (int i = 0; i < n; ++i) {
      float angle = static_cast<float>(i) * 2.0f * 3.14159f / static_cast<float>(n);
      poly.emplace_back(std::cos(angle), std::sin(angle));
    }
    auto indices = earClipTriangulate(poly);
    EXPECT_EQ(indices.size(), static_cast<size_t>((n - 2) * 3))
        << "For n=" << n << " vertices";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// generateWaterPolygon
// ─────────────────────────────────────────────────────────────────────────────

TEST(GenerateWaterPolygon, NonWaterTriggerReturnsNullopt) {
  auto trigger = makeWaterTrigger("NonWater", {{0, 0, 0}, {10, 0, 0}, {10, 10, 0}},
                                   /*isWater=*/false);
  auto result  = generateWaterPolygon(trigger);
  EXPECT_FALSE(result.has_value());
}

TEST(GenerateWaterPolygon, TooFewPointsReturnsNullopt) {
  auto trigger = makeWaterTrigger("TwoPoints", {{0, 0, 0}, {10, 0, 0}});
  auto result  = generateWaterPolygon(trigger);
  EXPECT_FALSE(result.has_value());
}

TEST(GenerateWaterPolygon, TriangleProducesValidPolygon) {
  auto trigger =
      makeWaterTrigger("Tri", {{0, 0, 10}, {10, 0, 10}, {5, 10, 10}});
  auto result = generateWaterPolygon(trigger);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->vertices.size(), 3u);
  EXPECT_EQ(result->indices.size(), 3u);
  EXPECT_EQ(result->name, "Tri");
}

TEST(GenerateWaterPolygon, SquareProducesCorrectIndexCount) {
  auto trigger = makeSquareTrigger(0, 0, 10, 10, 20);
  auto result  = generateWaterPolygon(trigger);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->vertices.size(), 4u);
  EXPECT_EQ(result->indices.size(), 6u); // 2 triangles
}

TEST(GenerateWaterPolygon, WaterSurfaceIsFlatAtHeight) {
  // All points at z=20 in trigger coords → world Y = 20 * MAP_HEIGHT_SCALE
  int32_t trigZ  = 20;
  float expected = static_cast<float>(trigZ) * map::MAP_HEIGHT_SCALE;

  auto trigger = makeSquareTrigger(0, 0, 10, 10, trigZ);
  auto result  = generateWaterPolygon(trigger);
  ASSERT_TRUE(result.has_value());

  for (const auto &v : result->vertices) {
    EXPECT_NEAR(v.position.y, expected, 0.001f)
        << "All vertices must be at the water height";
  }
}

TEST(GenerateWaterPolygon, WaterHeightMatchesAveragedZCoord) {
  // Mix of slightly different z values to test averaging.
  auto trigger = makeWaterTrigger("MixedZ",
                                   {{0, 0, 10}, {10, 0, 12}, {10, 10, 10}, {0, 10, 12}});
  auto result  = generateWaterPolygon(trigger);
  ASSERT_TRUE(result.has_value());

  float avgZ = (10.0f + 12.0f + 10.0f + 12.0f) / 4.0f;
  float expected = avgZ * map::MAP_HEIGHT_SCALE;
  EXPECT_NEAR(result->waterHeight, expected, 0.001f);
}

TEST(GenerateWaterPolygon, BoundsAreValid) {
  auto trigger = makeSquareTrigger(0, 0, 5, 5, 10);
  auto result  = generateWaterPolygon(trigger);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->bounds.valid());
}

TEST(GenerateWaterPolygon, BoundsContainAllVertices) {
  auto trigger = makeSquareTrigger(2, 3, 8, 9, 15);
  auto result  = generateWaterPolygon(trigger);
  ASSERT_TRUE(result.has_value());

  const auto &bb = result->bounds;
  for (const auto &v : result->vertices) {
    EXPECT_GE(v.position.x, bb.min.x);
    EXPECT_LE(v.position.x, bb.max.x);
    EXPECT_GE(v.position.y, bb.min.y);
    EXPECT_LE(v.position.y, bb.max.y);
    EXPECT_GE(v.position.z, bb.min.z);
    EXPECT_LE(v.position.z, bb.max.z);
  }
}

TEST(GenerateWaterPolygon, TexCoordsAreNormalisedByMapXYFactor) {
  auto trigger = makeSquareTrigger(0, 0, 1, 1, 10);
  auto result  = generateWaterPolygon(trigger);
  ASSERT_TRUE(result.has_value());

  // For a unit square (1 map cell), the UV difference should be 1.0.
  float minU = result->vertices[0].texCoord.x;
  float maxU = result->vertices[0].texCoord.x;
  for (const auto &v : result->vertices) {
    minU = std::min(minU, v.texCoord.x);
    maxU = std::max(maxU, v.texCoord.x);
  }
  EXPECT_NEAR(maxU - minU, 1.0f, 0.001f);
}

TEST(GenerateWaterPolygon, IndicesReferenceValidVertices) {
  auto trigger = makeSquareTrigger(0, 0, 10, 10, 5);
  auto result  = generateWaterPolygon(trigger);
  ASSERT_TRUE(result.has_value());

  uint32_t nv = static_cast<uint32_t>(result->vertices.size());
  for (uint32_t idx : result->indices) {
    EXPECT_LT(idx, nv);
  }
}

TEST(GenerateWaterPolygon, IndicesFormCompleteTriangles) {
  auto trigger = makeSquareTrigger(0, 0, 10, 10, 5);
  auto result  = generateWaterPolygon(trigger);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->indices.size() % 3, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// generateWaterMeshes
// ─────────────────────────────────────────────────────────────────────────────

TEST(GenerateWaterMeshes, EmptyTriggersProducesEmpty) {
  auto data = generateWaterMeshes({});
  EXPECT_TRUE(data.polygons.empty());
  EXPECT_FALSE(data.totalBounds.valid());
}

TEST(GenerateWaterMeshes, NonWaterTriggersAreSkipped) {
  std::vector<map::PolygonTrigger> triggers;
  triggers.push_back(makeWaterTrigger("Land", {{0, 0, 0}, {10, 0, 0}, {5, 10, 0}},
                                      /*isWater=*/false));
  auto data = generateWaterMeshes(triggers);
  EXPECT_TRUE(data.polygons.empty());
}

TEST(GenerateWaterMeshes, SingleWaterTriggerProducesSinglePolygon) {
  std::vector<map::PolygonTrigger> triggers;
  triggers.push_back(makeSquareTrigger(0, 0, 10, 10));
  auto data = generateWaterMeshes(triggers);
  ASSERT_EQ(data.polygons.size(), 1u);
}

TEST(GenerateWaterMeshes, MultipleWaterTriggersAllIncluded) {
  std::vector<map::PolygonTrigger> triggers;
  triggers.push_back(makeSquareTrigger(0, 0, 5, 5, 10));
  triggers.push_back(makeSquareTrigger(10, 10, 15, 15, 10));
  triggers.push_back(makeSquareTrigger(20, 20, 25, 25, 10));
  auto data = generateWaterMeshes(triggers);
  EXPECT_EQ(data.polygons.size(), 3u);
}

TEST(GenerateWaterMeshes, MixedTriggersOnlyWaterOnes) {
  std::vector<map::PolygonTrigger> triggers;
  triggers.push_back(makeSquareTrigger(0, 0, 5, 5, 10));
  triggers.push_back(makeWaterTrigger("Land", {{0, 0, 0}, {5, 0, 0}, {5, 5, 0}}, false));
  triggers.push_back(makeSquareTrigger(10, 10, 15, 15, 10));
  auto data = generateWaterMeshes(triggers);
  EXPECT_EQ(data.polygons.size(), 2u);
}

TEST(GenerateWaterMeshes, TotalBoundsContainsAllPolygons) {
  std::vector<map::PolygonTrigger> triggers;
  triggers.push_back(makeSquareTrigger(0, 0, 5, 5, 10));
  triggers.push_back(makeSquareTrigger(10, 10, 15, 15, 20));
  auto data = generateWaterMeshes(triggers);
  ASSERT_EQ(data.polygons.size(), 2u);
  EXPECT_TRUE(data.totalBounds.valid());

  for (const auto &poly : data.polygons) {
    EXPECT_GE(poly.bounds.min.x, data.totalBounds.min.x);
    EXPECT_GE(poly.bounds.min.z, data.totalBounds.min.z);
    EXPECT_LE(poly.bounds.max.x, data.totalBounds.max.x);
    EXPECT_LE(poly.bounds.max.z, data.totalBounds.max.z);
  }
}

TEST(GenerateWaterMeshes, PolygonNamesPreserved) {
  std::vector<map::PolygonTrigger> triggers;
  auto t1 = makeSquareTrigger(0, 0, 5, 5, 10);
  t1.name = "Lake";
  auto t2 = makeSquareTrigger(10, 10, 15, 15, 10);
  t2.name = "River";
  triggers.push_back(t1);
  triggers.push_back(t2);
  auto data = generateWaterMeshes(triggers);
  ASSERT_EQ(data.polygons.size(), 2u);
  EXPECT_EQ(data.polygons[0].name, "Lake");
  EXPECT_EQ(data.polygons[1].name, "River");
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinate consistency with terrain
// ─────────────────────────────────────────────────────────────────────────────

TEST(WaterMesh, WaterHeightConsistentWithTerrainScale) {
  // Heightmap value 128 → world height 128 * MAP_HEIGHT_SCALE.
  // Water at trigger Z=128 should be at the same world Y.
  float terrainHeight = 128.0f * map::MAP_HEIGHT_SCALE;
  auto world          = triggerPointToWorld({0, 0, 128});
  EXPECT_NEAR(world.y, terrainHeight, 0.001f);
}

TEST(WaterMesh, XZScaleMatchesTerrainGrid) {
  // Map cell (1, 1) → world position (MAP_XY_FACTOR, ?, MAP_XY_FACTOR).
  auto world = triggerPointToWorld({1, 1, 0});
  EXPECT_FLOAT_EQ(world.x, map::MAP_XY_FACTOR);
  EXPECT_FLOAT_EQ(world.z, map::MAP_XY_FACTOR);
}
