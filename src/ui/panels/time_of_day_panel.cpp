#include "time_of_day_panel.hpp"

#include "../ui_context.hpp"
#include "lib/formats/map/types.hpp"
#include "render/lighting_state.hpp"

#include <imgui.h>

namespace w3d {

void TimeOfDayPanel::draw(UIContext &ctx) {
  if (!ctx.hasMap() || !ctx.lightingState) {
    ImGui::TextDisabled("No map loaded");
    return;
  }

  if (!ctx.lightingState->hasLighting()) {
    ImGui::TextDisabled("No lighting data");
    return;
  }

  const char *todNames[] = {"Morning", "Afternoon", "Evening", "Night"};
  int currentIndex = static_cast<int>(ctx.lightingState->timeOfDay()) - 1;
  if (currentIndex < 0 || currentIndex > 3) {
    currentIndex = 0;
  }

  if (ImGui::Combo("Time", &currentIndex, todNames, 4)) {
    auto newTod = static_cast<map::TimeOfDay>(currentIndex + 1);
    ctx.lightingState->setTimeOfDay(newTod);
  }

  ImGui::Separator();

  // Show current lighting values
  auto ambient = ctx.lightingState->objectAmbient();
  auto diffuse = ctx.lightingState->objectDiffuse();
  auto direction = ctx.lightingState->objectLightDirection();

  ImGui::Text("Terrain Light");
  ImGui::Indent();
  auto pc = ctx.lightingState->makeTerrainPushConstant(false);
  ImGui::ColorEdit3("Ambient##T", &pc.ambientColor.x, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit3("Diffuse##T", &pc.diffuseColor.x, ImGuiColorEditFlags_NoInputs);
  ImGui::Text("Direction: (%.2f, %.2f, %.2f)", pc.lightDirection.x, pc.lightDirection.y,
              pc.lightDirection.z);
  ImGui::Unindent();

  ImGui::Text("Object Light");
  ImGui::Indent();
  ImGui::ColorEdit3("Ambient##O", &ambient.x, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit3("Diffuse##O", &diffuse.x, ImGuiColorEditFlags_NoInputs);
  ImGui::Text("Direction: (%.2f, %.2f, %.2f)", direction.x, direction.y, direction.z);
  ImGui::Unindent();
}

} // namespace w3d
