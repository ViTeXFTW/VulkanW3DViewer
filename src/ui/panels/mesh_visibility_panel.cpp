#include "mesh_visibility_panel.hpp"

#include "../ui_context.hpp"
#include "render/hlod_model.hpp"

#include <imgui.h>

namespace w3d {

void MeshVisibilityPanel::draw(UIContext &ctx) {
  if (!ctx.hlodModel || !ctx.hlodModel->hasData()) {
    ImGui::TextDisabled("No model loaded");
    return;
  }

  auto *model = ctx.hlodModel;
  bool useSkinned = ctx.renderState && ctx.renderState->useSkinnedRendering && model->hasSkinning();

  // Show All / Hide All buttons
  if (ImGui::Button("Show All")) {
    if (useSkinned) {
      model->setAllSkinnedMeshesHidden(false);
    } else {
      model->setAllMeshesHidden(false);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Hide All")) {
    if (useSkinned) {
      model->setAllSkinnedMeshesHidden(true);
    } else {
      model->setAllMeshesHidden(true);
    }
  }

  ImGui::Separator();

  // Get visible mesh indices (aggregates + current LOD)
  if (useSkinned) {
    const auto &meshes = model->skinnedMeshes();
    if (meshes.empty()) {
      ImGui::TextDisabled("No meshes");
      return;
    }

    // Show mesh count
    auto visibleIndices = model->visibleSkinnedMeshIndices();
    ImGui::Text("Meshes: %zu visible", visibleIndices.size());
    ImGui::Separator();

    // Scrollable list of meshes
    if (ImGui::BeginChild("MeshList", ImVec2(0, 200), true)) {
      for (size_t i = 0; i < meshes.size(); ++i) {
        const auto &mesh = meshes[i];

        // Only show meshes that would be visible at current LOD (ignore user visibility)
        if (!mesh.isAggregate && mesh.lodLevel != model->currentLOD()) {
          continue;
        }

        bool visible = !model->isSkinnedMeshHidden(i);
        std::string label = mesh.name;

        // Add aggregate indicator
        if (mesh.isAggregate) {
          label += " [A]";
        }

        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Checkbox(label.c_str(), &visible)) {
          model->setSkinnedMeshHidden(i, !visible);
        }
        ImGui::PopID();
      }
    }
    ImGui::EndChild();
  } else {
    const auto &meshes = model->meshes();
    if (meshes.empty()) {
      ImGui::TextDisabled("No meshes");
      return;
    }

    // Show mesh count
    auto visibleIndices = model->visibleMeshIndices();
    ImGui::Text("Meshes: %zu visible", visibleIndices.size());
    ImGui::Separator();

    // Scrollable list of meshes
    if (ImGui::BeginChild("MeshList", ImVec2(0, 200), true)) {
      for (size_t i = 0; i < meshes.size(); ++i) {
        const auto &mesh = meshes[i];

        // Only show meshes that would be visible at current LOD (ignore user visibility)
        if (!mesh.isAggregate && mesh.lodLevel != model->currentLOD()) {
          continue;
        }

        bool visible = !model->isMeshHidden(i);
        std::string label = mesh.name;

        // Add aggregate indicator
        if (mesh.isAggregate) {
          label += " [A]";
        }

        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Checkbox(label.c_str(), &visible)) {
          model->setMeshHidden(i, !visible);
        }
        ImGui::PopID();
      }
    }
    ImGui::EndChild();
  }
}

} // namespace w3d
