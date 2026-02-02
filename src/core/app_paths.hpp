#pragma once

#include <filesystem>
#include <optional>

namespace w3d {

/// Cross-platform application directory utilities.
/// Provides paths for storing user configuration and data.
class AppPaths {
public:
  /// Application name used for directory naming
  static constexpr const char* kAppName = "VulkanW3DViewer";

  /// Get the application data directory (creates if needed).
  /// Returns:
  ///   - Windows: %APPDATA%/VulkanW3DViewer
  ///   - Linux: $XDG_CONFIG_HOME/VulkanW3DViewer or ~/.config/VulkanW3DViewer
  ///   - macOS: ~/Library/Application Support/VulkanW3DViewer
  /// Returns nullopt if directory cannot be determined.
  static std::optional<std::filesystem::path> appDataDir();

  /// Get path for ImGui configuration file.
  /// Returns nullopt if app data directory cannot be determined.
  static std::optional<std::filesystem::path> imguiIniPath();

  /// Get path for application settings file.
  /// Returns nullopt if app data directory cannot be determined.
  static std::optional<std::filesystem::path> settingsFilePath();

  /// Ensure the application data directory exists.
  /// Returns true if directory exists or was created successfully.
  static bool ensureAppDataDir();

private:
  /// Get the platform-specific base configuration directory.
  /// Returns nullopt if the directory cannot be determined.
  static std::optional<std::filesystem::path> baseConfigDir();
};

} // namespace w3d
