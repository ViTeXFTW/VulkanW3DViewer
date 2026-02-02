#include "app_paths.hpp"

#include <cstdlib>
#include <iostream>

namespace w3d {

std::optional<std::filesystem::path> AppPaths::baseConfigDir() {
#ifdef _WIN32
  // Windows: Use APPDATA environment variable
  if (const char* appdata = std::getenv("APPDATA")) {
    return std::filesystem::path(appdata);
  }
  // Fallback: Use USERPROFILE
  if (const char* userprofile = std::getenv("USERPROFILE")) {
    return std::filesystem::path(userprofile) / "AppData" / "Roaming";
  }
  return std::nullopt;

#elif defined(__APPLE__)
  // macOS: ~/Library/Application Support
  if (const char* home = std::getenv("HOME")) {
    return std::filesystem::path(home) / "Library" / "Application Support";
  }
  return std::nullopt;

#else
  // Linux/BSD: XDG_CONFIG_HOME or ~/.config
  if (const char* xdgConfig = std::getenv("XDG_CONFIG_HOME")) {
    return std::filesystem::path(xdgConfig);
  }
  if (const char* home = std::getenv("HOME")) {
    return std::filesystem::path(home) / ".config";
  }
  return std::nullopt;
#endif
}

std::optional<std::filesystem::path> AppPaths::appDataDir() {
  auto base = baseConfigDir();
  if (!base) {
    return std::nullopt;
  }
  return *base / kAppName;
}

std::optional<std::filesystem::path> AppPaths::imguiIniPath() {
  auto dir = appDataDir();
  if (!dir) {
    return std::nullopt;
  }
  return *dir / "imgui.ini";
}

std::optional<std::filesystem::path> AppPaths::settingsFilePath() {
  auto dir = appDataDir();
  if (!dir) {
    return std::nullopt;
  }
  return *dir / "settings.json";
}

bool AppPaths::ensureAppDataDir() {
  auto dir = appDataDir();
  if (!dir) {
    std::cerr << "Warning: Could not determine application data directory\n";
    return false;
  }

  if (std::filesystem::exists(*dir)) {
    return true;
  }

  std::error_code ec;
  if (!std::filesystem::create_directories(*dir, ec)) {
    std::cerr << "Warning: Could not create application data directory: "
              << dir->string() << " (" << ec.message() << ")\n";
    return false;
  }

  return true;
}

} // namespace w3d
