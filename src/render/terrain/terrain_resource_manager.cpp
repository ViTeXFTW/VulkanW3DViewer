#include "terrain_resource_manager.hpp"

#include <string>
#include <vector>

namespace w3d::terrain {

bool TerrainResourceManager::loadTerrainTypesFromINI(const std::string &iniContent,
                                                     std::string *outError) {
  terrainTypes_.clear();

  try {
    terrainTypes_.loadFromINI(iniContent);
    initialized_ = true;
    return true;
  } catch (const std::exception &e) {
    if (outError) {
      *outError = std::string("Failed to parse Terrain.ini: ") + e.what();
    }
    return false;
  }
}

bool TerrainResourceManager::loadTerrainTypesFromBig(w3d::big::BigArchiveManager &bigManager,
                                                     std::string *outError) {
  if (!bigManager.isInitialized()) {
    if (outError) {
      *outError = "BigArchiveManager is not initialized";
    }
    return false;
  }

  auto iniData = bigManager.extractToMemory(TERRAIN_INI_PATH, outError);
  if (!iniData.has_value()) {
    if (outError && outError->empty()) {
      *outError = std::string("Failed to extract ") + TERRAIN_INI_PATH + " from BIG archives";
    }
    return false;
  }

  std::string iniContent(iniData->begin(), iniData->end());

  return loadTerrainTypesFromINI(iniContent, outError);
}

std::optional<std::string>
TerrainResourceManager::resolveTexturePath(const std::string &terrainClassName,
                                           std::string *outError) const {
  if (!initialized_) {
    if (outError) {
      *outError = "TerrainResourceManager is not initialized";
    }
    return std::nullopt;
  }

  const auto *terrainType = terrainTypes_.findByName(terrainClassName);
  if (!terrainType) {
    if (outError) {
      *outError = std::string("Terrain class not found: ") + terrainClassName;
    }
    return std::nullopt;
  }

  std::string fullPath = std::string(TERRAIN_TGA_DIR) + terrainType->texture;
  return fullPath;
}

void TerrainResourceManager::clear() {
  terrainTypes_.clear();
  initialized_ = false;
}

} // namespace w3d::terrain
