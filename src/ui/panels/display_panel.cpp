#include "display_panel.hpp"

#include "../ui_context.hpp"
#include "render/hover_detector.hpp"

#include <imgui.h>

namespace w3d {

void DisplayPanel::draw(UIContext &ctx) {
  if (!ctx.renderState)
    return;

  ImGui::Checkbox("Show Mesh", &ctx.renderState->showMesh);
  ImGui::Checkbox("Show Skeleton", &ctx.renderState->showSkeleton);

  ImGui::Separator();
  ImGui::Text("Hover Display");

  // Hover name display mode combo
  const char *modeNames[] = {"Full Name", "Base Name", "Descriptive"};
  int currentMode = static_cast<int>(ctx.renderState->hoverNameMode);
  if (ImGui::Combo("Name Mode", &currentMode, modeNames, 3)) {
    ctx.renderState->hoverNameMode = static_cast<HoverNameDisplayMode>(currentMode);
  }
}

} // namespace w3d
