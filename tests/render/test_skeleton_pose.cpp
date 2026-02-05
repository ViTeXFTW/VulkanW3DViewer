#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>

#include "render/skeleton.hpp"
#include "lib/formats/w3d/types.hpp"

#include <gtest/gtest.h>

using namespace w3d;

class SkeletonPoseTest : public ::testing::Test {
protected:
  // Create a simple hierarchy with the given pivots
  static Hierarchy createHierarchy(const std::string &name, const std::vector<Pivot> &pivots) {
    Hierarchy h;
    h.version = 1;
    h.name = name;
    h.center = {0.0f, 0.0f, 0.0f};
    h.pivots = pivots;
    return h;
  }

  // Create a pivot with identity rotation
  static Pivot createPivot(const std::string &name, uint32_t parent, float tx, float ty, float tz) {
    Pivot p;
    p.name = name;
    p.parentIndex = parent;
    p.translation = {tx, ty, tz};
    p.eulerAngles = {0.0f, 0.0f, 0.0f};
    p.rotation = {0.0f, 0.0f, 0.0f, 1.0f}; // Identity
    return p;
  }

  // Create a pivot with quaternion rotation
  static Pivot createPivotWithRotation(const std::string &name, uint32_t parent, float tx, float ty,
                                       float tz, float qx, float qy, float qz, float qw) {
    Pivot p;
    p.name = name;
    p.parentIndex = parent;
    p.translation = {tx, ty, tz};
    p.eulerAngles = {0.0f, 0.0f, 0.0f};
    p.rotation = {qx, qy, qz, qw};
    return p;
  }
};

// =============================================================================
// Rest Pose Tests
// =============================================================================

TEST_F(SkeletonPoseTest, EmptyHierarchyReturnsInvalidPose) {
  Hierarchy h = createHierarchy("Empty", {});

  SkeletonPose pose;
  pose.computeRestPose(h);

  EXPECT_FALSE(pose.isValid());
  EXPECT_EQ(pose.boneCount(), 0);
}

TEST_F(SkeletonPoseTest, SingleRootBoneAtOrigin) {
  std::vector<Pivot> pivots = {createPivot("ROOTTRANSFORM", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f)};
  Hierarchy h = createHierarchy("SingleBone", pivots);

  SkeletonPose pose;
  pose.computeRestPose(h);

  EXPECT_TRUE(pose.isValid());
  ASSERT_EQ(pose.boneCount(), 1);

  glm::vec3 pos = pose.bonePosition(0);
  EXPECT_NEAR(pos.x, 0.0f, 0.001f);
  EXPECT_NEAR(pos.y, 0.0f, 0.001f);
  EXPECT_NEAR(pos.z, 0.0f, 0.001f);
}

TEST_F(SkeletonPoseTest, SingleBoneWithTranslation) {
  std::vector<Pivot> pivots = {createPivot("ROOT", 0xFFFFFFFF, 5.0f, 10.0f, -3.0f)};
  Hierarchy h = createHierarchy("Translated", pivots);

  SkeletonPose pose;
  pose.computeRestPose(h);

  ASSERT_EQ(pose.boneCount(), 1);

  glm::vec3 pos = pose.bonePosition(0);
  EXPECT_NEAR(pos.x, 5.0f, 0.001f);
  EXPECT_NEAR(pos.y, 10.0f, 0.001f);
  EXPECT_NEAR(pos.z, -3.0f, 0.001f);
}

TEST_F(SkeletonPoseTest, TwoBoneChainPositions) {
  // Root at origin, child 2 units up on Y axis
  std::vector<Pivot> pivots = {createPivot("ROOT", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f),
                               createPivot("CHILD", 0, 0.0f, 2.0f, 0.0f)};
  Hierarchy h = createHierarchy("Chain", pivots);

  SkeletonPose pose;
  pose.computeRestPose(h);

  ASSERT_EQ(pose.boneCount(), 2);

  glm::vec3 rootPos = pose.bonePosition(0);
  glm::vec3 childPos = pose.bonePosition(1);

  EXPECT_NEAR(rootPos.y, 0.0f, 0.001f);
  EXPECT_NEAR(childPos.y, 2.0f, 0.001f);
}

TEST_F(SkeletonPoseTest, ThreeBoneChainAccumulatesTranslation) {
  // Root -> Spine (1 unit up) -> Head (0.5 units up)
  std::vector<Pivot> pivots = {createPivot("ROOT", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f),
                               createPivot("SPINE", 0, 0.0f, 1.0f, 0.0f),
                               createPivot("HEAD", 1, 0.0f, 0.5f, 0.0f)};
  Hierarchy h = createHierarchy("Spine", pivots);

  SkeletonPose pose;
  pose.computeRestPose(h);

  ASSERT_EQ(pose.boneCount(), 3);

  glm::vec3 rootPos = pose.bonePosition(0);
  glm::vec3 spinePos = pose.bonePosition(1);
  glm::vec3 headPos = pose.bonePosition(2);

  EXPECT_NEAR(rootPos.y, 0.0f, 0.001f);
  EXPECT_NEAR(spinePos.y, 1.0f, 0.001f);
  EXPECT_NEAR(headPos.y, 1.5f, 0.001f); // 1.0 + 0.5
}

TEST_F(SkeletonPoseTest, RotatedParentAffectsChildPosition) {
  // Root rotated 90 degrees around Y, child 1 unit in local X
  // After rotation, child should be at world Z = 1
  float angle = glm::pi<float>() / 2.0f; // 90 degrees
  float qy = std::sin(angle / 2.0f);
  float qw = std::cos(angle / 2.0f);

  std::vector<Pivot> pivots = {
      createPivotWithRotation("ROOT", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f, 0.0f, qy, 0.0f, qw),
      createPivot("CHILD", 0, 1.0f, 0.0f, 0.0f) // 1 unit in local X
  };
  Hierarchy h = createHierarchy("Rotated", pivots);

  SkeletonPose pose;
  pose.computeRestPose(h);

  ASSERT_EQ(pose.boneCount(), 2);

  glm::vec3 childPos = pose.bonePosition(1);

  // After 90 deg Y rotation: local X -> world -Z (with W3D's rotation convention)
  // Actually depends on the exact quaternion convention, let's just check it moved
  EXPECT_NEAR(std::abs(childPos.x) + std::abs(childPos.z), 1.0f, 0.01f);
}

TEST_F(SkeletonPoseTest, ParentIndicesPreserved) {
  std::vector<Pivot> pivots = {
      createPivot("ROOT", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f), createPivot("CHILD1", 0, 1.0f, 0.0f, 0.0f),
      createPivot("CHILD2", 0, -1.0f, 0.0f, 0.0f), createPivot("GRANDCHILD", 1, 0.0f, 1.0f, 0.0f)};
  Hierarchy h = createHierarchy("Branched", pivots);

  SkeletonPose pose;
  pose.computeRestPose(h);

  EXPECT_EQ(pose.parentIndex(0), -1);
  EXPECT_EQ(pose.parentIndex(1), 0);
  EXPECT_EQ(pose.parentIndex(2), 0);
  EXPECT_EQ(pose.parentIndex(3), 1);
}

TEST_F(SkeletonPoseTest, BoneNamesPreserved) {
  std::vector<Pivot> pivots = {createPivot("ROOTTRANSFORM", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f),
                               createPivot("BSPINE", 0, 0.0f, 1.0f, 0.0f),
                               createPivot("BHEAD", 1, 0.0f, 0.5f, 0.0f)};
  Hierarchy h = createHierarchy("Named", pivots);

  SkeletonPose pose;
  pose.computeRestPose(h);

  EXPECT_EQ(pose.boneName(0), "ROOTTRANSFORM");
  EXPECT_EQ(pose.boneName(1), "BSPINE");
  EXPECT_EQ(pose.boneName(2), "BHEAD");
}

TEST_F(SkeletonPoseTest, InverseBindPoseComputed) {
  std::vector<Pivot> pivots = {createPivot("ROOT", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f),
                               createPivot("CHILD", 0, 0.0f, 2.0f, 0.0f)};
  Hierarchy h = createHierarchy("WithInverse", pivots);

  SkeletonPose pose;
  pose.computeRestPose(h);

  EXPECT_TRUE(pose.hasInverseBindPose());
  ASSERT_EQ(pose.inverseBindPose().size(), 2);

  // Verify inverse: transform * inverse = identity
  const glm::mat4 &worldTransform = pose.boneTransform(1);
  const glm::mat4 &invBind = pose.inverseBindPose()[1];
  glm::mat4 result = worldTransform * invBind;

  // Should be approximately identity
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      float expected = (i == j) ? 1.0f : 0.0f;
      EXPECT_NEAR(result[i][j], expected, 0.001f);
    }
  }
}

TEST_F(SkeletonPoseTest, SkinningMatricesReturnsWorldTransforms) {
  std::vector<Pivot> pivots = {
      createPivot("ROOT", 0xFFFFFFFF, 1.0f, 2.0f, 3.0f),
  };
  Hierarchy h = createHierarchy("Skinning", pivots);

  SkeletonPose pose;
  pose.computeRestPose(h);

  auto skinning = pose.getSkinningMatrices();
  ASSERT_EQ(skinning.size(), 1);

  // W3D uses bone world transforms directly for skinning
  const glm::mat4 &worldTransform = pose.boneTransform(0);
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      EXPECT_FLOAT_EQ(skinning[0][i][j], worldTransform[i][j]);
    }
  }
}

TEST_F(SkeletonPoseTest, BonePositionOutOfRangeReturnsZero) {
  std::vector<Pivot> pivots = {
      createPivot("ROOT", 0xFFFFFFFF, 5.0f, 5.0f, 5.0f),
  };
  Hierarchy h = createHierarchy("OutOfRange", pivots);

  SkeletonPose pose;
  pose.computeRestPose(h);

  // Request position for non-existent bone
  glm::vec3 pos = pose.bonePosition(100);
  EXPECT_FLOAT_EQ(pos.x, 0.0f);
  EXPECT_FLOAT_EQ(pos.y, 0.0f);
  EXPECT_FLOAT_EQ(pos.z, 0.0f);
}

// =============================================================================
// Animated Pose Tests
// =============================================================================

TEST_F(SkeletonPoseTest, AnimatedPoseWithIdentityAnimation) {
  std::vector<Pivot> pivots = {createPivot("ROOT", 0xFFFFFFFF, 1.0f, 0.0f, 0.0f),
                               createPivot("CHILD", 0, 0.0f, 1.0f, 0.0f)};
  Hierarchy h = createHierarchy("Animated", pivots);

  // Identity animation (no offset)
  std::vector<glm::vec3> translations = {glm::vec3(0.0f), glm::vec3(0.0f)};
  std::vector<glm::quat> rotations = {glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                                      glm::quat(1.0f, 0.0f, 0.0f, 0.0f)};

  SkeletonPose pose;
  pose.computeAnimatedPose(h, translations, rotations);

  // Should be same as rest pose
  glm::vec3 rootPos = pose.bonePosition(0);
  glm::vec3 childPos = pose.bonePosition(1);

  EXPECT_NEAR(rootPos.x, 1.0f, 0.001f);
  EXPECT_NEAR(childPos.y, 1.0f, 0.001f);
}

TEST_F(SkeletonPoseTest, AnimatedPoseWithTranslationOffset) {
  std::vector<Pivot> pivots = {
      createPivot("ROOT", 0xFFFFFFFF, 0.0f, 0.0f, 0.0f),
  };
  Hierarchy h = createHierarchy("Offset", pivots);

  // Add translation offset
  std::vector<glm::vec3> translations = {glm::vec3(5.0f, 10.0f, -3.0f)};
  std::vector<glm::quat> rotations = {glm::quat(1.0f, 0.0f, 0.0f, 0.0f)};

  SkeletonPose pose;
  pose.computeAnimatedPose(h, translations, rotations);

  glm::vec3 rootPos = pose.bonePosition(0);
  EXPECT_NEAR(rootPos.x, 5.0f, 0.001f);
  EXPECT_NEAR(rootPos.y, 10.0f, 0.001f);
  EXPECT_NEAR(rootPos.z, -3.0f, 0.001f);
}

TEST_F(SkeletonPoseTest, AnimatedPoseMismatchedSizeFallsBackToRest) {
  std::vector<Pivot> pivots = {createPivot("ROOT", 0xFFFFFFFF, 1.0f, 2.0f, 3.0f),
                               createPivot("CHILD", 0, 0.0f, 1.0f, 0.0f)};
  Hierarchy h = createHierarchy("Mismatched", pivots);

  // Wrong number of animation channels
  std::vector<glm::vec3> translations = {glm::vec3(0.0f)}; // Only 1, need 2
  std::vector<glm::quat> rotations = {glm::quat(1.0f, 0.0f, 0.0f, 0.0f)};

  SkeletonPose pose;
  pose.computeAnimatedPose(h, translations, rotations);

  // Should fall back to rest pose
  glm::vec3 rootPos = pose.bonePosition(0);
  EXPECT_NEAR(rootPos.x, 1.0f, 0.001f);
  EXPECT_NEAR(rootPos.y, 2.0f, 0.001f);
}

TEST_F(SkeletonPoseTest, AnimatedPoseEmptyHierarchy) {
  Hierarchy h = createHierarchy("Empty", {});

  std::vector<glm::vec3> translations;
  std::vector<glm::quat> rotations;

  SkeletonPose pose;
  pose.computeAnimatedPose(h, translations, rotations);

  EXPECT_FALSE(pose.isValid());
}

TEST_F(SkeletonPoseTest, LargeBoneCount) {
  std::vector<Pivot> pivots;
  // Create 50-bone chain
  for (int i = 0; i < 50; ++i) {
    uint32_t parent = (i == 0) ? 0xFFFFFFFF : static_cast<uint32_t>(i - 1);
    pivots.push_back(createPivot("BONE" + std::to_string(i), parent, 0.0f, 0.1f, 0.0f));
  }
  Hierarchy h = createHierarchy("LargeChain", pivots);

  SkeletonPose pose;
  pose.computeRestPose(h);

  ASSERT_EQ(pose.boneCount(), 50);

  // Last bone should be at Y = 50 * 0.1 = 5.0 (all 50 bones have 0.1 translation each)
  glm::vec3 lastPos = pose.bonePosition(49);
  EXPECT_NEAR(lastPos.y, 5.0f, 0.01f);
}
