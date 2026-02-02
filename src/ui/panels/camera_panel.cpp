#include "camera_panel.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "../ui_context.hpp"
#include "render/camera.hpp"

#include <imgui.h>

namespace w3d {

void CameraPanel::draw(UIContext &ctx) {
  if (!ctx.camera) {
    ImGui::TextDisabled("No camera available");
    return;
  }

  auto &camera = *ctx.camera;

  ImGui::Text("Left-drag to orbit, scroll to zoom");

  float yaw = glm::degrees(camera.yaw());
  float pitch = glm::degrees(camera.pitch());
  float dist = camera.distance();

  if (ImGui::SliderFloat("Yaw", &yaw, -180.0f, 180.0f)) {
    camera.setYaw(glm::radians(yaw));
  }
  if (ImGui::SliderFloat("Pitch", &pitch, -85.0f, 85.0f)) {
    camera.setPitch(glm::radians(pitch));
  }
  if (ImGui::SliderFloat("Distance", &dist, 0.1f, 1000.0f, "%.1f", ImGuiSliderFlags_Logarithmic)) {
    camera.setDistance(dist);
  }

  if (ImGui::Button("Reset Camera")) {
    if (ctx.onResetCamera) {
      ctx.onResetCamera();
    }
  }
}

} // namespace w3d
