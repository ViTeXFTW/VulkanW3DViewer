#include "display_panel.hpp"

#include "../ui_context.hpp"

#include <imgui.h>

namespace w3d {

void DisplayPanel::draw(UIContext &ctx) {
  if (!ctx.renderState)
    return;

  ImGui::Checkbox("Show Mesh", &ctx.renderState->showMesh);
  ImGui::Checkbox("Show Skeleton", &ctx.renderState->showSkeleton);
}

} // namespace w3d
