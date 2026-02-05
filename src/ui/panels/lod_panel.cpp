#include "lod_panel.hpp"

#include "../ui_context.hpp"
#include "lib/formats/w3d/hlod_model.hpp"

#include <imgui.h>

namespace w3d {

void LODPanel::draw(UIContext &ctx) {
  if (!ctx.renderState || !ctx.renderState->useHLodModel || !ctx.hlodModel ||
      ctx.hlodModel->lodCount() <= 1) {
    ImGui::TextDisabled("No LOD levels available");
    return;
  }

  auto &model = *ctx.hlodModel;

  // LOD mode selector
  bool autoMode = model.selectionMode() == w3d_types::LODSelectionMode::Auto;
  if (ImGui::Checkbox("Auto LOD Selection", &autoMode)) {
    model.setSelectionMode(autoMode ? w3d_types::LODSelectionMode::Auto : w3d_types::LODSelectionMode::Manual);
  }

  // Show current LOD info
  ImGui::Text("Current LOD: %zu / %zu", model.currentLOD() + 1, model.lodCount());

  if (model.selectionMode() == w3d_types::LODSelectionMode::Auto) {
    ImGui::Text("Screen size: %.1f px", model.currentScreenSize());
  }

  // Manual LOD selector
  if (model.selectionMode() == w3d_types::LODSelectionMode::Manual) {
    int currentLod = static_cast<int>(model.currentLOD());
    if (ImGui::SliderInt("LOD Level", &currentLod, 0, static_cast<int>(model.lodCount()) - 1)) {
      model.setCurrentLOD(static_cast<size_t>(currentLod));
    }
  }

  // Show LOD level details
  if (ImGui::TreeNode("LOD Details")) {
    for (size_t i = 0; i < model.lodCount(); ++i) {
      const auto &level = model.lodLevel(i);
      bool isCurrent = (i == model.currentLOD());

      ImGui::PushStyleColor(ImGuiCol_Text, isCurrent ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
                                                     : ImGui::GetStyleColorVec4(ImGuiCol_Text));

      ImGui::Text("LOD %zu: %zu meshes (maxSize=%.0f)", i, level.meshes.size(),
                  level.maxScreenSize);

      ImGui::PopStyleColor();
    }

    if (model.aggregateCount() > 0) {
      ImGui::Text("Aggregates: %zu (always rendered)", model.aggregateCount());
    }

    ImGui::TreePop();
  }
}

} // namespace w3d
