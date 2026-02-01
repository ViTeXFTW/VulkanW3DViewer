#include "animation_panel.hpp"

#include "../ui_context.hpp"
#include "render/animation_player.hpp"

#include <imgui.h>

namespace w3d {

void AnimationPanel::draw(UIContext& ctx) {
  if (!ctx.animationPlayer || ctx.animationPlayer->animationCount() == 0) {
    ImGui::TextDisabled("No animations available");
    return;
  }

  auto& player = *ctx.animationPlayer;

  // Animation dropdown
  if (ImGui::BeginCombo("##animation",
                        player.animationName(player.currentAnimationIndex()).c_str())) {
    for (size_t i = 0; i < player.animationCount(); ++i) {
      bool isSelected = (i == player.currentAnimationIndex());
      if (ImGui::Selectable(player.animationName(i).c_str(), isSelected)) {
        player.selectAnimation(i);
      }
      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // Frame slider
  float frame = player.currentFrame();
  float maxFrame = player.maxFrame();
  if (ImGui::SliderFloat("Frame", &frame, 0.0f, maxFrame)) {
    player.pause();
    player.setFrame(frame);
  }

  // Play/Pause and Stop buttons
  if (player.isPlaying()) {
    if (ImGui::Button("Pause")) {
      player.pause();
    }
  } else {
    if (ImGui::Button("Play")) {
      player.play();
    }
  }

  ImGui::SameLine();
  if (ImGui::Button("Stop")) {
    player.stop();
  }

  // Playback mode
  ImGui::SameLine();
  const char* modeStr = "Loop";
  switch (player.playbackMode()) {
  case PlaybackMode::Once:
    modeStr = "Once";
    break;
  case PlaybackMode::Loop:
    modeStr = "Loop";
    break;
  case PlaybackMode::PingPong:
    modeStr = "PingPong";
    break;
  }

  if (ImGui::BeginCombo("Mode", modeStr)) {
    if (ImGui::Selectable("Once", player.playbackMode() == PlaybackMode::Once)) {
      player.setPlaybackMode(PlaybackMode::Once);
    }
    if (ImGui::Selectable("Loop", player.playbackMode() == PlaybackMode::Loop)) {
      player.setPlaybackMode(PlaybackMode::Loop);
    }
    if (ImGui::Selectable("PingPong", player.playbackMode() == PlaybackMode::PingPong)) {
      player.setPlaybackMode(PlaybackMode::PingPong);
    }
    ImGui::EndCombo();
  }

  // Info display
  ImGui::Text("Frame: %.1f / %u @ %u FPS", player.currentFrame(),
              player.numFrames() > 0 ? player.numFrames() - 1 : 0, player.frameRate());
}

} // namespace w3d
