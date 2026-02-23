#include "render/object_placement_utils.hpp"

#include <cctype>

#include <gtest/gtest.h>

using namespace w3d;

class ObjectPlacementUtilsTest : public ::testing::Test {};

TEST_F(ObjectPlacementUtilsTest, ResolveSimpleTemplateName) {
  auto path = ObjectPlacementUtils::templateNameToW3DName("AmericaBarracks");
  EXPECT_EQ(path, "AmericaBarracks");
}

TEST_F(ObjectPlacementUtilsTest, ResolvePathedTemplateName) {
  auto path = ObjectPlacementUtils::templateNameToW3DName("GLA/GLAWorker");
  EXPECT_EQ(path, "GLAWorker");
}

TEST_F(ObjectPlacementUtilsTest, ResolvePathedTemplateNameTwoLevels) {
  auto path = ObjectPlacementUtils::templateNameToW3DName("USA/Vehicles/AmericaTank");
  EXPECT_EQ(path, "AmericaTank");
}

TEST_F(ObjectPlacementUtilsTest, EmptyTemplateNameReturnsEmpty) {
  auto path = ObjectPlacementUtils::templateNameToW3DName("");
  EXPECT_EQ(path, "");
}

TEST_F(ObjectPlacementUtilsTest, TrailingSlashReturnsEmpty) {
  auto path = ObjectPlacementUtils::templateNameToW3DName("USA/");
  EXPECT_EQ(path, "");
}

TEST_F(ObjectPlacementUtilsTest, TemplateNameNormalizesToLowercase) {
  auto path = ObjectPlacementUtils::templateNameToW3DName("AmericaBarracks");
  std::string lower = path;
  for (auto &c : lower)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  EXPECT_EQ(lower, "americabarracks");
}

TEST_F(ObjectPlacementUtilsTest, BuildW3DFilename) {
  std::string name = ObjectPlacementUtils::templateNameToW3DName("AmericaBarracks");
  std::string filename = name + ".w3d";
  EXPECT_EQ(filename, "AmericaBarracks.w3d");
}

TEST_F(ObjectPlacementUtilsTest, IsRoadPoint) {
  EXPECT_TRUE(ObjectPlacementUtils::isRoadPoint(map::FLAG_ROAD_POINT1));
  EXPECT_TRUE(ObjectPlacementUtils::isRoadPoint(map::FLAG_ROAD_POINT2));
  EXPECT_FALSE(ObjectPlacementUtils::isRoadPoint(0));
  EXPECT_FALSE(ObjectPlacementUtils::isRoadPoint(map::FLAG_BRIDGE_POINT1));
}

TEST_F(ObjectPlacementUtilsTest, IsBridgePoint) {
  EXPECT_TRUE(ObjectPlacementUtils::isBridgePoint(map::FLAG_BRIDGE_POINT1));
  EXPECT_TRUE(ObjectPlacementUtils::isBridgePoint(map::FLAG_BRIDGE_POINT2));
  EXPECT_FALSE(ObjectPlacementUtils::isBridgePoint(0));
  EXPECT_FALSE(ObjectPlacementUtils::isBridgePoint(map::FLAG_ROAD_POINT1));
}

TEST_F(ObjectPlacementUtilsTest, ShouldRender) {
  EXPECT_TRUE(ObjectPlacementUtils::shouldRender(0));
  EXPECT_FALSE(ObjectPlacementUtils::shouldRender(map::FLAG_DONT_RENDER));
  EXPECT_FALSE(ObjectPlacementUtils::shouldRender(map::FLAG_ROAD_POINT1));
  EXPECT_FALSE(ObjectPlacementUtils::shouldRender(map::FLAG_BRIDGE_POINT1));
}

TEST_F(ObjectPlacementUtilsTest, MapObjectToVulkanPosition) {
  glm::vec3 mapPos = {100.0f, 200.0f, 12.5f};
  glm::vec3 vulkan = ObjectPlacementUtils::mapPositionToVulkan(mapPos);
  EXPECT_FLOAT_EQ(vulkan.x, 100.0f);
  EXPECT_FLOAT_EQ(vulkan.y, 12.5f);
  EXPECT_FLOAT_EQ(vulkan.z, 200.0f);
}
