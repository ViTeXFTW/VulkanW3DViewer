#include "settings_window.hpp"

#include <cstring>
#include <filesystem>

#include "core/settings.hpp"
#include "ui_context.hpp"

#include <imgui.h>

namespace w3d {

SettingsWindow::SettingsWindow() {
  // Configure the texture directory browser
  directoryBrowser_.setBrowseMode(BrowseMode::Directory);
  directoryBrowser_.setTitle("Select Texture Directory");
  directoryBrowser_.setVisible(false);

  // Configure the game directory browser
  gameDirectoryBrowser_.setBrowseMode(BrowseMode::Directory);
  gameDirectoryBrowser_.setTitle("Select Game Directory");
  gameDirectoryBrowser_.setVisible(false);
}

void SettingsWindow::open() {
  shouldOpen_ = true;
}

void SettingsWindow::copySettingsToEdit(const Settings &settings) {
  editTexturePath_ = settings.texturePath;
  editGameDirectory_ = settings.gameDirectory;
  editShowMesh_ = settings.showMesh;
  editShowSkeleton_ = settings.showSkeleton;
}

void SettingsWindow::applyAndSave(UIContext &ctx) {
  ctx.settings->texturePath = editTexturePath_;
  ctx.settings->gameDirectory = editGameDirectory_;
  ctx.settings->showMesh = editShowMesh_;
  ctx.settings->showSkeleton = editShowSkeleton_;
  ctx.settings->saveDefault();
}

void SettingsWindow::draw(UIContext &ctx) {
  // Check if settings are available
  if (!ctx.settings) {
    return;
  }

  // Handle directory browser being open - don't show settings modal while browsing
  if (directoryBrowserOpen_) {
    directoryBrowser_.draw(ctx);

    // Check if browser was closed (via Cancel or selection callback)
    if (!directoryBrowser_.isVisible()) {
      directoryBrowserOpen_ = false;
      // Reopen the settings modal
      shouldOpen_ = true;
    }
    return;
  }

  // Handle game directory browser being open
  if (gameDirectoryBrowserOpen_) {
    gameDirectoryBrowser_.draw(ctx);

    // Check if browser was closed
    if (!gameDirectoryBrowser_.isVisible()) {
      gameDirectoryBrowserOpen_ = false;
      // Reopen the settings modal
      shouldOpen_ = true;
    }
    return;
  }

  // Handle open request
  if (shouldOpen_) {
    ImGui::OpenPopup("Settings##Modal");
    // Only copy settings on fresh open, not when returning from browser
    if (!isOpen_) {
      copySettingsToEdit(*ctx.settings);
    }
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

    // Use a buffer for InputText
    char texturePathBuf[512];
#ifdef _WIN32
    strncpy_s(texturePathBuf, editTexturePath_.c_str(), sizeof(texturePathBuf) - 1);
#else
    std::strncpy(texturePathBuf, editTexturePath_.c_str(), sizeof(texturePathBuf) - 1);
    texturePathBuf[sizeof(texturePathBuf) - 1] = '\0';
#endif

    // Input field with Browse button
    float browseButtonWidth = 80.0f;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseButtonWidth -
                            ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::InputText("##TexturePath", texturePathBuf, sizeof(texturePathBuf))) {
      editTexturePath_ = texturePathBuf;
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse...", ImVec2(browseButtonWidth, 0))) {
      // Initialize browser at current path or current working directory
      if (!editTexturePath_.empty() && std::filesystem::exists(editTexturePath_)) {
        directoryBrowser_.openAt(editTexturePath_);
      } else {
        directoryBrowser_.openAt(std::filesystem::current_path());
      }

      // Set up callback to receive selected directory
      directoryBrowser_.setPathSelectedCallback([this](const std::filesystem::path &path) {
        editTexturePath_ = path.string();
        directoryBrowser_.setVisible(false);
      });

      directoryBrowser_.setVisible(true);
      directoryBrowserOpen_ = true;

      // Close the settings modal to allow interaction with browser
      ImGui::CloseCurrentPopup();
    }
    ImGui::TextDisabled("Leave empty to use default location");

    ImGui::Spacing();

    // BIG Archive Settings section
    ImGui::SeparatorText("BIG Archive Settings");

    ImGui::Text("Game Directory:");
    ImGui::TextDisabled("Location containing W3DZH.big and TexturesZH.big");

    // Use a buffer for InputText
    char gameDirBuf[512];
    std::strncpy(gameDirBuf, editGameDirectory_.c_str(), sizeof(gameDirBuf) - 1);
    gameDirBuf[sizeof(gameDirBuf) - 1] = '\0';

    // Input field with Browse button
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseButtonWidth -
                            ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::InputText("##GameDir", gameDirBuf, sizeof(gameDirBuf))) {
      editGameDirectory_ = gameDirBuf;
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse...##GameDir", ImVec2(browseButtonWidth, 0))) {
      // Initialize browser at current path or current working directory
      if (!editGameDirectory_.empty() && std::filesystem::exists(editGameDirectory_)) {
        gameDirectoryBrowser_.openAt(editGameDirectory_);
      } else {
        gameDirectoryBrowser_.openAt(std::filesystem::current_path());
      }

      // Set up callback to receive selected directory
      gameDirectoryBrowser_.setPathSelectedCallback([this](const std::filesystem::path &path) {
        editGameDirectory_ = path.string();
        gameDirectoryBrowser_.setVisible(false);
      });

      gameDirectoryBrowser_.setVisible(true);
      gameDirectoryBrowserOpen_ = true;

      // Close the settings modal to allow interaction with browser
      ImGui::CloseCurrentPopup();
    }

    ImGui::Spacing();

    // Cache status and management
    if (ctx.isBigArchiveInitialized) {
      ImGui::SeparatorText("Cache Status");

      // Format cache size
      char cacheSizeStr[64];
      if (ctx.cacheSize < 1024) {
        std::snprintf(cacheSizeStr, sizeof(cacheSizeStr), "%zu B", ctx.cacheSize);
      } else if (ctx.cacheSize < 1024 * 1024) {
        std::snprintf(cacheSizeStr, sizeof(cacheSizeStr), "%.1f KB", ctx.cacheSize / 1024.0);
      } else if (ctx.cacheSize < 1024 * 1024 * 1024) {
        std::snprintf(cacheSizeStr, sizeof(cacheSizeStr), "%.1f MB",
                      ctx.cacheSize / (1024.0 * 1024.0));
      } else {
        std::snprintf(cacheSizeStr, sizeof(cacheSizeStr), "%.2f GB",
                      ctx.cacheSize / (1024.0 * 1024.0 * 1024.0));
      }

      ImGui::Text("Cache Size: %s", cacheSizeStr);
      ImGui::Text("Models Found: %zu", ctx.availableModelCount);

      // Clear & Rescan button
      if (ImGui::Button("Clear & Rescan Cache")) {
        if (ctx.onClearAndRescanCache) {
          ctx.onClearAndRescanCache();
        }
      }
      ImGui::TextDisabled("Deletes all cached files and re-scans archives");
    }

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
      applyAndSave(ctx);
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
