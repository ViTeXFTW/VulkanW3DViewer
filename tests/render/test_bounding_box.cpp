#include <cmath>
#include <limits>

#include "lib/gfx/bounding_box.hpp"

#include <gtest/gtest.h>

using namespace w3d;
using gfx::BoundingBox;

class BoundingBoxTest : public ::testing::Test {};

// =============================================================================
// Initial State Tests
// =============================================================================

TEST_F(BoundingBoxTest, DefaultConstructorCreatesInvalidBox) {
  BoundingBox box;

  EXPECT_FALSE(box.valid());
  EXPECT_EQ(box.min.x, std::numeric_limits<float>::max());
  EXPECT_EQ(box.max.x, std::numeric_limits<float>::lowest());
}

// =============================================================================
// Expand with Point Tests
// =============================================================================

TEST_F(BoundingBoxTest, ExpandWithSinglePointCreatesValidBox) {
  BoundingBox box;
  box.expand(glm::vec3(1.0f, 2.0f, 3.0f));

  EXPECT_TRUE(box.valid());
  EXPECT_FLOAT_EQ(box.min.x, 1.0f);
  EXPECT_FLOAT_EQ(box.min.y, 2.0f);
  EXPECT_FLOAT_EQ(box.min.z, 3.0f);
  EXPECT_FLOAT_EQ(box.max.x, 1.0f);
  EXPECT_FLOAT_EQ(box.max.y, 2.0f);
  EXPECT_FLOAT_EQ(box.max.z, 3.0f);
}

TEST_F(BoundingBoxTest, ExpandWithMultiplePointsGrowsBox) {
  BoundingBox box;
  box.expand(glm::vec3(0.0f, 0.0f, 0.0f));
  box.expand(glm::vec3(10.0f, 5.0f, 3.0f));
  box.expand(glm::vec3(-2.0f, 8.0f, -1.0f));

  EXPECT_FLOAT_EQ(box.min.x, -2.0f);
  EXPECT_FLOAT_EQ(box.min.y, 0.0f);
  EXPECT_FLOAT_EQ(box.min.z, -1.0f);
  EXPECT_FLOAT_EQ(box.max.x, 10.0f);
  EXPECT_FLOAT_EQ(box.max.y, 8.0f);
  EXPECT_FLOAT_EQ(box.max.z, 3.0f);
}

TEST_F(BoundingBoxTest, ExpandWithPointInsideBoxDoesNotGrow) {
  BoundingBox box;
  box.expand(glm::vec3(-10.0f, -10.0f, -10.0f));
  box.expand(glm::vec3(10.0f, 10.0f, 10.0f));

  // Point inside
  box.expand(glm::vec3(0.0f, 0.0f, 0.0f));

  EXPECT_FLOAT_EQ(box.min.x, -10.0f);
  EXPECT_FLOAT_EQ(box.max.x, 10.0f);
}

TEST_F(BoundingBoxTest, ExpandWithNegativeCoordinates) {
  BoundingBox box;
  box.expand(glm::vec3(-5.0f, -3.0f, -1.0f));
  box.expand(glm::vec3(-10.0f, -8.0f, -2.0f));

  EXPECT_FLOAT_EQ(box.min.x, -10.0f);
  EXPECT_FLOAT_EQ(box.min.y, -8.0f);
  EXPECT_FLOAT_EQ(box.min.z, -2.0f);
  EXPECT_FLOAT_EQ(box.max.x, -5.0f);
  EXPECT_FLOAT_EQ(box.max.y, -3.0f);
  EXPECT_FLOAT_EQ(box.max.z, -1.0f);
}

// =============================================================================
// Expand with Other Box Tests
// =============================================================================

TEST_F(BoundingBoxTest, ExpandWithOtherValidBox) {
  BoundingBox box1;
  box1.expand(glm::vec3(0.0f, 0.0f, 0.0f));
  box1.expand(glm::vec3(5.0f, 5.0f, 5.0f));

  BoundingBox box2;
  box2.expand(glm::vec3(3.0f, 3.0f, 3.0f));
  box2.expand(glm::vec3(10.0f, 10.0f, 10.0f));

  box1.expand(box2);

  EXPECT_FLOAT_EQ(box1.min.x, 0.0f);
  EXPECT_FLOAT_EQ(box1.max.x, 10.0f);
}

TEST_F(BoundingBoxTest, ExpandWithInvalidBoxDoesNotChange) {
  BoundingBox box1;
  box1.expand(glm::vec3(1.0f, 2.0f, 3.0f));
  box1.expand(glm::vec3(4.0f, 5.0f, 6.0f));

  BoundingBox invalidBox; // Default, invalid

  box1.expand(invalidBox);

  EXPECT_FLOAT_EQ(box1.min.x, 1.0f);
  EXPECT_FLOAT_EQ(box1.max.x, 4.0f);
}

TEST_F(BoundingBoxTest, ExpandInvalidBoxWithValidBox) {
  BoundingBox invalidBox; // Default, invalid

  BoundingBox validBox;
  validBox.expand(glm::vec3(1.0f, 2.0f, 3.0f));
  validBox.expand(glm::vec3(4.0f, 5.0f, 6.0f));

  invalidBox.expand(validBox);

  EXPECT_TRUE(invalidBox.valid());
  EXPECT_FLOAT_EQ(invalidBox.min.x, 1.0f);
  EXPECT_FLOAT_EQ(invalidBox.max.x, 4.0f);
}

// =============================================================================
// Center Tests
// =============================================================================

TEST_F(BoundingBoxTest, CenterOfSymmetricBox) {
  BoundingBox box;
  box.expand(glm::vec3(-5.0f, -5.0f, -5.0f));
  box.expand(glm::vec3(5.0f, 5.0f, 5.0f));

  glm::vec3 center = box.center();

  EXPECT_FLOAT_EQ(center.x, 0.0f);
  EXPECT_FLOAT_EQ(center.y, 0.0f);
  EXPECT_FLOAT_EQ(center.z, 0.0f);
}

TEST_F(BoundingBoxTest, CenterOfAsymmetricBox) {
  BoundingBox box;
  box.expand(glm::vec3(0.0f, 0.0f, 0.0f));
  box.expand(glm::vec3(10.0f, 20.0f, 30.0f));

  glm::vec3 center = box.center();

  EXPECT_FLOAT_EQ(center.x, 5.0f);
  EXPECT_FLOAT_EQ(center.y, 10.0f);
  EXPECT_FLOAT_EQ(center.z, 15.0f);
}

TEST_F(BoundingBoxTest, CenterOfZeroSizeBox) {
  BoundingBox box;
  box.expand(glm::vec3(5.0f, 10.0f, 15.0f));

  glm::vec3 center = box.center();

  EXPECT_FLOAT_EQ(center.x, 5.0f);
  EXPECT_FLOAT_EQ(center.y, 10.0f);
  EXPECT_FLOAT_EQ(center.z, 15.0f);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(BoundingBoxTest, SizeOfBox) {
  BoundingBox box;
  box.expand(glm::vec3(0.0f, 0.0f, 0.0f));
  box.expand(glm::vec3(10.0f, 20.0f, 30.0f));

  glm::vec3 size = box.size();

  EXPECT_FLOAT_EQ(size.x, 10.0f);
  EXPECT_FLOAT_EQ(size.y, 20.0f);
  EXPECT_FLOAT_EQ(size.z, 30.0f);
}

TEST_F(BoundingBoxTest, SizeOfZeroSizeBox) {
  BoundingBox box;
  box.expand(glm::vec3(5.0f, 5.0f, 5.0f));

  glm::vec3 size = box.size();

  EXPECT_FLOAT_EQ(size.x, 0.0f);
  EXPECT_FLOAT_EQ(size.y, 0.0f);
  EXPECT_FLOAT_EQ(size.z, 0.0f);
}

TEST_F(BoundingBoxTest, SizeWithNegativeCoordinates) {
  BoundingBox box;
  box.expand(glm::vec3(-10.0f, -5.0f, -2.0f));
  box.expand(glm::vec3(10.0f, 5.0f, 8.0f));

  glm::vec3 size = box.size();

  EXPECT_FLOAT_EQ(size.x, 20.0f);
  EXPECT_FLOAT_EQ(size.y, 10.0f);
  EXPECT_FLOAT_EQ(size.z, 10.0f);
}

// =============================================================================
// Radius Tests
// =============================================================================

TEST_F(BoundingBoxTest, RadiusOfUnitCube) {
  BoundingBox box;
  box.expand(glm::vec3(-0.5f, -0.5f, -0.5f));
  box.expand(glm::vec3(0.5f, 0.5f, 0.5f));

  float radius = box.radius();

  // Diagonal of unit cube is sqrt(3), half of that
  float expected = std::sqrt(3.0f) / 2.0f;
  EXPECT_NEAR(radius, expected, 0.001f);
}

TEST_F(BoundingBoxTest, RadiusOfZeroSizeBox) {
  BoundingBox box;
  box.expand(glm::vec3(0.0f, 0.0f, 0.0f));

  float radius = box.radius();

  EXPECT_FLOAT_EQ(radius, 0.0f);
}

TEST_F(BoundingBoxTest, RadiusOfRectangularBox) {
  BoundingBox box;
  box.expand(glm::vec3(0.0f, 0.0f, 0.0f));
  box.expand(glm::vec3(3.0f, 4.0f, 0.0f)); // 3-4-5 triangle

  float radius = box.radius();

  // Size is (3, 4, 0), length is 5, half is 2.5
  EXPECT_NEAR(radius, 2.5f, 0.001f);
}

// =============================================================================
// Valid Tests
// =============================================================================

TEST_F(BoundingBoxTest, ValidAfterSingleExpand) {
  BoundingBox box;
  EXPECT_FALSE(box.valid());

  box.expand(glm::vec3(0.0f, 0.0f, 0.0f));
  EXPECT_TRUE(box.valid());
}

TEST_F(BoundingBoxTest, ValidWithExactlyEqualMinMax) {
  BoundingBox box;
  box.min = glm::vec3(5.0f, 5.0f, 5.0f);
  box.max = glm::vec3(5.0f, 5.0f, 5.0f);

  EXPECT_TRUE(box.valid());
}

TEST_F(BoundingBoxTest, InvalidWhenMinGreaterThanMax) {
  BoundingBox box;
  box.min = glm::vec3(10.0f, 10.0f, 10.0f);
  box.max = glm::vec3(5.0f, 5.0f, 5.0f);

  EXPECT_FALSE(box.valid());
}

TEST_F(BoundingBoxTest, InvalidWhenPartiallyInverted) {
  BoundingBox box;
  box.min = glm::vec3(0.0f, 10.0f, 0.0f); // Y min > Y max
  box.max = glm::vec3(10.0f, 5.0f, 10.0f);

  EXPECT_FALSE(box.valid());
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(BoundingBoxTest, VeryLargeCoordinates) {
  BoundingBox box;
  box.expand(glm::vec3(-1e6f, -1e6f, -1e6f));
  box.expand(glm::vec3(1e6f, 1e6f, 1e6f));

  EXPECT_TRUE(box.valid());

  glm::vec3 center = box.center();
  EXPECT_NEAR(center.x, 0.0f, 1.0f);
  EXPECT_NEAR(center.y, 0.0f, 1.0f);
  EXPECT_NEAR(center.z, 0.0f, 1.0f);
}

TEST_F(BoundingBoxTest, VerySmallCoordinates) {
  BoundingBox box;
  box.expand(glm::vec3(1e-6f, 1e-6f, 1e-6f));
  box.expand(glm::vec3(2e-6f, 2e-6f, 2e-6f));

  EXPECT_TRUE(box.valid());

  glm::vec3 size = box.size();
  EXPECT_NEAR(size.x, 1e-6f, 1e-9f);
}

TEST_F(BoundingBoxTest, ManyPointExpansions) {
  BoundingBox box;

  // Expand with 1000 random-ish points
  for (int i = 0; i < 1000; ++i) {
    float x = static_cast<float>(i % 100) - 50.0f;
    float y = static_cast<float>(i / 10) - 50.0f;
    float z = static_cast<float>(i % 50) - 25.0f;
    box.expand(glm::vec3(x, y, z));
  }

  EXPECT_TRUE(box.valid());
  EXPECT_LE(box.min.x, -49.0f);
  EXPECT_GE(box.max.x, 49.0f);
}
