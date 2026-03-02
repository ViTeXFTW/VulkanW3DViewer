#pragma once

#include <cstdint>
#include <vector>

#include "lib/formats/map/types.hpp"

namespace w3d::terrain {

constexpr int32_t BLEND_PATTERN_SIZE = 64;
constexpr int32_t NUM_BLEND_PATTERNS = 12;

enum class BlendDirection : int32_t {
  Horizontal = 0,
  HorizontalInv = 1,
  Vertical = 2,
  VerticalInv = 3,
  DiagonalRight = 4,
  DiagonalRightInv = 5,
  DiagonalLeft = 6,
  DiagonalLeftInv = 7,
  LongDiagonal = 8,
  LongDiagonalInv = 9,
  LongDiagonalAlt = 10,
  LongDiagonalAltInv = 11,
};

struct BlendPattern {
  int32_t size = BLEND_PATTERN_SIZE;
  std::vector<uint8_t> alpha;
};

[[nodiscard]] BlendPattern generateBlendPattern(BlendDirection direction);

[[nodiscard]] std::vector<BlendPattern> generateAllBlendPatterns();

[[nodiscard]] BlendDirection blendDirectionFromInfo(const map::BlendTileInfo &info);

[[nodiscard]] bool cellHasBlend(const map::BlendTileInfo &info);

} // namespace w3d::terrain
