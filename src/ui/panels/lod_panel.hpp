#pragma once

#include "../ui_panel.hpp"

namespace w3d {

/// Panel for LOD (Level of Detail) controls.
/// Shows current LOD level, auto/manual selection mode, and LOD details.
class LODPanel : public UIPanel {
public:
  const char* title() const override { return "LOD Controls"; }
  void draw(UIContext& ctx) override;
};

} // namespace w3d
