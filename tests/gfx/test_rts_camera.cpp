#include "lib/gfx/rts_camera.hpp"

#include <gtest/gtest.h>

using namespace w3d::gfx;

TEST(RTSCameraTest, DefaultConstructorSetsReasonableValues) {
  RTSCamera camera;

  EXPECT_EQ(camera.yaw(), 0.0f);
  EXPECT_FLOAT_EQ(camera.pitch(), 1.047f);
  EXPECT_FLOAT_EQ(camera.height(), 50.0f);
}

TEST(RTSCameraTest, CanSetAndGetYaw) {
  RTSCamera camera;

  camera.setYaw(1.5f);

  EXPECT_FLOAT_EQ(camera.yaw(), 1.5f);
}

TEST(RTSCameraTest, PitchClampedToValidRange) {
  RTSCamera camera;

  camera.setPitch(0.0f);
  EXPECT_GT(camera.pitch(), 0.0f);

  camera.setPitch(2.0f);
  EXPECT_LT(camera.pitch(), 2.0f);

  camera.setPitch(1.0f);
  EXPECT_FLOAT_EQ(camera.pitch(), 1.0f);
}

TEST(RTSCameraTest, HeightClampedToValidRange) {
  RTSCamera camera;

  camera.setHeight(1.0f);
  EXPECT_GE(camera.height(), 5.0f);

  camera.setHeight(1000.0f);
  EXPECT_LE(camera.height(), 500.0f);

  camera.setHeight(50.0f);
  EXPECT_FLOAT_EQ(camera.height(), 50.0f);
}

TEST(RTSCameraTest, PositionReflectsHeight) {
  RTSCamera camera;

  camera.setPosition(glm::vec3(10.0f, 0.0f, 20.0f));
  camera.setHeight(75.0f);

  auto pos = camera.position();

  EXPECT_FLOAT_EQ(pos.y, 75.0f);

  EXPECT_GT(pos.x, 0.0f);
  EXPECT_LT(pos.z, 100.0f);
}

TEST(RTSCameraTest, ViewMatrixIsNotIdentity) {
  RTSCamera camera;

  auto viewMat = camera.viewMatrix();

  bool isIdentity = true;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      float expected = (i == j) ? 1.0f : 0.0f;
      if (std::abs(viewMat[i][j] - expected) > 0.01f) {
        isIdentity = false;
        break;
      }
    }
  }

  EXPECT_FALSE(isIdentity);
}

TEST(RTSCameraTest, CanSetMovementSpeed) {
  RTSCamera camera;

  camera.setMovementSpeed(100.0f);

  EXPECT_FLOAT_EQ(camera.movementSpeed(), 100.0f);
}

TEST(RTSCameraTest, CanSetRotationSpeed) {
  RTSCamera camera;

  camera.setRotationSpeed(2.0f);

  EXPECT_FLOAT_EQ(camera.rotationSpeed(), 2.0f);
}

TEST(RTSCameraTest, CanSetZoomSpeed) {
  RTSCamera camera;

  camera.setZoomSpeed(15.0f);

  EXPECT_FLOAT_EQ(camera.zoomSpeed(), 15.0f);
}
