#include "layer_toggles_panel.hpp"

#include "../ui_context.hpp"

#include <imgui.h>

namespace w3d {

void LayerTogglesPanel::draw(UIContext &ctx) {
  if (!ctx.renderState) {
    return;
  }

  if (!ctx.hasMap()) {
    ImGui::TextDisabled("No map loaded");
    return;
  }

  ImGui::Checkbox("Terrain", &ctx.renderState->showTerrain);
  ImGui::Checkbox("Water", &ctx.renderState->showWater);
  ImGui::Checkbox("Objects", &ctx.renderState->showObjects);
  ImGui::Checkbox("Triggers", &ctx.renderState->showTriggers);
}

} // namespace w3d
