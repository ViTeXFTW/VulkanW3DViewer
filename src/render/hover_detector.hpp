#ifndef W3D_RENDER_HOVER_DETECTOR_HPP
#define W3D_RENDER_HOVER_DETECTOR_HPP

#include "raycast.hpp"
#include <glm/glm.hpp>
#include <limits>
#include <string>

namespace w3d {

class RenderableMesh;
class SkeletonRenderer;

enum class HoverType {
  None,
  Mesh,
  Bone,
  Joint
};

struct HoverState {
  HoverType type = HoverType::None;
  size_t objectIndex = 0;      // Which mesh/bone/joint
  size_t triangleIndex = 0;    // For mesh triangles (debugging/future use)
  glm::vec3 hitPoint{0.0f};
  float distance = std::numeric_limits<float>::max();
  std::string objectName;      // Name of hovered mesh/bone

  void reset() {
    type = HoverType::None;
    objectIndex = 0;
    triangleIndex = 0;
    hitPoint = glm::vec3(0.0f);
    distance = std::numeric_limits<float>::max();
    objectName.clear();
  }

  bool isHovering() const { return type != HoverType::None; }
};

class HoverDetector {
public:
  HoverDetector() = default;

  // Update hover state based on current mouse position
  // This generates the ray and prepares for testing
  void update(
    const glm::vec2 &mousePos,
    const glm::vec2 &screenSize,
    const glm::mat4 &viewMatrix,
    const glm::mat4 &projMatrix
  );

  // Test against renderable meshes
  void testMeshes(const RenderableMesh &meshes);

  // Test against skeleton
  void testSkeleton(
    const SkeletonRenderer &skeleton,
    float boneThickness = 0.05f
  );

  // Query current hover state
  const HoverState &state() const { return state_; }
  HoverState &state() { return state_; }

  // Convenience queries
  bool isHovering() const { return state_.isHovering(); }
  bool isHoveringMesh() const { return state_.type == HoverType::Mesh; }
  bool isHoveringBone() const { return state_.type == HoverType::Bone; }
  bool isHoveringJoint() const { return state_.type == HoverType::Joint; }

  // Get the current ray (for debugging)
  const Ray &ray() const { return currentRay_; }

private:
  HoverState state_;
  Ray currentRay_;
};

}  // namespace w3d

#endif  // W3D_RENDER_HOVER_DETECTOR_HPP
