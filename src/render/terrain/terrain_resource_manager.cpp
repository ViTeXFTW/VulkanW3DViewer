#include "terrain_resource_manager.hpp"

#include <algorithm>
#include <cstring>
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

bool TerrainResourceManager::decodeTgaFromMemory(const std::vector<uint8_t> &tgaData,
                                                 TgaImage &outImage, std::string *outError) const {
  constexpr size_t TGA_HEADER_SIZE = 18;
  if (tgaData.size() < TGA_HEADER_SIZE) {
    if (outError) {
      *outError = "TGA data too small to contain a valid header";
    }
    return false;
  }

  uint8_t idLength = tgaData[0];
  uint8_t colorMapType = tgaData[1];
  uint8_t imageType = tgaData[2];
  uint32_t width = tgaData[12] | (static_cast<uint32_t>(tgaData[13]) << 8);
  uint32_t height = tgaData[14] | (static_cast<uint32_t>(tgaData[15]) << 8);
  uint8_t bpp = tgaData[16];
  uint8_t imageDescriptor = tgaData[17];

  if (colorMapType != 0 || (imageType != 2 && imageType != 3)) {
    if (outError) {
      *outError = "Unsupported TGA format (only uncompressed true-color or greyscale supported)";
    }
    return false;
  }

  if (width == 0 || height == 0) {
    if (outError) {
      *outError = "TGA has zero width or height";
    }
    return false;
  }

  size_t bytesPerPixel = bpp / 8;
  size_t pixelDataOffset = TGA_HEADER_SIZE + idLength;
  size_t pixelDataSize = static_cast<size_t>(width) * height * bytesPerPixel;

  if (tgaData.size() < pixelDataOffset + pixelDataSize) {
    if (outError) {
      *outError = "TGA data truncated: not enough pixel data";
    }
    return false;
  }

  const uint8_t *src = tgaData.data() + pixelDataOffset;

  outImage.width = width;
  outImage.height = height;
  outImage.pixels.resize(static_cast<size_t>(width) * height * 4);

  for (size_t i = 0; i < static_cast<size_t>(width) * height; ++i) {
    size_t srcIdx = i * bytesPerPixel;
    size_t dstIdx = i * 4;

    if (bpp == 32) {
      outImage.pixels[dstIdx + 0] = src[srcIdx + 2]; // R (from BGR)
      outImage.pixels[dstIdx + 1] = src[srcIdx + 1]; // G
      outImage.pixels[dstIdx + 2] = src[srcIdx + 0]; // B
      outImage.pixels[dstIdx + 3] = src[srcIdx + 3]; // A
    } else if (bpp == 24) {
      outImage.pixels[dstIdx + 0] = src[srcIdx + 2]; // R (from BGR)
      outImage.pixels[dstIdx + 1] = src[srcIdx + 1]; // G
      outImage.pixels[dstIdx + 2] = src[srcIdx + 0]; // B
      outImage.pixels[dstIdx + 3] = 255;             // A
    } else if (bpp == 8) {
      outImage.pixels[dstIdx + 0] = src[srcIdx];     // R
      outImage.pixels[dstIdx + 1] = src[srcIdx];     // G
      outImage.pixels[dstIdx + 2] = src[srcIdx];     // B
      outImage.pixels[dstIdx + 3] = 255;             // A
    } else {
      if (outError) {
        *outError = std::string("Unsupported TGA bit depth: ") + std::to_string(bpp);
      }
      return false;
    }
  }

  bool flipVertical = (imageDescriptor & 0x20) == 0;
  if (flipVertical) {
    size_t rowSize = static_cast<size_t>(width) * 4;
    std::vector<uint8_t> flipped(outImage.pixels.size());
    for (uint32_t y = 0; y < height; ++y) {
      std::memcpy(&flipped[y * rowSize], &outImage.pixels[(height - 1 - y) * rowSize], rowSize);
    }
    outImage.pixels = std::move(flipped);
  }

  return true;
}

std::vector<std::vector<uint8_t>>
TerrainResourceManager::splitImageIntoTiles(const TgaImage &image, int32_t tileSize) const {
  if (!image.isValid() || tileSize <= 0) {
    return {};
  }

  int32_t tilesX = static_cast<int32_t>(image.width) / tileSize;
  int32_t tilesY = static_cast<int32_t>(image.height) / tileSize;

  if (tilesX <= 0 || tilesY <= 0) {
    return {};
  }

  size_t tileByteSize = static_cast<size_t>(tileSize) * tileSize * 4;
  std::vector<std::vector<uint8_t>> tiles;
  tiles.reserve(static_cast<size_t>(tilesX * tilesY));

  for (int32_t ty = 0; ty < tilesY; ++ty) {
    for (int32_t tx = 0; tx < tilesX; ++tx) {
      std::vector<uint8_t> tile(tileByteSize);

      for (int32_t row = 0; row < tileSize; ++row) {
        int32_t srcY = ty * tileSize + row;
        int32_t srcX = tx * tileSize;
        size_t srcIdx = (static_cast<size_t>(srcY) * image.width + static_cast<size_t>(srcX)) * 4;
        size_t dstIdx = static_cast<size_t>(row) * static_cast<size_t>(tileSize) * 4;
        std::memcpy(tile.data() + dstIdx, image.pixels.data() + srcIdx,
                    static_cast<size_t>(tileSize) * 4);
      }

      tiles.push_back(std::move(tile));
    }
  }

  return tiles;
}

std::vector<std::vector<uint8_t>> TerrainResourceManager::extractTilesForTextureClasses(
    const std::vector<map::TextureClass> &textureClasses, w3d::big::BigArchiveManager &bigManager,
    std::string *outError) const {
  if (textureClasses.empty()) {
    return {};
  }

  if (!bigManager.isInitialized()) {
    if (outError) {
      *outError = "BigArchiveManager is not initialized";
    }
    return {};
  }

  // Compute the total number of source tile slots required.
  // tileNdxes[] values index into a flat array where each TextureClass occupies
  // slots [firstTile, firstTile + numTiles). We must honour firstTile exactly so
  // that decoded tile indices land on the correct GPU texture array layer.
  // This mirrors the original engine's m_sourceTiles[] layout (WorldHeightMap::readTexClass).
  int32_t totalSlots = 0;
  for (const auto &tc : textureClasses) {
    int32_t end = tc.firstTile + tc.numTiles;
    if (end > totalSlots) {
      totalSlots = end;
    }
  }

  if (totalSlots <= 0) {
    return {};
  }

  // Pre-fill every slot with a checkerboard fallback. Texture classes that load
  // successfully will overwrite their slots below; gaps (if any) stay as fallbacks.
  auto fallbackTile = generateCheckerboardTile(map::TILE_PIXEL_EXTENT);
  std::vector<std::vector<uint8_t>> allTiles(static_cast<size_t>(totalSlots), fallbackTile);

  std::string missingTextures;

  for (const auto &tc : textureClasses) {
    if (tc.numTiles <= 0 || tc.firstTile < 0 || tc.firstTile >= totalSlots) {
      continue;
    }

    std::string extractError;
    bool loaded = false;

    auto tgaPath = resolveTexturePath(tc.name, &extractError);
    if (tgaPath.has_value()) {
      auto tgaData = bigManager.extractToMemory(tgaPath.value(), &extractError);
      if (tgaData.has_value()) {
        TgaImage img;
        if (decodeTgaFromMemory(tgaData.value(), img, &extractError)) {
          auto tiles = splitImageIntoTiles(img, map::TILE_PIXEL_EXTENT);
          if (!tiles.empty()) {
            // Clamp to numTiles: the original engine (WorldHeightMap::readTexClass)
            // explicitly limits tile count to texClass->numTiles, ensuring subsequent
            // classes are never displaced by an oversized TGA.
            int32_t tilesToPlace = std::min(static_cast<int32_t>(tiles.size()), tc.numTiles);

            // Place tiles at their absolute firstTile positions so that
            // tileNdxes[] decoded indices map to the correct allTiles[] layer.
            for (int32_t i = 0; i < tilesToPlace; ++i) {
              allTiles[static_cast<size_t>(tc.firstTile + i)] =
                  std::move(tiles[static_cast<size_t>(i)]);
            }
            loaded = true;
          }
        }
      }
    }

    if (!loaded) {
      if (!missingTextures.empty()) {
        missingTextures += ", ";
      }
      missingTextures += tc.name;
      // Slots already contain fallback tiles from the pre-fill above.
    }
  }

  if (!missingTextures.empty() && outError) {
    *outError = "Missing terrain textures (using fallback): " + missingTextures;
  }

  return allTiles;
}

std::vector<uint8_t> TerrainResourceManager::generateCheckerboardTile(int32_t tileSize) const {
  if (tileSize <= 0) {
    return {};
  }

  std::vector<uint8_t> tile(static_cast<size_t>(tileSize) * tileSize * 4);
  constexpr int32_t CHECKER_SIZE = 8;

  for (int32_t y = 0; y < tileSize; ++y) {
    for (int32_t x = 0; x < tileSize; ++x) {
      bool isMagenta = ((x / CHECKER_SIZE) + (y / CHECKER_SIZE)) % 2 == 0;
      size_t idx = (static_cast<size_t>(y) * tileSize + x) * 4;
      if (isMagenta) {
        tile[idx + 0] = 255; // R
        tile[idx + 1] = 0;   // G
        tile[idx + 2] = 255; // B
        tile[idx + 3] = 255; // A
      } else {
        tile[idx + 0] = 0;   // R
        tile[idx + 1] = 0;   // G
        tile[idx + 2] = 0;   // B
        tile[idx + 3] = 255; // A
      }
    }
  }

  return tile;
}

TileArrayData
TerrainResourceManager::buildTileArrayData(const std::vector<std::vector<uint8_t>> &tiles) const {
  constexpr uint32_t EXPECTED_TILE_SIZE = map::TILE_PIXEL_EXTENT;
  constexpr size_t EXPECTED_BYTES = EXPECTED_TILE_SIZE * EXPECTED_TILE_SIZE * 4;

  TileArrayData data;
  data.tileSize = EXPECTED_TILE_SIZE;

  for (const auto &tile : tiles) {
    if (tile.size() != EXPECTED_BYTES) {
      continue;
    }
    data.layers.push_back(tile);
  }

  data.layerCount = static_cast<uint32_t>(data.layers.size());
  return data;
}

} // namespace w3d::terrain
