#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace map {

class DataChunkReader;

constexpr float MAP_XY_FACTOR = 10.0f;
constexpr float MAP_HEIGHT_SCALE = MAP_XY_FACTOR / 16.0f;

constexpr int32_t FLAG_VAL = 0x7ADA0000;
constexpr uint8_t INVERTED_MASK = 0x1;
constexpr uint8_t FLIPPED_MASK = 0x2;
constexpr int32_t TILE_PIXEL_EXTENT = 64;

struct HeightMap {
  int32_t width = 0;
  int32_t height = 0;
  int32_t borderSize = 0;
  std::vector<glm::ivec2> boundaries;
  std::vector<uint8_t> data;

  float getWorldHeight(int32_t x, int32_t y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
      return 0.0f;
    }
    return data[y * width + x] * MAP_HEIGHT_SCALE;
  }

  void setHeight(int32_t x, int32_t y, uint8_t value) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
      data[y * width + x] = value;
    }
  }

  uint8_t getHeight(int32_t x, int32_t y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
      return 0;
    }
    return data[y * width + x];
  }

  bool isValid() const {
    return width > 0 && height > 0 && static_cast<int32_t>(data.size()) == width * height;
  }
};

struct TextureClass {
  int32_t firstTile = 0;
  int32_t numTiles = 0;
  int32_t width = 0;
  std::string name;
};

struct BlendTileInfo {
  int32_t blendNdx = 0;
  int8_t horiz = 0;
  int8_t vert = 0;
  int8_t rightDiagonal = 0;
  int8_t leftDiagonal = 0;
  int8_t inverted = 0;
  int8_t longDiagonal = 0;
  int32_t customBlendEdgeClass = -1;
};

struct CliffInfo {
  int32_t tileIndex = 0;
  float u0 = 0.0f, v0 = 0.0f;
  float u1 = 0.0f, v1 = 0.0f;
  float u2 = 0.0f, v2 = 0.0f;
  float u3 = 0.0f, v3 = 0.0f;
  int8_t flip = 0;
  int8_t mutant = 0;
};

struct BlendTileData {
  int32_t dataSize = 0;
  std::vector<int16_t> tileNdxes;
  std::vector<int16_t> blendTileNdxes;
  std::vector<int16_t> extraBlendTileNdxes;
  std::vector<int16_t> cliffInfoNdxes;
  std::vector<uint8_t> cellCliffState;

  int32_t numBitmapTiles = 0;
  int32_t numBlendedTiles = 0;
  int32_t numCliffInfo = 0;

  std::vector<TextureClass> textureClasses;
  int32_t numEdgeTiles = 0;
  std::vector<TextureClass> edgeTextureClasses;
  std::vector<BlendTileInfo> blendTileInfos;
  std::vector<CliffInfo> cliffInfos;

  bool isValid() const {
    return dataSize > 0 && static_cast<int32_t>(tileNdxes.size()) == dataSize &&
           static_cast<int32_t>(blendTileNdxes.size()) == dataSize;
  }
};

enum MapObjectFlags : uint32_t {
  FLAG_DRAWS_IN_MIRROR = 0x001,
  FLAG_ROAD_POINT1 = 0x002,
  FLAG_ROAD_POINT2 = 0x004,
  FLAG_ROAD_CORNER_ANGLED = 0x008,
  FLAG_BRIDGE_POINT1 = 0x010,
  FLAG_BRIDGE_POINT2 = 0x020,
  FLAG_ROAD_CORNER_TIGHT = 0x040,
  FLAG_ROAD_JOIN = 0x080,
  FLAG_DONT_RENDER = 0x100
};

struct DictValue;
using Dict = std::unordered_map<std::string, DictValue>;

struct MapObject {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  float angle = 0.0f;
  uint32_t flags = 0;
  std::string templateName;
  Dict properties;

  bool isRoadPoint() const { return (flags & (FLAG_ROAD_POINT1 | FLAG_ROAD_POINT2)) != 0; }

  bool isBridgePoint() const { return (flags & (FLAG_BRIDGE_POINT1 | FLAG_BRIDGE_POINT2)) != 0; }

  bool shouldRender() const { return (flags & FLAG_DONT_RENDER) == 0; }
};

} // namespace map
