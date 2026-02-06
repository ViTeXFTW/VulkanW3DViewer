#include "asset_registry.hpp"

#include <bigx/archive.hpp>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <system_error>

#include "core/app_paths.hpp"

namespace w3d::big {

namespace {
  // Known BIG archive files for C&C Generals
  constexpr const char *kBigArchives[] = {
      "W3DZH.big",      // Models
      "TexturesZH.big", // Textures
      "INIZH.big",      // Configuration
      "TerrainZH.big",  // Terrain data
      "MapsZH.big"      // Map files
  };

  // Archive path prefixes
  constexpr const char *kModelPathPrefix = "Art/W3D/";
  constexpr const char *kTexturePathPrefix = "Art/Textures/";

  // Supported extensions
  constexpr const char *kModelExtension = ".w3d";
  constexpr const char *kTextureExtensions[] = {".dds", ".tga"};
  constexpr const char *kIniExtension = ".ini";
} // namespace

bool AssetRegistry::setupCacheDirectory(std::string *outError) {
  auto appDataDir = AppPaths::appDataDir();
  if (!appDataDir) {
    if (outError) {
      *outError = "Failed to get application data directory";
    }
    return false;
  }

  cacheDirectory_ = *appDataDir / "big_cache";

  // Create cache directory if it doesn't exist
  std::error_code ec;
  if (!std::filesystem::exists(cacheDirectory_, ec)) {
    if (!std::filesystem::create_directories(cacheDirectory_, ec)) {
      if (outError) {
        *outError = "Failed to create cache directory: " + ec.message();
      }
      return false;
    }
  }

  // Add cache subdirectories to search paths
  searchPaths_.push_back(cacheDirectory_ / "Art" / "W3D");
  searchPaths_.push_back(cacheDirectory_ / "Art" / "Textures");

  return true;
}

bool AssetRegistry::scanArchives(const std::filesystem::path &gameDirectory,
                                  std::string *outError) {
  clear();

  gameDirectory_ = gameDirectory;

  // Verify game directory exists
  std::error_code ec;
  if (!std::filesystem::exists(gameDirectory_, ec)) {
    if (outError) {
      *outError = "Game directory does not exist: " + gameDirectory_.string();
    }
    return false;
  }

  // Set up cache directory
  if (!setupCacheDirectory(outError)) {
    return false;
  }

  // Scan each known BIG archive
  size_t archivesFound = 0;
  for (const char *archiveName : kBigArchives) {
    std::filesystem::path archivePath = gameDirectory_ / archiveName;
    if (std::filesystem::exists(archivePath, ec)) {
      if (scanArchive(archivePath, archiveName, outError)) {
        archivesFound++;
      }
    }
  }

  if (archivesFound == 0) {
    if (outError) {
      *outError = "No BIG archives found in game directory";
    }
    return false;
  }

  scanned_ = true;
  return true;
}

bool AssetRegistry::scanArchive(const std::filesystem::path &archivePath,
                                 const std::string &archiveName,
                                 std::string *outError) {
  std::string error;
  auto archive = ::big::Archive::open(archivePath, &error);

  if (!archive) {
    if (outError) {
      *outError = "Failed to open " + archiveName + ": " + error;
    }
    return false;
  }

  // Scan files in archive
  for (const auto &file : archive->files()) {
    std::string path = file.path;

    // Check for model files
    if (path.find(kModelPathPrefix) == 0) {
      std::string filename = path.substr(std::strlen(kModelPathPrefix));

      // Only process .w3d files
      if (filename.length() > 4 &&
          filename.substr(filename.length() - 4) == kModelExtension) {
        std::string modelName = filename.substr(0, filename.length() - 4); // Remove .w3d
        modelName = normalizeAssetName(modelName);

        if (modelArchivePaths_.find(modelName) == modelArchivePaths_.end()) {
          modelArchivePaths_[modelName] = path;
          availableModels_.push_back(modelName);
        }
      }
    }

    // Check for texture files
    if (path.find(kTexturePathPrefix) == 0) {
      std::string filename = path.substr(std::strlen(kTexturePathPrefix));

      // Check for supported texture extensions
      for (const char *ext : kTextureExtensions) {
        size_t extLen = std::strlen(ext);
        if (filename.length() > extLen &&
            filename.substr(filename.length() - extLen) == ext) {
          std::string textureName = filename.substr(0, filename.length() - extLen);
          textureName = normalizeAssetName(textureName);

          if (textureArchivePaths_.find(textureName) == textureArchivePaths_.end()) {
            textureArchivePaths_[textureName] = path;
            availableTextures_.push_back(textureName);
          }
          break;
        }
      }
    }

    // Check for INI files
    if (path.length() > 4 && path.substr(path.length() - 4) == kIniExtension) {
      std::string iniName = path.substr(0, path.length() - 4); // Remove .ini
      if (std::find(availableIniFiles_.begin(), availableIniFiles_.end(), iniName) ==
          availableIniFiles_.end()) {
        availableIniFiles_.push_back(iniName);
      }
    }
  }

  return true;
}

void AssetRegistry::addSearchPath(const std::filesystem::path &path) {
  if (std::find(searchPaths_.begin(), searchPaths_.end(), path) == searchPaths_.end()) {
    searchPaths_.push_back(path);
  }
}

void AssetRegistry::removeSearchPath(const std::filesystem::path &path) {
  auto it = std::find(searchPaths_.begin(), searchPaths_.end(), path);
  if (it != searchPaths_.end()) {
    searchPaths_.erase(it);
  }
}

void AssetRegistry::clear() {
  scanned_ = false;
  gameDirectory_.clear();
  cacheDirectory_.clear();
  searchPaths_.clear();
  availableModels_.clear();
  availableTextures_.clear();
  availableIniFiles_.clear();
  modelArchivePaths_.clear();
  textureArchivePaths_.clear();
}

std::string AssetRegistry::getModelArchivePath(const std::string &modelName) const {
  std::string normalizedName = normalizeAssetName(modelName);
  auto it = modelArchivePaths_.find(normalizedName);
  if (it != modelArchivePaths_.end()) {
    return it->second;
  }
  return "";
}

std::string AssetRegistry::getTextureArchivePath(const std::string &textureName) const {
  std::string normalizedName = normalizeAssetName(textureName);
  auto it = textureArchivePaths_.find(normalizedName);
  if (it != textureArchivePaths_.end()) {
    return it->second;
  }
  return "";
}

std::string AssetRegistry::normalizeAssetName(const std::string &name) {
  std::string normalized = name;

  // Convert to lowercase
  for (char &c : normalized) {
    if (c >= 'A' && c <= 'Z') {
      c = c - 'A' + 'a';
    }
  }

  return normalized;
}

} // namespace w3d::big
