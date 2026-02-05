#include "viewport_window.hpp"

#include "panels/animation_panel.hpp"
#include "panels/camera_panel.hpp"
#include "panels/display_panel.hpp"
#include "panels/lod_panel.hpp"
#include "panels/mesh_visibility_panel.hpp"
#include "panels/model_info_panel.hpp"

#include <imgui.h>

namespace w3d {

ViewportWindow::ViewportWindow() {
  // Add default panels in display order
  addPanel<ModelInfoPanel>();
  addPanel<AnimationPanel>();
  addPanel<DisplayPanel>();
  addPanel<MeshVisibilityPanel>();
  addPanel<LODPanel>();
  addPanel<CameraPanel>();
}

void ViewportWindow::draw(UIContext &ctx) {
  if (!ImGui::Begin(name(), visiblePtr())) {
    ImGui::End();
    return;
  }

  for (auto &panel : panels_) {
    if (!panel->isEnabled()) {
      continue;
    }

    // Use collapsing header for each panel
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
