#include "hover_tooltip.hpp"

#include "render/hover_detector.hpp"
#include "ui_context.hpp"

#include <imgui.h>

namespace w3d {

HoverTooltip::HoverTooltip() {
  // Always visible - this is a special window
  visible_ = true;
}

void HoverTooltip::draw(UIContext &ctx) {
  // Only show when something is being hovered
  if (!ctx.hoverState || !ctx.hoverState->isHovering() || ctx.hoverState->objectName.empty()) {
    return;
  }

  const auto &hover = *ctx.hoverState;

  // Position tooltip near mouse cursor
  ImVec2 mousePos = ImGui::GetMousePos();
  ImGui::SetNextWindowPos(ImVec2(mousePos.x + 15, mousePos.y + 15));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;

  ImGui::Begin(name(), nullptr, flags);

  // Determine type string
  const char *typeStr = "";
  switch (hover.type) {
  case HoverType::Mesh:
    typeStr = "Mesh";
    break;
  case HoverType::Bone:
    typeStr = "Bone";
    break;
  case HoverType::Joint:
    typeStr = "Joint";
    break;
  default:
    break;
  }

  // Get display name based on settings
  HoverNameDisplayMode displayMode = HoverNameDisplayMode::FullName;
  if (ctx.renderState) {
    displayMode = ctx.renderState->hoverNameMode;
  }
  std::string displayName = hover.displayName(displayMode);

  // Display with colored text
  ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "%s: %s", typeStr, displayName.c_str());

  ImGui::End();
}

} // namespace w3d
