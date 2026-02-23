#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "lib/scene/scene_node.hpp"

#include <gtest/gtest.h>

using namespace w3d::scene;
using namespace w3d::gfx;

namespace {

class ConcreteNode : public SceneNode {
public:
  const char *typeName() const override { return "ConcreteNode"; }
};

} // namespace

class SceneNodeTest : public ::testing::Test {
protected:
  ConcreteNode node_;
};

TEST_F(SceneNodeTest, DefaultPositionIsOrigin) {
  EXPECT_FLOAT_EQ(node_.position().x, 0.0f);
  EXPECT_FLOAT_EQ(node_.position().y, 0.0f);
  EXPECT_FLOAT_EQ(node_.position().z, 0.0f);
}

TEST_F(SceneNodeTest, DefaultRotationIsZero) {
  EXPECT_FLOAT_EQ(node_.rotationY(), 0.0f);
}

TEST_F(SceneNodeTest, DefaultScaleIsOne) {
  EXPECT_FLOAT_EQ(node_.scale().x, 1.0f);
  EXPECT_FLOAT_EQ(node_.scale().y, 1.0f);
  EXPECT_FLOAT_EQ(node_.scale().z, 1.0f);
}

TEST_F(SceneNodeTest, DefaultVisibilityIsTrue) {
  EXPECT_TRUE(node_.isVisible());
}

TEST_F(SceneNodeTest, SetPositionUpdatesPosition) {
  node_.setPosition({10.0f, 5.0f, 20.0f});
  EXPECT_FLOAT_EQ(node_.position().x, 10.0f);
  EXPECT_FLOAT_EQ(node_.position().y, 5.0f);
  EXPECT_FLOAT_EQ(node_.position().z, 20.0f);
}

TEST_F(SceneNodeTest, SetRotationYUpdatesRotation) {
  node_.setRotationY(1.57f);
  EXPECT_FLOAT_EQ(node_.rotationY(), 1.57f);
}

TEST_F(SceneNodeTest, SetScaleUpdatesScale) {
  node_.setScale({2.0f, 3.0f, 4.0f});
  EXPECT_FLOAT_EQ(node_.scale().x, 2.0f);
  EXPECT_FLOAT_EQ(node_.scale().y, 3.0f);
  EXPECT_FLOAT_EQ(node_.scale().z, 4.0f);
}

TEST_F(SceneNodeTest, SetVisibleFalse) {
  node_.setVisible(false);
  EXPECT_FALSE(node_.isVisible());
}

TEST_F(SceneNodeTest, WorldTransformIdentityByDefault) {
  glm::mat4 t = node_.worldTransform();
  glm::mat4 identity(1.0f);
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      EXPECT_NEAR(t[col][row], identity[col][row], 1e-5f)
          << "Mismatch at (" << col << "," << row << ")";
    }
  }
}

TEST_F(SceneNodeTest, WorldTransformWithTranslation) {
  node_.setPosition({10.0f, 0.0f, 5.0f});
  glm::mat4 t = node_.worldTransform();
  EXPECT_NEAR(t[3][0], 10.0f, 1e-5f);
  EXPECT_NEAR(t[3][1], 0.0f, 1e-5f);
  EXPECT_NEAR(t[3][2], 5.0f, 1e-5f);
  EXPECT_NEAR(t[3][3], 1.0f, 1e-5f);
}

TEST_F(SceneNodeTest, WorldTransformWithScale) {
  node_.setScale({2.0f, 2.0f, 2.0f});
  glm::mat4 t = node_.worldTransform();
  EXPECT_NEAR(t[0][0], 2.0f, 1e-5f);
  EXPECT_NEAR(t[1][1], 2.0f, 1e-5f);
  EXPECT_NEAR(t[2][2], 2.0f, 1e-5f);
}

TEST_F(SceneNodeTest, WorldTransformWithRotationY90) {
  node_.setRotationY(glm::radians(90.0f));
  glm::mat4 t = node_.worldTransform();

  glm::vec4 xAxis = t * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
  EXPECT_NEAR(xAxis.x, 0.0f, 1e-5f);
  EXPECT_NEAR(xAxis.y, 0.0f, 1e-5f);
  EXPECT_NEAR(xAxis.z, -1.0f, 1e-5f);
}

TEST_F(SceneNodeTest, LocalBoundsDefaultInvalid) {
  EXPECT_FALSE(node_.localBounds().valid());
}

TEST_F(SceneNodeTest, SetLocalBoundsIsStored) {
  BoundingBox b;
  b.expand(glm::vec3(-1.0f, -1.0f, -1.0f));
  b.expand(glm::vec3(1.0f, 1.0f, 1.0f));
  node_.setLocalBounds(b);
  EXPECT_TRUE(node_.localBounds().valid());
}

TEST_F(SceneNodeTest, WorldBoundsInvalidWithoutLocalBounds) {
  EXPECT_FALSE(node_.worldBounds().valid());
}

TEST_F(SceneNodeTest, WorldBoundsWithTranslation) {
  BoundingBox b;
  b.expand(glm::vec3(-1.0f, -1.0f, -1.0f));
  b.expand(glm::vec3(1.0f, 1.0f, 1.0f));
  node_.setLocalBounds(b);
  node_.setPosition({10.0f, 5.0f, 0.0f});

  BoundingBox wb = node_.worldBounds();
  EXPECT_TRUE(wb.valid());
  EXPECT_NEAR(wb.center().x, 10.0f, 0.1f);
  EXPECT_NEAR(wb.center().y, 5.0f, 0.1f);
  EXPECT_NEAR(wb.center().z, 0.0f, 0.1f);
}

TEST_F(SceneNodeTest, TypeNameIsCorrect) {
  EXPECT_STREQ(node_.typeName(), "ConcreteNode");
}
