#include "hover_detector.hpp"

#include <algorithm>

#include "renderable_mesh.hpp"
#include "skeleton_renderer.hpp"

namespace w3d {

void HoverDetector::update(const glm::vec2 &mousePos, const glm::vec2 &screenSize,
                           const glm::mat4 &viewMatrix, const glm::mat4 &projMatrix) {
  // Reset state
  state_.reset();

  // Generate ray from screen position
  currentRay_ = screenToWorldRay(mousePos, screenSize, viewMatrix, projMatrix);
}

void HoverDetector::testMeshes(const RenderableMesh &meshes) {
  if (!meshes.hasData()) {
    return;
  }

  float closestMeshDist = std::numeric_limits<float>::max();
  size_t closestMeshIndex = 0;
  size_t closestTriIndex = 0;
  glm::vec3 closestMeshPoint(0.0f);

  // Test all meshes and all triangles
  for (size_t meshIdx = 0; meshIdx < meshes.meshCount(); ++meshIdx) {
    size_t triCount = meshes.triangleCount(meshIdx);

    for (size_t triIdx = 0; triIdx < triCount; ++triIdx) {
      glm::vec3 v0, v1, v2;
      if (!meshes.getTriangle(meshIdx, triIdx, v0, v1, v2)) {
        continue;
      }

      TriangleHit hit = intersectRayTriangle(currentRay_, v0, v1, v2);

      if (hit.hit && hit.distance < closestMeshDist) {
        closestMeshDist = hit.distance;
        closestMeshIndex = meshIdx;
        closestTriIndex = triIdx;
        closestMeshPoint = hit.point;
      }
    }
  }

  // Update state if we found a mesh hit closer than current state
  // Note: Skeleton takes priority, so only update if:
  // 1. We're not currently hovering over skeleton, OR
  // 2. Mesh is closer AND we haven't tested skeleton yet (handled by caller)
  if (closestMeshDist < state_.distance) {
    state_.type = HoverType::Mesh;
    state_.objectIndex = closestMeshIndex;
    state_.triangleIndex = closestTriIndex;
    state_.hitPoint = closestMeshPoint;
    state_.distance = closestMeshDist;
    state_.objectName = meshes.meshName(closestMeshIndex);
  }
}

void HoverDetector::testSkeleton(const SkeletonRenderer &skeleton, float boneThickness) {
  if (!skeleton.hasData()) {
    return;
  }

  float closestSkeletonDist = std::numeric_limits<float>::max();
  HoverType closestSkeletonType = HoverType::None;
  size_t closestSkeletonIndex = 0;
  glm::vec3 closestSkeletonPoint(0.0f);

  // Test joints first (spheres)
  for (size_t i = 0; i < skeleton.jointCount(); ++i) {
    glm::vec3 center;
    float radius;

    if (!skeleton.getJointSphere(i, center, radius)) {
      continue;
    }

    SphereHit hit = intersectRaySphere(currentRay_, center, radius);

    if (hit.hit && hit.distance < closestSkeletonDist) {
      closestSkeletonDist = hit.distance;
      closestSkeletonType = HoverType::Joint;
      closestSkeletonIndex = i;
      closestSkeletonPoint = hit.point;
    }
  }

  // Test bones (line segments)
  for (size_t i = 0; i < skeleton.boneCount(); ++i) {
    glm::vec3 start, end;

    if (!skeleton.getBoneSegment(i, start, end)) {
      continue; // Root bone or invalid
    }

    LineHit hit = intersectRayLineSegment(currentRay_, start, end, boneThickness);

    if (hit.hit && hit.distance < closestSkeletonDist) {
      closestSkeletonDist = hit.distance;
      closestSkeletonType = HoverType::Bone;
      closestSkeletonIndex = i;
      closestSkeletonPoint = hit.point;
    }
  }

  // Skeleton takes priority over meshes, so always update if we found a skeleton hit
  // Even if mesh is closer, skeleton wins
  if (closestSkeletonType != HoverType::None) {
    state_.type = closestSkeletonType;
    state_.objectIndex = closestSkeletonIndex;
    state_.hitPoint = closestSkeletonPoint;
    state_.distance = closestSkeletonDist;
    state_.objectName = skeleton.boneName(closestSkeletonIndex);
  }
}

} // namespace w3d
