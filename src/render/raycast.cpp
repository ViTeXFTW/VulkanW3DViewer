#include "raycast.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace w3d {

Ray screenToWorldRay(
  const glm::vec2 &screenPos,
  const glm::vec2 &screenSize,
  const glm::mat4 &viewMatrix,
  const glm::mat4 &projMatrix
) {
  // Convert screen coordinates to normalized device coordinates (NDC)
  // Screen space: (0,0) = top-left, (width, height) = bottom-right
  // Vulkan NDC: (-1,-1) = top-left, (1,1) = bottom-right (Y-down after proj flip)
  // Note: projMatrix should already have the Vulkan Y-flip applied (proj[1][1] *= -1)
  float x = (2.0f * screenPos.x) / screenSize.x - 1.0f;
  float y = (2.0f * screenPos.y) / screenSize.y - 1.0f;  // No Y-flip (Vulkan uses Y-down)

  // NDC coordinates (at near and far plane)
  glm::vec4 rayNDC_near(x, y, 0.0f, 1.0f);  // Near plane in Vulkan NDC (depth = 0)
  glm::vec4 rayNDC_far(x, y, 1.0f, 1.0f);   // Far plane in Vulkan NDC (depth = 1)

  // Transform to view space
  glm::mat4 invProj = glm::inverse(projMatrix);
  glm::vec4 rayView_near = invProj * rayNDC_near;
  glm::vec4 rayView_far = invProj * rayNDC_far;

  // Perspective divide
  rayView_near /= rayView_near.w;
  rayView_far /= rayView_far.w;

  // Transform to world space
  glm::mat4 invView = glm::inverse(viewMatrix);
  glm::vec4 rayWorld_near = invView * rayView_near;
  glm::vec4 rayWorld_far = invView * rayView_far;

  // Construct ray
  glm::vec3 rayOrigin(rayWorld_near);
  glm::vec3 rayEnd(rayWorld_far);
  glm::vec3 rayDirection = glm::normalize(rayEnd - rayOrigin);

  return Ray{rayOrigin, rayDirection};
}

TriangleHit intersectRayTriangle(
  const Ray &ray,
  const glm::vec3 &v0,
  const glm::vec3 &v1,
  const glm::vec3 &v2
) {
  const float EPSILON = 1e-8f;

  // Möller-Trumbore algorithm
  glm::vec3 edge1 = v1 - v0;
  glm::vec3 edge2 = v2 - v0;
  glm::vec3 h = glm::cross(ray.direction, edge2);
  float a = glm::dot(edge1, h);

  // Ray is parallel to triangle
  if (std::abs(a) < EPSILON) {
    return TriangleHit{};
  }

  float f = 1.0f / a;
  glm::vec3 s = ray.origin - v0;
  float u = f * glm::dot(s, h);

  // Intersection point is outside triangle
  if (u < 0.0f || u > 1.0f) {
    return TriangleHit{};
  }

  glm::vec3 q = glm::cross(s, edge1);
  float v = f * glm::dot(ray.direction, q);

  // Intersection point is outside triangle
  if (v < 0.0f || u + v > 1.0f) {
    return TriangleHit{};
  }

  // Compute distance along ray
  float t = f * glm::dot(edge2, q);

  // Intersection is behind ray origin
  if (t < EPSILON) {
    return TriangleHit{};
  }

  // Valid hit
  TriangleHit hit;
  hit.hit = true;
  hit.distance = t;
  hit.point = ray.origin + ray.direction * t;
  hit.u = u;
  hit.v = v;

  return hit;
}

LineHit intersectRayLineSegment(
  const Ray &ray,
  const glm::vec3 &lineStart,
  const glm::vec3 &lineEnd,
  float tolerance
) {
  const float EPSILON = 1e-8f;

  // Algorithm: Find closest points between ray and line segment
  // Then check if distance is within tolerance

  glm::vec3 lineDir = lineEnd - lineStart;
  float lineLength = glm::length(lineDir);

  if (lineLength < EPSILON) {
    // Degenerate line segment (point)
    // Treat as sphere intersection
    glm::vec3 toPoint = lineStart - ray.origin;
    float t = glm::dot(toPoint, ray.direction);

    if (t < 0.0f) {
      return LineHit{};  // Point is behind ray origin
    }

    glm::vec3 closestPointOnRay = ray.origin + ray.direction * t;
    float dist = glm::length(closestPointOnRay - lineStart);

    if (dist <= tolerance) {
      LineHit hit;
      hit.hit = true;
      hit.distance = t;
      hit.point = lineStart;
      hit.t = 0.0f;
      return hit;
    }

    return LineHit{};
  }

  lineDir /= lineLength;  // Normalize

  // Closest point on ray to line segment
  glm::vec3 w0 = ray.origin - lineStart;
  float a = glm::dot(ray.direction, ray.direction);  // Always 1 for normalized ray
  float b = glm::dot(ray.direction, lineDir);
  float c = glm::dot(lineDir, lineDir);              // Always 1 for normalized line
  float d = glm::dot(ray.direction, w0);
  float e = glm::dot(lineDir, w0);

  float denom = a * c - b * b;  // Should be close to 1 - cos^2(angle)

  float sc, tc;  // Parameters along ray and line

  if (std::abs(denom) < EPSILON) {
    // Ray and line are parallel
    // Find closest point on line to ray origin
    sc = 0.0f;
    tc = (b > c ? d / b : e / c);
  } else {
    sc = (b * e - c * d) / denom;
    tc = (a * e - b * d) / denom;
  }

  // Clamp line parameter to segment
  tc = std::clamp(tc / lineLength, 0.0f, 1.0f);

  // Recalculate ray parameter for clamped line point
  glm::vec3 pointOnLine = lineStart + lineDir * (tc * lineLength);
  glm::vec3 toPoint = pointOnLine - ray.origin;
  sc = glm::dot(toPoint, ray.direction);

  // Check if point is behind ray
  if (sc < 0.0f) {
    return LineHit{};
  }

  // Calculate distance between closest points
  glm::vec3 pointOnRay = ray.origin + ray.direction * sc;
  float dist = glm::length(pointOnRay - pointOnLine);

  if (dist <= tolerance) {
    LineHit hit;
    hit.hit = true;
    hit.distance = sc;
    hit.point = pointOnLine;
    hit.t = tc;
    return hit;
  }

  return LineHit{};
}

SphereHit intersectRaySphere(
  const Ray &ray,
  const glm::vec3 &center,
  float radius
) {
  const float EPSILON = 1e-8f;

  glm::vec3 oc = ray.origin - center;
  float a = glm::dot(ray.direction, ray.direction);  // Should be 1 for normalized ray
  float b = 2.0f * glm::dot(oc, ray.direction);
  float c = glm::dot(oc, oc) - radius * radius;

  float discriminant = b * b - 4.0f * a * c;

  if (discriminant < 0.0f) {
    return SphereHit{};  // No intersection
  }

  // Find nearest intersection (front face)
  float t = (-b - std::sqrt(discriminant)) / (2.0f * a);

  // If front face is behind ray, try back face
  if (t < EPSILON) {
    t = (-b + std::sqrt(discriminant)) / (2.0f * a);
  }

  // Both intersections are behind ray
  if (t < EPSILON) {
    return SphereHit{};
  }

  SphereHit hit;
  hit.hit = true;
  hit.distance = t;
  hit.point = ray.origin + ray.direction * t;

  return hit;
}

}  // namespace w3d
