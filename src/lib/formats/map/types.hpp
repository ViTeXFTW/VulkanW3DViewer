#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace map {

constexpr float MAP_XY_FACTOR = 10.0f;
constexpr float MAP_HEIGHT_SCALE = MAP_XY_FACTOR / 16.0f;

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

} // namespace map
