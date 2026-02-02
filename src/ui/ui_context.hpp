#pragma once

#include <functional>
#include <optional>
#include <string>

// Forward declarations to avoid header dependencies
struct GLFWwindow;

namespace w3d {

// Forward declarations
class Camera;
class AnimationPlayer;
class HLodModel;
class RenderableMesh;
class SkeletonPose;
struct HoverState;
struct W3DFile;

/// Shared UI context passed to all windows and panels.
/// Contains references to application state that UI components need to read/modify.
///
/// This struct enables loose coupling between UI components and application state.
/// Panels can access what they need without knowing about the application class.
///
/// Usage:
/// 1. Application creates UIContext and populates pointers
/// 2. UIManager passes UIContext to all windows/panels during draw()
/// 3. Panels read/write through the context references
struct UIContext {
  // === Window Reference ===
  GLFWwindow *window = nullptr;

  // === Loaded Model Data ===
  /// Currently loaded W3D file (null if nothing loaded)
  const W3DFile *loadedFile = nullptr;

  /// Path to the loaded file
  std::string loadedFilePath;

  // === Rendering Objects ===
  /// HLod model for complex models with LOD
  HLodModel *hlodModel = nullptr;

  /// Simple renderable mesh (when no HLod present)
  RenderableMesh *renderableMesh = nullptr;

  /// Whether to use HLod model (vs simple mesh)
  bool useHLodModel = false;

  /// Whether skinned rendering is active
  bool useSkinnedRendering = false;

  // === Camera ===
  Camera *camera = nullptr;

  // === Skeleton & Animation ===
  /// Current skeleton pose
  SkeletonPose *skeletonPose = nullptr;

  /// Animation player for playback control
  AnimationPlayer *animationPlayer = nullptr;

  // === Display Toggles ===
  /// Show mesh geometry
  bool *showMesh = nullptr;

  /// Show skeleton overlay
  bool *showSkeleton = nullptr;

  // === Hover Detection ===
  /// Current hover state (read-only for UI display)
  const HoverState *hoverState = nullptr;

  // === Actions/Callbacks ===
  /// Callback to request camera reset
  std::function<void()> onResetCamera;

  /// Callback to request file open dialog
  std::function<void()> onOpenFile;

  /// Callback to exit application
  std::function<void()> onExit;

  // === Convenience Methods ===

  /// Check if a model is currently loaded
  bool hasModel() const { return loadedFile != nullptr; }

  /// Check if model has mesh data to render
  bool hasMeshData() const;

  /// Check if skeleton is available
  bool hasSkeleton() const;

  /// Check if animations are available
  bool hasAnimations() const;
};

// Inline implementations
inline bool UIContext::hasMeshData() const {
  if (useHLodModel && hlodModel) {
    // HLodModel::hasData() check - we can't call it directly without including header
    return true; // Assume true if hlodModel is set
  }
  if (!useHLodModel && renderableMesh) {
    return true; // Assume true if renderableMesh is set
  }
  return false;
}

inline bool UIContext::hasSkeleton() const {
  return skeletonPose != nullptr;
}

inline bool UIContext::hasAnimations() const {
  return animationPlayer != nullptr;
}

} // namespace w3d
