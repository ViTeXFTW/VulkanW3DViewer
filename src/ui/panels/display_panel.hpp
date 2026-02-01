#pragma once

#include "../ui_panel.hpp"

namespace w3d {

/// Panel for display options.
/// Controls visibility of mesh and skeleton.
class DisplayPanel : public UIPanel {
public:
  const char* title() const override { return "Display Options"; }
  void draw(UIContext& ctx) override;
};

} // namespace w3d
