#include "skeleton.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace w3d {

glm::quat SkeletonPose::toGlmQuat(const Quaternion &q) {
  // GLM quaternion constructor order: w, x, y, z
  return glm::quat(q.w, q.x, q.y, q.z);
}

glm::vec3 SkeletonPose::toGlmVec3(const Vector3 &v) {
  return glm::vec3(v.x, v.y, v.z);
}

glm::mat4 SkeletonPose::pivotToLocalMatrix(const Pivot &pivot) {
  // Start with identity
  glm::mat4 localMatrix(1.0f);

  // Apply translation
  localMatrix = glm::translate(localMatrix, toGlmVec3(pivot.translation));

  // Apply rotation from quaternion
  // W3D stores quaternion rotation, which we use directly
  glm::quat rotation = toGlmQuat(pivot.rotation);
  localMatrix *= glm::mat4_cast(rotation);

  return localMatrix;
}

void SkeletonPose::computeRestPose(const Hierarchy &hierarchy) {
  size_t numBones = hierarchy.pivots.size();
  if (numBones == 0) {
    boneWorldTransforms_.clear();
    parentIndices_.clear();
    boneNames_.clear();
    return;
  }

  boneWorldTransforms_.resize(numBones);
  parentIndices_.resize(numBones);
  boneNames_.resize(numBones);

  // Process bones in order (parents come before children in W3D format)
  for (size_t i = 0; i < numBones; ++i) {
    const Pivot &pivot = hierarchy.pivots[i];

    // Store bone name and parent index
    boneNames_[i] = pivot.name;
    // W3D uses 0xFFFFFFFF (-1 as unsigned) to indicate root bone
    parentIndices_[i] =
        (pivot.parentIndex == 0xFFFFFFFF) ? -1 : static_cast<int>(pivot.parentIndex);

    // Compute local transform from pivot data
    glm::mat4 localTransform = pivotToLocalMatrix(pivot);

    // Compute world transform
    if (parentIndices_[i] < 0) {
      // Root bone - local transform is world transform
      boneWorldTransforms_[i] = localTransform;
    } else {
      // Child bone - multiply parent's world transform by local transform
      size_t parentIdx = static_cast<size_t>(parentIndices_[i]);
      boneWorldTransforms_[i] = boneWorldTransforms_[parentIdx] * localTransform;
    }
  }
}

void SkeletonPose::computeAnimatedPose(const Hierarchy &hierarchy,
                                       const std::vector<glm::vec3> &animTranslations,
                                       const std::vector<glm::quat> &animRotations) {
  size_t numBones = hierarchy.pivots.size();
  if (numBones == 0) {
    boneWorldTransforms_.clear();
    parentIndices_.clear();
    boneNames_.clear();
    return;
  }

  // Ensure animation data matches bone count
  if (animTranslations.size() != numBones || animRotations.size() != numBones) {
    // Fall back to rest pose if animation data doesn't match
    computeRestPose(hierarchy);
    return;
  }

  boneWorldTransforms_.resize(numBones);
  parentIndices_.resize(numBones);
  boneNames_.resize(numBones);

  // Process bones in order (parents come before children in W3D format)
  for (size_t i = 0; i < numBones; ++i) {
    const Pivot &pivot = hierarchy.pivots[i];

    // Store bone name and parent index
    boneNames_[i] = pivot.name;
    parentIndices_[i] =
        (pivot.parentIndex == 0xFFFFFFFF) ? -1 : static_cast<int>(pivot.parentIndex);

    // Compute local transform: base translation + animated translation + animated rotation
    glm::mat4 localTransform(1.0f);

    // Apply base translation from pivot
    localTransform = glm::translate(localTransform, toGlmVec3(pivot.translation));

    // Apply animated translation offset
    localTransform = glm::translate(localTransform, animTranslations[i]);

    // Apply animated rotation
    localTransform *= glm::mat4_cast(animRotations[i]);

    // Compute world transform
    if (parentIndices_[i] < 0) {
      // Root bone - local transform is world transform
      boneWorldTransforms_[i] = localTransform;
    } else {
      // Child bone - multiply parent's world transform by local transform
      size_t parentIdx = static_cast<size_t>(parentIndices_[i]);
      boneWorldTransforms_[i] = boneWorldTransforms_[parentIdx] * localTransform;
    }
  }
}

glm::vec3 SkeletonPose::bonePosition(size_t index) const {
  if (index >= boneWorldTransforms_.size()) {
    return glm::vec3(0.0f);
  }

  // Extract position from the 4th column of the world transform matrix
  const glm::mat4 &transform = boneWorldTransforms_[index];
  return glm::vec3(transform[3]);
}

} // namespace w3d
