#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "render/hover_detector.hpp"
#include "render/raycast.hpp"

#include <gtest/gtest.h>

using namespace w3d;

// Helper to check vector equality
bool vecApproxEqual(const glm::vec3 &a, const glm::vec3 &b, float epsilon = 1e-5f) {
  return glm::length(a - b) < epsilon;
}

// =============================================================================
// HoverNameDisplayMode and HoverState::displayName tests
// =============================================================================

TEST(HoverStateTest, DisplayNameFullName) {
  HoverState state;
  state.objectName = "SoldierBody_sub0";
  state.baseName = "SoldierBody";
  state.subMeshIndex = 0;
  state.subMeshTotal = 3;

  EXPECT_EQ(state.displayName(HoverNameDisplayMode::FullName), "SoldierBody_sub0");
}

TEST(HoverStateTest, DisplayNameBaseName) {
  HoverState state;
  state.objectName = "SoldierBody_sub0";
  state.baseName = "SoldierBody";
  state.subMeshIndex = 0;
  state.subMeshTotal = 3;

  EXPECT_EQ(state.displayName(HoverNameDisplayMode::BaseName), "SoldierBody");
}

TEST(HoverStateTest, DisplayNameDescriptive) {
  HoverState state;
  state.objectName = "SoldierBody_sub0";
  state.baseName = "SoldierBody";
  state.subMeshIndex = 0;
  state.subMeshTotal = 3;

  EXPECT_EQ(state.displayName(HoverNameDisplayMode::Descriptive), "SoldierBody (part 1 of 3)");
}

TEST(HoverStateTest, DisplayNameSingleSubMesh) {
  HoverState state;
  state.objectName = "SimpleBox";
  state.baseName = "SimpleBox";
  state.subMeshIndex = 0;
  state.subMeshTotal = 1;

  // For single sub-mesh, all modes should return just the name
  EXPECT_EQ(state.displayName(HoverNameDisplayMode::FullName), "SimpleBox");
  EXPECT_EQ(state.displayName(HoverNameDisplayMode::BaseName), "SimpleBox");
  EXPECT_EQ(state.displayName(HoverNameDisplayMode::Descriptive), "SimpleBox");
}

TEST(HoverStateTest, DisplayNameEmptyBaseName) {
  HoverState state;
  state.objectName = "UnknownMesh_sub2";
  state.baseName = ""; // Empty base name
  state.subMeshIndex = 2;
  state.subMeshTotal = 5;

  // Should fall back to objectName for BaseName and Descriptive modes
  EXPECT_EQ(state.displayName(HoverNameDisplayMode::FullName), "UnknownMesh_sub2");
  EXPECT_EQ(state.displayName(HoverNameDisplayMode::BaseName), "UnknownMesh_sub2");
  EXPECT_EQ(state.displayName(HoverNameDisplayMode::Descriptive), "UnknownMesh_sub2");
}

// =============================================================================
// Ray-to-bone-space transformation tests
// =============================================================================

TEST(RayBoneTransformTest, IdentityTransform) {
  Ray worldRay{glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};
  glm::mat4 identity(1.0f);

  Ray localRay = transformRayToBoneSpace(worldRay, identity);

  EXPECT_TRUE(vecApproxEqual(localRay.origin, worldRay.origin));
  EXPECT_TRUE(vecApproxEqual(localRay.direction, worldRay.direction));
}

TEST(RayBoneTransformTest, TranslatedBone) {
  // Ray from origin along +Z
  Ray worldRay{glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

  // Bone translated by (5, 0, 0)
  glm::mat4 boneTransform = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f));

  Ray localRay = transformRayToBoneSpace(worldRay, boneTransform);

  // In bone-local space, the ray origin should be at (-5, 0, 0)
  EXPECT_TRUE(vecApproxEqual(localRay.origin, glm::vec3(-5.0f, 0.0f, 0.0f)));
  // Direction should be unchanged (translation doesn't affect direction)
  EXPECT_TRUE(vecApproxEqual(localRay.direction, glm::vec3(0.0f, 0.0f, 1.0f)));
}

TEST(RayBoneTransformTest, RotatedBone90DegreesY) {
  // Ray from origin along +Z
  Ray worldRay{glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

  // Bone rotated 90 degrees around Y axis
  glm::mat4 boneTransform =
      glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

  Ray localRay = transformRayToBoneSpace(worldRay, boneTransform);

  // Origin stays at (0,0,0)
  EXPECT_TRUE(vecApproxEqual(localRay.origin, glm::vec3(0.0f, 0.0f, 0.0f)));
  // Direction should be rotated: +Z world -> -X local (inverse of 90 deg Y rotation)
  EXPECT_TRUE(vecApproxEqual(localRay.direction, glm::vec3(-1.0f, 0.0f, 0.0f), 1e-4f));
}

TEST(RayBoneTransformTest, CombinedTransform) {
  // Ray from (1, 2, 3) along +X
  Ray worldRay{glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(1.0f, 0.0f, 0.0f)};

  // Bone at (10, 0, 0), rotated 90 degrees around Z
  glm::mat4 boneTransform = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f));
  boneTransform = glm::rotate(boneTransform, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

  Ray localRay = transformRayToBoneSpace(worldRay, boneTransform);

  // Ray should be normalized
  EXPECT_NEAR(glm::length(localRay.direction), 1.0f, 1e-5f);
}

TEST(RayBoneTransformTest, ScaledBone) {
  // Ray from (2, 0, 0) along +Z
  Ray worldRay{glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

  // Bone scaled by 2x
  glm::mat4 boneTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));

  Ray localRay = transformRayToBoneSpace(worldRay, boneTransform);

  // Origin should be at (1, 0, 0) in local space (scaled down)
  EXPECT_TRUE(vecApproxEqual(localRay.origin, glm::vec3(1.0f, 0.0f, 0.0f)));
  // Direction should still be normalized
  EXPECT_NEAR(glm::length(localRay.direction), 1.0f, 1e-5f);
}

// =============================================================================
// Visible mesh filtering tests (will be implemented in HLodModel)
// These are placeholder tests that will verify the visibleMeshIndices() behavior
// =============================================================================

// Note: Full HLodModel tests require VulkanContext, so we test the logic separately
// The actual integration will be tested manually or with integration tests

TEST(HLodVisibilityTest, AggregatesAlwaysVisible) {
  // This is a conceptual test - aggregates should always be included
  // Implementation will verify that:
  // - Mesh indices 0 to aggregateCount-1 are always in visibleMeshIndices()
  SUCCEED(); // Placeholder - actual test needs mock HLodModel
}

TEST(HLodVisibilityTest, OnlyCurrentLODMeshesVisible) {
  // Meshes with lodLevel != currentLOD should not be in visibleMeshIndices()
  SUCCEED(); // Placeholder - actual test needs mock HLodModel
}
