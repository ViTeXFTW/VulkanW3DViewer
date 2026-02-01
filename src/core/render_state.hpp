#pragma once

namespace w3d {

/**
 * Centralized rendering state and display options.
 *
 * This struct consolidates all rendering-related flags and state
 * that control what and how things are rendered, separating
 * rendering decisions from the Application class.
 */
struct RenderState {
  // Display toggles
  bool showMesh = true;
  bool showSkeleton = true;

  // Rendering mode flags
  bool useHLodModel = false;
  bool useSkinnedRendering = false;

  // Animation state tracking
  float lastAppliedFrame = -1.0f;
};

} // namespace w3d
