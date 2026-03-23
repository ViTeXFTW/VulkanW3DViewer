// Stub for render_state.hpp used in tests
#pragma once

namespace w3d {

enum class ViewerMode { ModelViewer, MapViewer };

struct RenderState {
  ViewerMode mode = ViewerMode::ModelViewer;

  bool showMesh = true;
  bool showSkeleton = true;

  bool showTerrain = true;
  bool showWater = true;
  bool showObjects = true;
  bool showTriggers = false;

  bool useHLodModel = false;
  bool useSkinnedRendering = false;
  float lastAppliedFrame = -1.0f;
};

} // namespace w3d
