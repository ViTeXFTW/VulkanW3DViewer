#include "asset_registry.hpp"

#include <bigx/archive.hpp>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <system_error>

#include "core/app_paths.hpp"
#include "core/debug.hpp"

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

  // First, scan the well-known archives from the root directory
  size_t archivesFound = 0;
  for (const char *archiveName : kBigArchives) {
    std::filesystem::path archivePath = gameDirectory_ / archiveName;
    if (std::filesystem::exists(archivePath, ec)) {
      if (scanArchive(archivePath, archiveName, outError)) {
        archivesFound++;
      }
    }
  }

  // Then, recursively search for any additional .big files in subdirectories
  [[maybe_unused]] size_t additionalCount = 0;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(gameDirectory_, ec)) {
    if (ec) {
      continue; // Skip directories we can't access
    }

    if (entry.is_regular_file(ec)) {
      std::filesystem::path path = entry.path();
      if (path.extension() == ".big") {
        // Get relative path from game directory for display name
        std::filesystem::path relativePath = std::filesystem::relative(path, gameDirectory_);
        std::string displayName = relativePath.string();

        // Convert backslashes to forward slashes for consistency
        for (char &c : displayName) {
          if (c == '\\') {
            c = '/';
          }
        }

        // Skip if already scanned (the well-known archives above)
        bool alreadyScanned = false;
        for (const char *archiveName : kBigArchives) {
          if (displayName == archiveName) {
            alreadyScanned = true;
            break;
          }
        }

        if (alreadyScanned) {
          continue;
        }

        // Scan this additional archive
        if (scanArchive(path, displayName, outError)) {
          archivesFound++;
          additionalCount++;
        }
      }
    }
  }

  LOG_DEBUG("[AssetRegistry] Total archives scanned: " << archivesFound
            << " (" << additionalCount << " additional)\n");

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

  size_t modelsFound = 0;
  [[maybe_unused]] size_t texturesFound = 0;
  [[maybe_unused]] size_t iniFilesFound = 0;

  // Scan files in archive
  for (const auto &file : archive->files()) {
    // Use lowercase path for case-insensitive extension matching
    // (original case is preserved in file.path, but many archives have uppercase extensions)
    const std::string &path = file.lowercasePath;

    // Check for .w3d model files anywhere in the archive
    if (path.length() > 4 &&
        path.substr(path.length() - 4) == kModelExtension) {
      // Use the original path for archive lookups, but lowercase for the model name
      std::string originalPath = file.path;
      std::string modelName = path.substr(0, path.length() - 4); // Remove .w3d

      // Only add if not already present (avoid duplicates across archives)
      if (modelArchivePaths_.find(modelName) == modelArchivePaths_.end()) {
        modelArchivePaths_[modelName] = originalPath;
        availableModels_.push_back(modelName);
        modelsFound++;
      }
      // Log when model is skipped due to duplicate
      // (only log a few samples to avoid spam)
      else if (modelsFound < 5 || modelName.find("tank") != std::string::npos) {
        LOG_DEBUG("[AssetRegistry] Skipped duplicate: " << modelName
                  << " (original: " << modelArchivePaths_[modelName] << ")\n");
      }
    }

    // Check for texture files
    for (const char *ext : kTextureExtensions) {
      size_t extLen = std::strlen(ext);
      if (path.length() > extLen &&
          path.substr(path.length() - extLen) == ext) {
        std::string originalPath = file.path;
        std::string textureName = path.substr(0, path.length() - extLen);

        // Only add if not already present
        if (textureArchivePaths_.find(textureName) == textureArchivePaths_.end()) {
          textureArchivePaths_[textureName] = originalPath;
          availableTextures_.push_back(textureName);
          texturesFound++;
        }
        break;
      }
    }

    // Check for INI files
    if (path.length() > 4 && path.substr(path.length() - 4) == kIniExtension) {
      std::string iniName = path.substr(0, path.length() - 4); // Remove .ini
      if (std::find(availableIniFiles_.begin(), availableIniFiles_.end(), iniName) ==
          availableIniFiles_.end()) {
        availableIniFiles_.push_back(iniName);
        iniFilesFound++;
      }
    }
  }

  // Debug output
  LOG_DEBUG("[AssetRegistry] Scanned " << archiveName << ": "
            << modelsFound << " models, " << texturesFound << " textures, "
            << iniFilesFound << " INI files\n");

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
