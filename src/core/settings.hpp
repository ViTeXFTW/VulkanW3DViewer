#pragma once

#include <filesystem>
#include <string>

namespace w3d {

/// Application settings that persist between sessions.
/// Uses JSON format for storage via nlohmann/json.
struct Settings {
  // === Path Settings ===
  /// Custom texture search path (empty = use default)
  std::string texturePath;
  /// Last directory opened in file browser
  std::string lastBrowsedDirectory;

  // === Window Settings ===
  /// Last window width
  int windowWidth = 1280;
  /// Last window height
  int windowHeight = 720;

  // === Display Settings ===
  /// Show mesh by default
  bool showMesh = true;
  /// Show skeleton by default
  bool showSkeleton = true;

  // === Serialization ===

  /// Load settings from a file.
  /// Returns default settings if file doesn't exist or is invalid.
  static Settings load(const std::filesystem::path& path);

  /// Load settings from the default location.
  /// Uses AppPaths::settingsFilePath() to determine the path.
  static Settings loadDefault();

  /// Save settings to a file.
  /// Returns true on success, false on failure.
  bool save(const std::filesystem::path& path) const;

  /// Save settings to the default location.
  /// Uses AppPaths::settingsFilePath() to determine the path.
  bool saveDefault() const;
};

} // namespace w3d
