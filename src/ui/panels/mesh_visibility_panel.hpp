#pragma once

#include "../ui_panel.hpp"

namespace w3d {

/// Panel for toggling visibility of individual meshes.
/// Shows meshes visible at the current LOD level with checkboxes.
class MeshVisibilityPanel : public UIPanel {
public:
  const char *title() const override { return "Mesh Visibility"; }
  void draw(UIContext &ctx) override;
};

} // namespace w3d
