#pragma once

#include <optional>
#include <string>

#include "../../lib/formats/big/big_archive_manager.hpp"
#include "../../lib/formats/ini/terrain_types.hpp"

namespace w3d::terrain {

class TerrainResourceManager {
public:
  TerrainResourceManager() = default;
  ~TerrainResourceManager() = default;

  TerrainResourceManager(const TerrainResourceManager &) = delete;
  TerrainResourceManager &operator=(const TerrainResourceManager &) = delete;
  TerrainResourceManager(TerrainResourceManager &&) noexcept = default;
  TerrainResourceManager &operator=(TerrainResourceManager &&) noexcept = default;

  [[nodiscard]] bool loadTerrainTypesFromINI(const std::string &iniContent,
                                             std::string *outError = nullptr);

  [[nodiscard]] bool loadTerrainTypesFromBig(w3d::big::BigArchiveManager &bigManager,
                                             std::string *outError = nullptr);

  [[nodiscard]] std::optional<std::string>
  resolveTexturePath(const std::string &terrainClassName, std::string *outError = nullptr) const;

  [[nodiscard]] bool isInitialized() const { return initialized_; }

  [[nodiscard]] const ini::TerrainTypeCollection &getTerrainTypes() const { return terrainTypes_; }

  void clear();

private:
  bool initialized_ = false;
  ini::TerrainTypeCollection terrainTypes_;

  static constexpr const char *TERRAIN_INI_PATH = "Data/INI/Terrain.ini";
  static constexpr const char *TERRAIN_TGA_DIR = "Art/Terrain/";
};

} // namespace w3d::terrain
