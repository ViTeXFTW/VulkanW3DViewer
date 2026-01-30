#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

#include "render/raycast.hpp"

#include <gtest/gtest.h>

using namespace w3d;

// Helper function to check if two vectors are approximately equal
bool vectorsApproxEqual(const glm::vec3 &a, const glm::vec3 &b, float epsilon = 1e-5f) {
  return glm::length(a - b) < epsilon;
}

// Test ray-triangle intersection
TEST(RaycastTest, TriangleIntersectionFrontFace) {
  // Triangle in XY plane at Z=1
  glm::vec3 v0(0.0f, 0.0f, 1.0f);
  glm::vec3 v1(1.0f, 0.0f, 1.0f);
  glm::vec3 v2(0.0f, 1.0f, 1.0f);

  // Ray from origin along +Z axis
  Ray ray{glm::vec3(0.25f, 0.25f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

  TriangleHit hit = intersectRayTriangle(ray, v0, v1, v2);

  EXPECT_TRUE(hit.hit);
  EXPECT_FLOAT_EQ(hit.distance, 1.0f);
  EXPECT_TRUE(vectorsApproxEqual(hit.point, glm::vec3(0.25f, 0.25f, 1.0f)));
  EXPECT_FLOAT_EQ(hit.u, 0.25f);
  EXPECT_FLOAT_EQ(hit.v, 0.25f);
}

TEST(RaycastTest, TriangleIntersectionMiss) {
  // Triangle in XY plane at Z=1
  glm::vec3 v0(0.0f, 0.0f, 1.0f);
  glm::vec3 v1(1.0f, 0.0f, 1.0f);
  glm::vec3 v2(0.0f, 1.0f, 1.0f);

  // Ray from origin along +Z axis, but outside triangle bounds
  Ray ray{glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

  TriangleHit hit = intersectRayTriangle(ray, v0, v1, v2);

  EXPECT_FALSE(hit.hit);
}

TEST(RaycastTest, TriangleIntersectionBehindRay) {
  // Triangle in XY plane at Z=1
  glm::vec3 v0(0.0f, 0.0f, 1.0f);
  glm::vec3 v1(1.0f, 0.0f, 1.0f);
  glm::vec3 v2(0.0f, 1.0f, 1.0f);

  // Ray pointing away from triangle
  Ray ray{glm::vec3(0.25f, 0.25f, 2.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

  TriangleHit hit = intersectRayTriangle(ray, v0, v1, v2);

  EXPECT_FALSE(hit.hit);
}

TEST(RaycastTest, TriangleIntersectionParallel) {
  // Triangle in XY plane at Z=1
  glm::vec3 v0(0.0f, 0.0f, 1.0f);
  glm::vec3 v1(1.0f, 0.0f, 1.0f);
  glm::vec3 v2(0.0f, 1.0f, 1.0f);

  // Ray parallel to triangle plane
  Ray ray{glm::vec3(0.25f, 0.25f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)};

  TriangleHit hit = intersectRayTriangle(ray, v0, v1, v2);

  EXPECT_FALSE(hit.hit);
}

TEST(RaycastTest, TriangleIntersectionEdgeCase) {
  // Triangle in XY plane at Z=1
  glm::vec3 v0(0.0f, 0.0f, 1.0f);
  glm::vec3 v1(1.0f, 0.0f, 1.0f);
  glm::vec3 v2(0.0f, 1.0f, 1.0f);

  // Ray hitting triangle edge
  Ray ray{glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

  TriangleHit hit = intersectRayTriangle(ray, v0, v1, v2);

  EXPECT_TRUE(hit.hit);
  EXPECT_FLOAT_EQ(hit.distance, 1.0f);
}

// Test ray-sphere intersection
TEST(RaycastTest, SphereIntersectionHit) {
  // Sphere at origin with radius 1
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  float radius = 1.0f;

  // Ray from (-2, 0, 0) along +X axis
  Ray ray{glm::vec3(-2.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)};

  SphereHit hit = intersectRaySphere(ray, center, radius);

  EXPECT_TRUE(hit.hit);
  EXPECT_FLOAT_EQ(hit.distance, 1.0f);
  EXPECT_TRUE(vectorsApproxEqual(hit.point, glm::vec3(-1.0f, 0.0f, 0.0f)));
}

TEST(RaycastTest, SphereIntersectionMiss) {
  // Sphere at origin with radius 1
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  float radius = 1.0f;

  // Ray that misses the sphere
  Ray ray{glm::vec3(-2.0f, 2.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)};

  SphereHit hit = intersectRaySphere(ray, center, radius);

  EXPECT_FALSE(hit.hit);
}

TEST(RaycastTest, SphereIntersectionFromInside) {
  // Sphere at origin with radius 1
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  float radius = 1.0f;

  // Ray from inside sphere
  Ray ray{glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)};

  SphereHit hit = intersectRaySphere(ray, center, radius);

  EXPECT_TRUE(hit.hit);
  EXPECT_FLOAT_EQ(hit.distance, 1.0f);
  EXPECT_TRUE(vectorsApproxEqual(hit.point, glm::vec3(1.0f, 0.0f, 0.0f)));
}

// Test ray-line segment intersection
TEST(RaycastTest, LineSegmentIntersectionHit) {
  // Line segment from (0, 0, 1) to (1, 0, 1)
  glm::vec3 lineStart(0.0f, 0.0f, 1.0f);
  glm::vec3 lineEnd(1.0f, 0.0f, 1.0f);

  // Ray from (0.5, 0, 0) along +Z axis
  Ray ray{glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

  LineHit hit = intersectRayLineSegment(ray, lineStart, lineEnd, 0.1f);

  EXPECT_TRUE(hit.hit);
  EXPECT_FLOAT_EQ(hit.distance, 1.0f);
  EXPECT_TRUE(vectorsApproxEqual(hit.point, glm::vec3(0.5f, 0.0f, 1.0f)));
  EXPECT_NEAR(hit.t, 0.5f, 1e-5f);
}

TEST(RaycastTest, LineSegmentIntersectionNearMiss) {
  // Line segment from (0, 0, 1) to (1, 0, 1)
  glm::vec3 lineStart(0.0f, 0.0f, 1.0f);
  glm::vec3 lineEnd(1.0f, 0.0f, 1.0f);

  // Ray slightly offset from line (within tolerance)
  Ray ray{glm::vec3(0.5f, 0.02f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

  LineHit hit = intersectRayLineSegment(ray, lineStart, lineEnd, 0.05f);

  EXPECT_TRUE(hit.hit);
}

TEST(RaycastTest, LineSegmentIntersectionFarMiss) {
  // Line segment from (0, 0, 1) to (1, 0, 1)
  glm::vec3 lineStart(0.0f, 0.0f, 1.0f);
  glm::vec3 lineEnd(1.0f, 0.0f, 1.0f);

  // Ray too far from line
  Ray ray{glm::vec3(0.5f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

  LineHit hit = intersectRayLineSegment(ray, lineStart, lineEnd, 0.05f);

  EXPECT_FALSE(hit.hit);
}

TEST(RaycastTest, LineSegmentIntersectionAtEndpoint) {
  // Line segment from (0, 0, 1) to (1, 0, 1)
  glm::vec3 lineStart(0.0f, 0.0f, 1.0f);
  glm::vec3 lineEnd(1.0f, 0.0f, 1.0f);

  // Ray hitting near the start point
  Ray ray{glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

  LineHit hit = intersectRayLineSegment(ray, lineStart, lineEnd, 0.1f);

  EXPECT_TRUE(hit.hit);
  EXPECT_NEAR(hit.t, 0.0f, 1e-3f);
}

// Test screen to world ray conversion
TEST(RaycastTest, ScreenToWorldRay) {
  // Setup simple camera
  glm::vec2 screenSize(800.0f, 600.0f);
  glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), // Eye position
                               glm::vec3(0.0f, 0.0f, 0.0f), // Look at origin
                               glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
  );
  glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

  // Center of screen should produce ray along -Z axis
  glm::vec2 screenCenter(400.0f, 300.0f);
  Ray ray = screenToWorldRay(screenCenter, screenSize, view, proj);

  // Ray should be pointing roughly along -Z
  EXPECT_TRUE(vectorsApproxEqual(ray.direction, glm::vec3(0.0f, 0.0f, -1.0f), 0.01f));

  // Ray origin should be near camera position
  EXPECT_TRUE(vectorsApproxEqual(ray.origin, glm::vec3(0.0f, 0.0f, 5.0f), 0.5f));
}

TEST(RaycastTest, ScreenToWorldRayCorner) {
  // Setup simple camera
  glm::vec2 screenSize(800.0f, 600.0f);
  glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

  // Top-left corner
  glm::vec2 screenTopLeft(0.0f, 0.0f);
  Ray ray = screenToWorldRay(screenTopLeft, screenSize, view, proj);

  // Ray should be normalized
  EXPECT_NEAR(glm::length(ray.direction), 1.0f, 1e-5f);

  // Ray should point up and to the left (negative X, positive Y)
  EXPECT_LT(ray.direction.x, 0.0f);
  EXPECT_GT(ray.direction.y, 0.0f);
  EXPECT_LT(ray.direction.z, 0.0f);
}
