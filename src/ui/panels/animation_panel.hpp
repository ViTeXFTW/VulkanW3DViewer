#pragma once

#include "../ui_panel.hpp"

namespace w3d {

/// Panel for animation playback controls.
/// Includes animation selection, play/pause/stop buttons, frame slider, and playback mode.
class AnimationPanel : public UIPanel {
public:
  const char* title() const override { return "Animation"; }
  void draw(UIContext& ctx) override;
};

} // namespace w3d
