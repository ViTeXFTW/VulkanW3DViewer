#pragma once

#include "../ui_panel.hpp"

namespace w3d {

/// Panel for camera controls.
/// Shows yaw, pitch, distance sliders and reset button.
class CameraPanel : public UIPanel {
public:
  const char* title() const override { return "Camera Controls"; }
  void draw(UIContext& ctx) override;
};

} // namespace w3d
