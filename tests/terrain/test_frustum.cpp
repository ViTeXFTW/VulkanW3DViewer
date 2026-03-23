#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "lib/gfx/frustum.hpp"

#include <gtest/gtest.h>

using namespace w3d::gfx;

class FrustumTest : public ::testing::Test {
protected:
  void SetUp() override {
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

    vp_ = proj * view;
    frustum_.extractFromVP(vp_);
  }

  Frustum frustum_;
  glm::mat4 vp_;
};

TEST_F(FrustumTest, BoxAtOriginIsVisible) {
  BoundingBox box;
  box.expand(glm::vec3(-1.0f, -1.0f, -1.0f));
  box.expand(glm::vec3(1.0f, 1.0f, 1.0f));
  EXPECT_TRUE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, BoxDirectlyInFrontIsVisible) {
  BoundingBox box;
  box.expand(glm::vec3(-0.5f, -0.5f, -2.0f));
  box.expand(glm::vec3(0.5f, 0.5f, -1.0f));
  EXPECT_TRUE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, BoxFarBehindCameraIsNotVisible) {
  BoundingBox box;
  box.expand(glm::vec3(-1.0f, -1.0f, 10.0f));
  box.expand(glm::vec3(1.0f, 1.0f, 12.0f));
  EXPECT_FALSE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, BoxFarLeftIsNotVisible) {
  BoundingBox box;
  box.expand(glm::vec3(-100.0f, -1.0f, 0.0f));
  box.expand(glm::vec3(-90.0f, 1.0f, 1.0f));
  EXPECT_FALSE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, BoxFarRightIsNotVisible) {
  BoundingBox box;
  box.expand(glm::vec3(90.0f, -1.0f, 0.0f));
  box.expand(glm::vec3(100.0f, 1.0f, 1.0f));
  EXPECT_FALSE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, BoxFarAboveIsNotVisible) {
  BoundingBox box;
  box.expand(glm::vec3(-1.0f, 90.0f, 0.0f));
  box.expand(glm::vec3(1.0f, 100.0f, 1.0f));
  EXPECT_FALSE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, BoxFarBelowIsNotVisible) {
  BoundingBox box;
  box.expand(glm::vec3(-1.0f, -100.0f, 0.0f));
  box.expand(glm::vec3(1.0f, -90.0f, 1.0f));
  EXPECT_FALSE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, BoxBeyondFarPlaneIsNotVisible) {
  BoundingBox box;
  box.expand(glm::vec3(-1.0f, -1.0f, -200.0f));
  box.expand(glm::vec3(1.0f, 1.0f, -150.0f));
  EXPECT_FALSE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, BoxBeforeNearPlaneIsNotVisible) {
  // Camera at z=5, looking at -z. Near plane is at z=4.9. Box is at z=6 (behind camera).
  BoundingBox box;
  box.expand(glm::vec3(-0.1f, -0.1f, 5.5f));
  box.expand(glm::vec3(0.1f, 0.1f, 6.0f));
  EXPECT_FALSE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, LargeBoxEnclosingFrustumIsVisible) {
  BoundingBox box;
  box.expand(glm::vec3(-500.0f, -500.0f, -500.0f));
  box.expand(glm::vec3(500.0f, 500.0f, 500.0f));
  EXPECT_TRUE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, BoxPartiallyInsideIsVisible) {
  BoundingBox box;
  box.expand(glm::vec3(-50.0f, -1.0f, -1.0f));
  box.expand(glm::vec3(1.0f, 1.0f, 1.0f));
  EXPECT_TRUE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, InvalidBoxIsNotVisible) {
  BoundingBox box;
  EXPECT_FALSE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, ZeroSizeBoxInFrustumIsVisible) {
  BoundingBox box;
  box.expand(glm::vec3(0.0f, 0.0f, 0.0f));
  EXPECT_TRUE(frustum_.isBoxVisible(box));
}

TEST_F(FrustumTest, PlaneNormalsAreNormalized) {
  for (int i = 0; i < Frustum::Count; ++i) {
    float len = glm::length(frustum_.plane(i).normal);
    EXPECT_NEAR(len, 1.0f, 0.001f) << "Plane " << i << " normal is not normalized";
  }
}

TEST_F(FrustumTest, RTSCameraViewFrustum) {
  glm::vec3 cameraPos(500.0f, 100.0f, 500.0f);
  glm::vec3 target(500.0f, 0.0f, 400.0f);
  glm::mat4 view = glm::lookAt(cameraPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 proj = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 2000.0f);

  Frustum rtsFrustum;
  rtsFrustum.extractFromVP(proj * view);

  BoundingBox nearChunk;
  nearChunk.expand(glm::vec3(480.0f, 0.0f, 380.0f));
  nearChunk.expand(glm::vec3(520.0f, 50.0f, 420.0f));
  EXPECT_TRUE(rtsFrustum.isBoxVisible(nearChunk));

  BoundingBox farChunk;
  farChunk.expand(glm::vec3(-1000.0f, 0.0f, -1000.0f));
  farChunk.expand(glm::vec3(-900.0f, 50.0f, -900.0f));
  EXPECT_FALSE(rtsFrustum.isBoxVisible(farChunk));
}

TEST_F(FrustumTest, TerrainChunkGrid) {
  glm::vec3 cameraPos(160.0f, 80.0f, 160.0f);
  glm::vec3 target(160.0f, 0.0f, 80.0f);
  glm::mat4 view = glm::lookAt(cameraPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 1000.0f);

  Frustum terrainFrustum;
  terrainFrustum.extractFromVP(proj * view);

  int visibleCount = 0;
  int totalCount = 0;

  for (int cy = 0; cy < 10; ++cy) {
    for (int cx = 0; cx < 10; ++cx) {
      BoundingBox chunk;
      float x0 = static_cast<float>(cx * 320);
      float z0 = static_cast<float>(cy * 320);
      chunk.expand(glm::vec3(x0, 0.0f, z0));
      chunk.expand(glm::vec3(x0 + 320.0f, 50.0f, z0 + 320.0f));

      if (terrainFrustum.isBoxVisible(chunk)) {
        ++visibleCount;
      }
      ++totalCount;
    }
  }

  EXPECT_GT(visibleCount, 0);
  EXPECT_LT(visibleCount, totalCount);
}

TEST_F(FrustumTest, OrthographicProjection) {
  glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, -1.0f));
  glm::mat4 proj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);

  Frustum orthoFrustum;
  orthoFrustum.extractFromVP(proj * view);

  BoundingBox inside;
  inside.expand(glm::vec3(-5.0f, -5.0f, -5.0f));
  inside.expand(glm::vec3(5.0f, 5.0f, 5.0f));
  EXPECT_TRUE(orthoFrustum.isBoxVisible(inside));

  BoundingBox outside;
  outside.expand(glm::vec3(20.0f, 0.0f, 0.0f));
  outside.expand(glm::vec3(30.0f, 1.0f, 1.0f));
  EXPECT_FALSE(orthoFrustum.isBoxVisible(outside));
}
