#ifndef W3D_RENDER_RAYCAST_HPP
#define W3D_RENDER_RAYCAST_HPP

#include <glm/glm.hpp>

#include <limits>

namespace w3d {

struct Ray {
  glm::vec3 origin;
  glm::vec3 direction; // Must be normalized
};

struct TriangleHit {
  bool hit = false;
  float distance = std::numeric_limits<float>::max();
  glm::vec3 point{0.0f};
  float u = 0.0f; // Barycentric coordinate
  float v = 0.0f; // Barycentric coordinate
};

struct LineHit {
  bool hit = false;
  float distance = std::numeric_limits<float>::max();
  glm::vec3 point{0.0f};
  float t = 0.0f; // Parameter along line segment [0,1]
};

struct SphereHit {
  bool hit = false;
  float distance = std::numeric_limits<float>::max();
  glm::vec3 point{0.0f};
};

// Create ray from screen coordinates
// screenPos: Mouse position (0,0 = top-left corner)
// screenSize: Window dimensions in pixels
// viewMatrix: Camera view matrix
// projMatrix: Camera projection matrix
Ray screenToWorldRay(const glm::vec2 &screenPos, const glm::vec2 &screenSize,
                     const glm::mat4 &viewMatrix, const glm::mat4 &projMatrix);

// Ray-triangle intersection using Möller-Trumbore algorithm
// Returns hit information including barycentric coordinates
TriangleHit intersectRayTriangle(const Ray &ray, const glm::vec3 &v0, const glm::vec3 &v1,
                                 const glm::vec3 &v2);

// Ray-line segment intersection with tolerance for clickability
// tolerance: Click radius around the line segment
LineHit intersectRayLineSegment(const Ray &ray, const glm::vec3 &lineStart,
                                const glm::vec3 &lineEnd, float tolerance = 0.05f);

// Ray-sphere intersection
// Returns closest intersection point (front face)
SphereHit intersectRaySphere(const Ray &ray, const glm::vec3 &center, float radius);

} // namespace w3d

#endif // W3D_RENDER_RAYCAST_HPP
