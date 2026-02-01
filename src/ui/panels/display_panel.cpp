#include "display_panel.hpp"

#include "../ui_context.hpp"

#include <imgui.h>

namespace w3d {

void DisplayPanel::draw(UIContext &ctx) {
  if (ctx.showMesh) {
    ImGui::Checkbox("Show Mesh", ctx.showMesh);
  }
  if (ctx.showSkeleton) {
    ImGui::Checkbox("Show Skeleton", ctx.showSkeleton);
  }
}

} // namespace w3d
