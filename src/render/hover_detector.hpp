#ifndef W3D_RENDER_HOVER_DETECTOR_HPP
#define W3D_RENDER_HOVER_DETECTOR_HPP

#include <glm/glm.hpp>

#include <limits>
#include <string>

#include "raycast.hpp"

namespace w3d {

class RenderableMesh;
class SkeletonRenderer;
class HLodModel;
class SkeletonPose;

enum class HoverType { None, Mesh, Bone, Joint };

// Display mode for hover tooltip mesh names
enum class HoverNameDisplayMode {
  FullName,   // "SoldierBody_sub0" - exact internal name
  BaseName,   // "SoldierBody" - base mesh name without suffix
  Descriptive // "SoldierBody (part 1 of 3)" - user-friendly description
};

struct HoverState {
  HoverType type = HoverType::None;
  size_t objectIndex = 0;   // Which mesh/bone/joint
  size_t triangleIndex = 0; // For mesh triangles (debugging/future use)
  glm::vec3 hitPoint{0.0f};
  float distance = std::numeric_limits<float>::max();
  std::string objectName; // Name of hovered mesh/bone (full name with suffix)

  // Sub-mesh metadata (populated for HLod meshes)
  std::string baseName;    // Base mesh name without _subN suffix
  size_t subMeshIndex = 0; // Which sub-mesh (0-indexed)
  size_t subMeshTotal = 1; // Total sub-meshes for this base mesh

  void reset() {
    type = HoverType::None;
    objectIndex = 0;
    triangleIndex = 0;
    hitPoint = glm::vec3(0.0f);
    distance = std::numeric_limits<float>::max();
    objectName.clear();
    baseName.clear();
    subMeshIndex = 0;
    subMeshTotal = 1;
  }

  bool isHovering() const { return type != HoverType::None; }

  // Get formatted name based on display mode
  std::string displayName(HoverNameDisplayMode mode) const {
    // Handle empty state
    if (objectName.empty()) {
      return "";
    }

    // For single sub-mesh or empty baseName, always return objectName
    if (subMeshTotal <= 1 || baseName.empty()) {
      return objectName;
    }

    switch (mode) {
    case HoverNameDisplayMode::FullName:
      return objectName;

    case HoverNameDisplayMode::BaseName:
      return baseName;

    case HoverNameDisplayMode::Descriptive:
      // Format: "BaseName (part X of Y)" with 1-indexed parts
      return baseName + " (part " + std::to_string(subMeshIndex + 1) + " of " +
             std::to_string(subMeshTotal) + ")";
    }

    return objectName; // Fallback
  }
};

class HoverDetector {
public:
  HoverDetector() = default;

  // Update hover state based on current mouse position
  // This generates the ray and prepares for testing
  void update(const glm::vec2 &mousePos, const glm::vec2 &screenSize, const glm::mat4 &viewMatrix,
              const glm::mat4 &projMatrix);

  // Test against renderable meshes
  void testMeshes(const RenderableMesh &meshes);

  // Test against HLod model meshes (LOD-aware, bone-space ray transform)
  // Only tests visible meshes (aggregates + current LOD level)
  // pose: Optional skeleton pose for bone-space ray transformation
  void testHLodMeshes(const HLodModel &model, const SkeletonPose *pose = nullptr);

  // Test against HLod skinned meshes (uses rest-pose geometry)
  // Note: For GPU-skinned meshes, we test against rest-pose vertices
  // which may be less accurate during animation
  void testHLodSkinnedMeshes(const HLodModel &model);

  // Test against skeleton
  void testSkeleton(const SkeletonRenderer &skeleton, float boneThickness = 0.05f);

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

} // namespace w3d

#endif // W3D_RENDER_HOVER_DETECTOR_HPP
