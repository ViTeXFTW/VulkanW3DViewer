#pragma once

#include "../ui_panel.hpp"

namespace w3d {

class LayerTogglesPanel : public UIPanel {
public:
  const char *title() const override { return "Layer Visibility"; }
  void draw(UIContext &ctx) override;
};

} // namespace w3d
