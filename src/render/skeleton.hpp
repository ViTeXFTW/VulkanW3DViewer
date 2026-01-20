#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

#include "w3d/types.hpp"

namespace w3d {

// Represents the computed pose of a skeleton (bone world transforms)
class SkeletonPose {
public:
  SkeletonPose() = default;

  // Compute the rest pose from a hierarchy
  void computeRestPose(const Hierarchy &hierarchy);

  // Get bone count
  size_t boneCount() const { return boneWorldTransforms_.size(); }

  // Get world-space transform for a bone
  const glm::mat4 &boneTransform(size_t index) const { return boneWorldTransforms_[index]; }

  // Get world-space position of a bone
  glm::vec3 bonePosition(size_t index) const;

  // Get parent index for a bone (-1 if root)
  int parentIndex(size_t index) const { return parentIndices_[index]; }

  // Get bone name
  const std::string &boneName(size_t index) const { return boneNames_[index]; }

  // Check if pose is valid
  bool isValid() const { return !boneWorldTransforms_.empty(); }

  // Get all bone transforms (for passing to GPU)
  const std::vector<glm::mat4> &allTransforms() const { return boneWorldTransforms_; }

private:
  // Convert W3D Pivot to a local transformation matrix
  static glm::mat4 pivotToLocalMatrix(const Pivot &pivot);

  // Convert W3D quaternion to GLM
  static glm::quat toGlmQuat(const Quaternion &q);

  // Convert W3D vector to GLM
  static glm::vec3 toGlmVec3(const Vector3 &v);

  std::vector<glm::mat4> boneWorldTransforms_; // World-space transforms
  std::vector<int> parentIndices_;             // Parent bone indices (-1 for root)
  std::vector<std::string> boneNames_;         // Bone names for debugging
};

} // namespace w3d
