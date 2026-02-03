#include "hover_detector.hpp"

#include <algorithm>

#include "hlod_model.hpp"
#include "renderable_mesh.hpp"
#include "skeleton.hpp"
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

void HoverDetector::testHLodMeshes(const HLodModel &model, const SkeletonPose *pose) {
  if (!model.hasData()) {
    return;
  }

  // Get only visible meshes (aggregates + current LOD)
  auto visibleIndices = model.visibleMeshIndices();

  float closestDist = std::numeric_limits<float>::max();
  size_t closestMeshIndex = 0;
  size_t closestTriIndex = 0;
  glm::vec3 closestPoint(0.0f);

  for (size_t visIdx : visibleIndices) {
    const auto &mesh = model.meshes()[visIdx];

    // Transform ray to bone space if mesh is bone-attached and pose exists
    Ray testRay = currentRay_;
    if (pose && mesh.boneIndex >= 0 && static_cast<size_t>(mesh.boneIndex) < pose->boneCount()) {
      testRay = transformRayToBoneSpace(currentRay_,
                                        pose->boneTransform(static_cast<size_t>(mesh.boneIndex)));
    }

    // Test all triangles in this mesh
    size_t triCount = model.triangleCount(visIdx);
    for (size_t triIdx = 0; triIdx < triCount; ++triIdx) {
      glm::vec3 v0, v1, v2;
      if (!model.getTriangle(visIdx, triIdx, v0, v1, v2)) {
        continue;
      }

      TriangleHit hit = intersectRayTriangle(testRay, v0, v1, v2);

      if (hit.hit && hit.distance < closestDist) {
        closestDist = hit.distance;
        closestMeshIndex = visIdx;
        closestTriIndex = triIdx;
        closestPoint = hit.point;
      }
    }
  }

  // Update state if we found a hit closer than current
  if (closestDist < state_.distance) {
    const auto &hitMesh = model.meshes()[closestMeshIndex];

    state_.type = HoverType::Mesh;
    state_.objectIndex = closestMeshIndex;
    state_.triangleIndex = closestTriIndex;
    state_.hitPoint = closestPoint;
    state_.distance = closestDist;
    state_.objectName = hitMesh.name;
    state_.baseName = hitMesh.baseName;
    state_.subMeshIndex = hitMesh.subMeshIndex;
    state_.subMeshTotal = hitMesh.subMeshTotal;
  }
}

void HoverDetector::testHLodSkinnedMeshes(const HLodModel &model) {
  if (!model.hasSkinning()) {
    return;
  }

  // Get only visible skinned meshes (aggregates + current LOD)
  auto visibleIndices = model.visibleSkinnedMeshIndices();

  float closestDist = std::numeric_limits<float>::max();
  size_t closestMeshIndex = 0;
  size_t closestTriIndex = 0;
  glm::vec3 closestPoint(0.0f);

  for (size_t visIdx : visibleIndices) {
    // Test all triangles in this mesh (rest-pose geometry)
    size_t triCount = model.skinnedTriangleCount(visIdx);
    for (size_t triIdx = 0; triIdx < triCount; ++triIdx) {
      glm::vec3 v0, v1, v2;
      if (!model.getSkinnedTriangle(visIdx, triIdx, v0, v1, v2)) {
        continue;
      }

      TriangleHit hit = intersectRayTriangle(currentRay_, v0, v1, v2);

      if (hit.hit && hit.distance < closestDist) {
        closestDist = hit.distance;
        closestMeshIndex = visIdx;
        closestTriIndex = triIdx;
        closestPoint = hit.point;
      }
    }
  }

  // Update state if we found a hit closer than current
  if (closestDist < state_.distance) {
    const auto &hitMesh = model.skinnedMeshes()[closestMeshIndex];

    state_.type = HoverType::Mesh;
    state_.objectIndex = closestMeshIndex;
    state_.triangleIndex = closestTriIndex;
    state_.hitPoint = closestPoint;
    state_.distance = closestDist;
    state_.objectName = hitMesh.name;
    state_.baseName = hitMesh.baseName;
    state_.subMeshIndex = hitMesh.subMeshIndex;
    state_.subMeshTotal = hitMesh.subMeshTotal;
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
