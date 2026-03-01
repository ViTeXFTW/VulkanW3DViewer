#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "lib/formats/big/big_archive_manager.hpp"
#include "lib/formats/ini/terrain_types.hpp"
#include "lib/formats/map/types.hpp"

namespace w3d::terrain {

struct TgaImage {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<uint8_t> pixels;

  bool isValid() const { return width > 0 && height > 0 && pixels.size() == width * height * 4; }
};

struct TileArrayData {
  uint32_t tileSize = 64;
  uint32_t layerCount = 0;
  std::vector<std::vector<uint8_t>> layers;

  bool isValid() const { return layerCount > 0 && layers.size() == layerCount && tileSize > 0; }
};

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

  // Phase 1.2: Decode a TGA image from raw memory bytes into RGBA pixels
  [[nodiscard]] bool decodeTgaFromMemory(const std::vector<uint8_t> &tgaData, TgaImage &outImage,
                                         std::string *outError = nullptr) const;

  // Phase 1.3: Split an RGBA image into a list of tileSize x tileSize RGBA tiles
  // Returns tiles in row-major order (left-to-right, top-to-bottom)
  [[nodiscard]] std::vector<std::vector<uint8_t>> splitImageIntoTiles(const TgaImage &image,
                                                                      int32_t tileSize) const;

  // Phase 1.2: For each texture class, look up the TGA in the BIG archive,
  // decode it, and split into 64x64 tiles. Returns all tiles as a flat list
  // in the same order as the textureClasses -> tile indices.
  // Returns empty vector if bigManager is not initialized.
  [[nodiscard]] std::vector<std::vector<uint8_t>>
  extractTilesForTextureClasses(const std::vector<map::TextureClass> &textureClasses,
                                w3d::big::BigArchiveManager &bigManager,
                                std::string *outError = nullptr) const;

  // Phase 1.4: Assemble extracted 64x64 RGBA tile bitmaps into a TileArrayData
  // ready to be uploaded to the GPU via TextureManager::createTextureArray().
  // Tiles with incorrect sizes are silently skipped.
  [[nodiscard]] TileArrayData
  buildTileArrayData(const std::vector<std::vector<uint8_t>> &tiles) const;

private:
  bool initialized_ = false;
  ini::TerrainTypeCollection terrainTypes_;

  static constexpr const char *TERRAIN_INI_PATH = "Data/INI/Terrain.ini";
  static constexpr const char *TERRAIN_TGA_DIR = "Art/Terrain/";
};

} // namespace w3d::terrain
