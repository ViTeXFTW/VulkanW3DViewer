#pragma once

#include "render/hover_detector.hpp"

namespace w3d {

enum class ViewerMode { ModelViewer, MapViewer };

/**
 * Centralized rendering state and display options.
 *
 * This struct consolidates all rendering-related flags and state
 * that control what and how things are rendered, separating
 * rendering decisions from the Application class.
 */
struct RenderState {
  // Active viewer mode
  ViewerMode mode = ViewerMode::ModelViewer;

  // Display toggles
  bool showMesh = true;
  bool showSkeleton = true;

  // Map viewer layer toggles
  bool showTerrain = true;
  bool showWater = true;
  bool showObjects = true;
  bool showTriggers = false;

  // Rendering mode flags
  bool useHLodModel = false;
  bool useSkinnedRendering = false;

  // Animation state tracking
  float lastAppliedFrame = -1.0f;

  // Hover display settings
  HoverNameDisplayMode hoverNameMode = HoverNameDisplayMode::FullName;
};

} // namespace w3d
