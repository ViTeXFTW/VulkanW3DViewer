#include "render/terrain/terrain_atlas.hpp"

#include <algorithm>
#include <cmath>

namespace w3d::terrain {

int32_t decodeTileIndex(int16_t tileNdx) {
  return static_cast<int32_t>((static_cast<uint16_t>(tileNdx) >> 2) & 0x3FFF);
}

int32_t decodeQuadrant(int16_t tileNdx) {
  return static_cast<int32_t>(static_cast<uint16_t>(tileNdx) & 0x3);
}

TileUV computeQuadrantUV(const TileUV &tileUV, int32_t quadrant) {
  TileUV result = tileUV;
  result.uSize = tileUV.uSize * 0.5f;
  result.vSize = tileUV.vSize * 0.5f;

  if (quadrant == 1 || quadrant == 3) {
    result.u = tileUV.u + result.uSize;
  }
  if (quadrant == 2 || quadrant == 3) {
    result.v = tileUV.v + result.vSize;
  }
  return result;
}

TileUV decodeTileNdxUV(int16_t tileNdx, const std::vector<TileUV> &tileUVs) {
  int32_t tileIndex = decodeTileIndex(tileNdx);
  int32_t quadrant = decodeQuadrant(tileNdx);

  if (tileIndex < 0 || static_cast<size_t>(tileIndex) >= tileUVs.size()) {
    return TileUV{};
  }

  return computeQuadrantUV(tileUVs[static_cast<size_t>(tileIndex)], quadrant);
}

std::vector<TileUV> computeTileUVTable(const std::vector<map::TextureClass> &textureClasses,
                                       int32_t atlasWidth, int32_t tilePixelSize) {
  if (textureClasses.empty() || atlasWidth <= 0 || tilePixelSize <= 0) {
    return {};
  }

  int32_t tilesPerRow = atlasWidth / tilePixelSize;
  if (tilesPerRow <= 0) {
    return {};
  }

  int32_t totalTiles = 0;
  for (const auto &tc : textureClasses) {
    totalTiles += tc.numTiles;
  }

  if (totalTiles <= 0) {
    return {};
  }

  int32_t totalRows = (totalTiles + tilesPerRow - 1) / tilesPerRow;
  int32_t atlasHeight = totalRows * tilePixelSize;

  float uStep = static_cast<float>(tilePixelSize) / static_cast<float>(atlasWidth);
  float vStep = static_cast<float>(tilePixelSize) / static_cast<float>(atlasHeight);

  std::vector<TileUV> result;
  result.reserve(static_cast<size_t>(totalTiles));

  int32_t tileIdx = 0;
  for (const auto &tc : textureClasses) {
    for (int32_t i = 0; i < tc.numTiles; ++i) {
      int32_t col = tileIdx % tilesPerRow;
      int32_t row = tileIdx / tilesPerRow;

      TileUV uv;
      uv.u = static_cast<float>(col) * uStep;
      uv.v = static_cast<float>(row) * vStep;
      uv.uSize = uStep;
      uv.vSize = vStep;
      result.push_back(uv);

      ++tileIdx;
    }
  }

  return result;
}

TerrainAtlasData buildProceduralAtlas(int32_t numTiles, int32_t atlasWidth,
                                      int32_t tilePixelSize) {
  if (numTiles <= 0 || atlasWidth <= 0 || tilePixelSize <= 0) {
    return {};
  }

  int32_t tilesPerRow = atlasWidth / tilePixelSize;
  if (tilesPerRow <= 0) {
    return {};
  }

  int32_t totalRows = (numTiles + tilesPerRow - 1) / tilesPerRow;
  int32_t atlasHeight = totalRows * tilePixelSize;

  TerrainAtlasData data;
  data.atlasWidth = atlasWidth;
  data.atlasHeight = atlasHeight;
  data.tilePixelSize = tilePixelSize;
  data.tilesPerRow = tilesPerRow;

  data.pixels.resize(static_cast<size_t>(atlasWidth * atlasHeight * 4), 0);

  float uStep = static_cast<float>(tilePixelSize) / static_cast<float>(atlasWidth);
  float vStep = static_cast<float>(tilePixelSize) / static_cast<float>(atlasHeight);

  data.tileUVs.reserve(static_cast<size_t>(numTiles));

  for (int32_t t = 0; t < numTiles; ++t) {
    int32_t col = t % tilesPerRow;
    int32_t row = t / tilesPerRow;

    uint8_t r = static_cast<uint8_t>((t * 37 + 50) & 0xFF);
    uint8_t g = static_cast<uint8_t>((t * 73 + 100) & 0xFF);
    uint8_t b = static_cast<uint8_t>((t * 113 + 150) & 0xFF);

    int32_t startPx = col * tilePixelSize;
    int32_t startPy = row * tilePixelSize;

    for (int32_t py = startPy; py < startPy + tilePixelSize && py < atlasHeight; ++py) {
      for (int32_t px = startPx; px < startPx + tilePixelSize && px < atlasWidth; ++px) {
        size_t idx = static_cast<size_t>((py * atlasWidth + px) * 4);
        data.pixels[idx + 0] = r;
        data.pixels[idx + 1] = g;
        data.pixels[idx + 2] = b;
        data.pixels[idx + 3] = 255;
      }
    }

    TileUV uv;
    uv.u = static_cast<float>(col) * uStep;
    uv.v = static_cast<float>(row) * vStep;
    uv.uSize = uStep;
    uv.vSize = vStep;
    data.tileUVs.push_back(uv);
  }

  return data;
}

} // namespace w3d::terrain
