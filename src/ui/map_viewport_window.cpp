#include "map_viewport_window.hpp"

#include "panels/layer_toggles_panel.hpp"
#include "panels/map_info_panel.hpp"
#include "panels/object_list_panel.hpp"
#include "panels/time_of_day_panel.hpp"

#include <imgui.h>

namespace w3d {

MapViewportWindow::MapViewportWindow() {
  addPanel<MapInfoPanel>();
  addPanel<LayerTogglesPanel>();
  addPanel<TimeOfDayPanel>();
  addPanel<ObjectListPanel>();
}

void MapViewportWindow::draw(UIContext &ctx) {
  if (!ImGui::Begin(name(), visiblePtr())) {
    ImGui::End();
    return;
  }

  for (auto &panel : panels_) {
    if (!panel->isEnabled()) {
      continue;
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (ImGui::CollapsingHeader(panel->title(), flags)) {
      ImGui::PushID(panel->title());
      panel->draw(ctx);
      ImGui::PopID();
    }

    ImGui::Separator();
  }

  ImGui::End();
}

} // namespace w3d
