#include "settings.hpp"

#include <fstream>
#include <iostream>

#include "app_paths.hpp"

#include <nlohmann/json.hpp>

namespace w3d {

Settings Settings::load(const std::filesystem::path &path) {
  Settings settings; // Start with defaults

  if (!std::filesystem::exists(path)) {
    return settings; // First run - use defaults
  }

  std::ifstream file(path);
  if (!file) {
    std::cerr << "Warning: Could not open settings file: " << path.string() << "\n";
    return settings;
  }

  try {
    nlohmann::json json;
    file >> json;

    // Parse paths section
    if (json.contains("paths")) {
      auto &paths = json["paths"];
      if (paths.contains("texture_path")) {
        settings.texturePath = paths["texture_path"].get<std::string>();
      }
      if (paths.contains("last_browsed_directory")) {
        settings.lastBrowsedDirectory = paths["last_browsed_directory"].get<std::string>();
      }
      if (paths.contains("game_directory")) {
        settings.gameDirectory = paths["game_directory"].get<std::string>();
      }
      if (paths.contains("search_paths")) {
        settings.searchPaths = paths["search_paths"].get<std::vector<std::string>>();
      }
      if (paths.contains("custom_search_paths")) {
        settings.customSearchPaths = paths["custom_search_paths"].get<std::vector<std::string>>();
      }
    }

    // Parse window section
    if (json.contains("window")) {
      auto &window = json["window"];
      if (window.contains("width")) {
        settings.windowWidth = window["width"].get<int>();
      }
      if (window.contains("height")) {
        settings.windowHeight = window["height"].get<int>();
      }
    }

    // Parse display section
    if (json.contains("display")) {
      auto &display = json["display"];
      if (display.contains("show_mesh")) {
        settings.showMesh = display["show_mesh"].get<bool>();
      }
      if (display.contains("show_skeleton")) {
        settings.showSkeleton = display["show_skeleton"].get<bool>();
      }
    }
  } catch (const nlohmann::json::exception &e) {
    std::cerr << "Warning: Error parsing settings file: " << e.what() << "\n";
    // Return partially loaded settings or defaults
  }

  return settings;
}

Settings Settings::loadDefault() {
  auto path = AppPaths::settingsFilePath();
  if (!path) {
    return Settings{}; // Use defaults
  }
  return load(*path);
}

bool Settings::save(const std::filesystem::path &path) const {
  // Ensure parent directory exists
  auto parent = path.parent_path();
  if (!parent.empty() && !std::filesystem::exists(parent)) {
    std::error_code ec;
    if (!std::filesystem::create_directories(parent, ec)) {
      std::cerr << "Warning: Could not create settings directory: " << parent.string() << " ("
                << ec.message() << ")\n";
      return false;
    }
  }

  std::ofstream file(path);
  if (!file) {
    std::cerr << "Warning: Could not open settings file for writing: " << path.string() << "\n";
    return false;
  }

  try {
    nlohmann::json json;

    // Paths section
    json["paths"]["texture_path"] = texturePath;
    json["paths"]["last_browsed_directory"] = lastBrowsedDirectory;
    json["paths"]["game_directory"] = gameDirectory;
    json["paths"]["search_paths"] = searchPaths;
    json["paths"]["custom_search_paths"] = customSearchPaths;

    // Window section
    json["window"]["width"] = windowWidth;
    json["window"]["height"] = windowHeight;

    // Display section
    json["display"]["show_mesh"] = showMesh;
    json["display"]["show_skeleton"] = showSkeleton;

    file << json.dump(2); // Pretty print with 2-space indent
  } catch (const nlohmann::json::exception &e) {
    std::cerr << "Warning: Error serializing settings: " << e.what() << "\n";
    return false;
  }

  return true;
}

bool Settings::saveDefault() const {
  if (!AppPaths::ensureAppDataDir()) {
    return false;
  }

  auto path = AppPaths::settingsFilePath();
  if (!path) {
    return false;
  }
  return save(*path);
}

} // namespace w3d
