// Stub for render_state.hpp used in tests
#pragma once

namespace w3d {

struct RenderState {
  bool showMesh = true;
  bool showSkeleton = true;
  bool useHLodModel = false;
  bool useSkinnedRendering = false;
  float lastAppliedFrame = -1.0f;
};

} // namespace w3d
