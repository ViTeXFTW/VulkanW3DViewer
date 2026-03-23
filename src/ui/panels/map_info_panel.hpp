#pragma once

#include "../ui_panel.hpp"

namespace w3d {

class MapInfoPanel : public UIPanel {
public:
  const char *title() const override { return "Map Info"; }
  void draw(UIContext &ctx) override;
};

} // namespace w3d
