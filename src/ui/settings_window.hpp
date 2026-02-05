#pragma once

#include <string>

#include "ui_window.hpp"
#include "file_browser.hpp"

namespace w3d {

struct Settings;

/// Modal popup window for editing application settings.
/// Displays settings that persist across sessions (texturePath, default display toggles).
/// Changes are only saved when the user clicks "Save", otherwise discarded on close.
class SettingsWindow : public UIWindow {
public:
  SettingsWindow();

  // UIWindow interface
  void draw(UIContext &ctx) override;
  const char *name() const override { return "Settings"; }
  bool showInViewMenu() const override { return false; }

  /// Open the settings modal popup.
  /// Call this from the menu bar to show the settings dialog.
  void open();

private:
  /// Whether to open the popup on next frame
  bool shouldOpen_ = false;

  /// Whether the popup is currently active
  bool isOpen_ = false;

  /// Whether the directory browser is currently open
  bool directoryBrowserOpen_ = false;

  /// Directory browser for selecting texture path
  FileBrowser directoryBrowser_;

  // Editable copies of settings (modified until Save/Cancel)
  std::string editTexturePath_;
  bool editShowMesh_ = true;
  bool editShowSkeleton_ = true;

  /// Copy current settings to edit buffers
  void copySettingsToEdit(const Settings &settings);

  /// Apply edit buffers to settings and save
  void applyAndSave(UIContext &ctx);
};

} // namespace w3d
