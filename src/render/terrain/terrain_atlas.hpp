#pragma once

#include <cstdint>
#include <vector>

#include "lib/formats/map/types.hpp"

namespace w3d::terrain {

struct TileUV {
  float u = 0.0f;
  float v = 0.0f;
  float uSize = 0.0f;
  float vSize = 0.0f;
};

struct TerrainAtlasData {
  int32_t atlasWidth = 0;
  int32_t atlasHeight = 0;
  int32_t tilePixelSize = 64;
  int32_t tilesPerRow = 0;
  std::vector<uint8_t> pixels;
  std::vector<TileUV> tileUVs;

  bool isValid() const { return atlasWidth > 0 && atlasHeight > 0 && !pixels.empty(); }
};

[[nodiscard]] int32_t decodeTileIndex(int16_t tileNdx);

[[nodiscard]] int32_t decodeQuadrant(int16_t tileNdx);

[[nodiscard]] TileUV computeQuadrantUV(const TileUV &tileUV, int32_t quadrant);

[[nodiscard]] TileUV decodeTileNdxUV(int16_t tileNdx, const std::vector<TileUV> &tileUVs);

[[nodiscard]] std::vector<TileUV> computeTileUVTable(
    const std::vector<map::TextureClass> &textureClasses, int32_t atlasWidth = 2048,
    int32_t tilePixelSize = 64);

[[nodiscard]] TerrainAtlasData buildProceduralAtlas(int32_t numTiles, int32_t atlasWidth = 2048,
                                                    int32_t tilePixelSize = 64);

} // namespace w3d::terrain
