#include "model_info_panel.hpp"

#include "../ui_context.hpp"
#include "render/hlod_model.hpp"
#include "render/renderable_mesh.hpp"
#include "render/skeleton.hpp"
#include "w3d/types.hpp"

#include <imgui.h>

namespace w3d {

void ModelInfoPanel::draw(UIContext& ctx) {
  if (!ctx.loadedFile) {
    ImGui::Text("No model loaded");
    ImGui::Text("Use File > Open to load a W3D model");
    return;
  }

  ImGui::Text("Loaded: %s", ctx.loadedFilePath.c_str());

  if (ctx.useHLodModel && ctx.hlodModel) {
    ImGui::Text("HLod: %s", ctx.hlodModel->name().c_str());
    ImGui::Text("Meshes: %zu (GPU: %zu)", ctx.loadedFile->meshes.size(),
                ctx.hlodModel->totalMeshCount());
  } else if (ctx.renderableMesh) {
    ImGui::Text("Meshes: %zu (GPU: %zu)", ctx.loadedFile->meshes.size(),
                ctx.renderableMesh->meshCount());
  } else {
    ImGui::Text("Meshes: %zu", ctx.loadedFile->meshes.size());
  }

  ImGui::Text("Hierarchies: %zu", ctx.loadedFile->hierarchies.size());
  ImGui::Text("Animations: %zu",
              ctx.loadedFile->animations.size() + ctx.loadedFile->compressedAnimations.size());

  // Skeleton info
  if (ctx.skeletonPose && ctx.skeletonPose->isValid()) {
    ImGui::Text("Skeleton bones: %zu", ctx.skeletonPose->boneCount());
  }
}

} // namespace w3d
