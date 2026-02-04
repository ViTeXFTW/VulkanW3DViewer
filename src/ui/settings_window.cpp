#include "settings_window.hpp"

#include <cstring>

#include "core/settings.hpp"
#include "ui_context.hpp"

#include <imgui.h>

namespace w3d {

void SettingsWindow::open() {
  shouldOpen_ = true;
}

void SettingsWindow::copySettingsToEdit(const Settings &settings) {
  editTexturePath_ = settings.texturePath;
  editShowMesh_ = settings.showMesh;
  editShowSkeleton_ = settings.showSkeleton;
}

void SettingsWindow::applyAndSave(Settings &settings) {
  settings.texturePath = editTexturePath_;
  settings.showMesh = editShowMesh_;
  settings.showSkeleton = editShowSkeleton_;
  settings.saveDefault();
}

void SettingsWindow::draw(UIContext &ctx) {
  // Check if settings are available
  if (!ctx.settings) {
    return;
  }

  // Handle open request
  if (shouldOpen_) {
    ImGui::OpenPopup("Settings##Modal");
    copySettingsToEdit(*ctx.settings);
    shouldOpen_ = false;
    isOpen_ = true;
  }

  // Center the modal
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(450, 0), ImGuiCond_Appearing);

  if (ImGui::BeginPopupModal("Settings##Modal", &isOpen_, ImGuiWindowFlags_AlwaysAutoResize)) {
    // Path Settings section
    ImGui::SeparatorText("Path Settings");

    ImGui::Text("Texture Path:");
    ImGui::SetNextItemWidth(-1);

    // Use a buffer for InputText
    char texturePathBuf[512];
    std::strncpy(texturePathBuf, editTexturePath_.c_str(), sizeof(texturePathBuf) - 1);
    texturePathBuf[sizeof(texturePathBuf) - 1] = '\0';

    if (ImGui::InputText("##TexturePath", texturePathBuf, sizeof(texturePathBuf))) {
      editTexturePath_ = texturePathBuf;
    }
    ImGui::TextDisabled("Leave empty to use default location");

    ImGui::Spacing();

    // Display Settings section
    ImGui::SeparatorText("Default Display Settings");
    ImGui::TextDisabled("These settings determine what is shown when the application starts.");

    ImGui::Checkbox("Show Mesh on Startup", &editShowMesh_);
    ImGui::Checkbox("Show Skeleton on Startup", &editShowSkeleton_);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Buttons
    float buttonWidth = 100.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float totalWidth = buttonWidth * 2 + spacing;
    float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

    if (ImGui::Button("Save", ImVec2(buttonWidth, 0))) {
      applyAndSave(*ctx.settings);
      ImGui::CloseCurrentPopup();
      isOpen_ = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
      ImGui::CloseCurrentPopup();
      isOpen_ = false;
    }

    // Handle Escape key to cancel
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      ImGui::CloseCurrentPopup();
      isOpen_ = false;
    }

    ImGui::EndPopup();
  }
}

} // namespace w3d
