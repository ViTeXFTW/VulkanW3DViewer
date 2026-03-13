#include "render/terrain/terrain_blend.hpp"

#include <algorithm>
#include <cmath>

namespace w3d::terrain {

BlendPattern generateBlendPattern(BlendDirection direction) {
  BlendPattern pattern;
  pattern.size = BLEND_PATTERN_SIZE;
  pattern.alpha.resize(static_cast<size_t>(BLEND_PATTERN_SIZE * BLEND_PATTERN_SIZE), 0);

  for (int32_t y = 0; y < BLEND_PATTERN_SIZE; ++y) {
    for (int32_t x = 0; x < BLEND_PATTERN_SIZE; ++x) {
      float nx = static_cast<float>(x) / static_cast<float>(BLEND_PATTERN_SIZE - 1);
      float ny = static_cast<float>(y) / static_cast<float>(BLEND_PATTERN_SIZE - 1);
      float value = 0.0f;

      switch (direction) {
      case BlendDirection::Horizontal:
        value = nx;
        break;
      case BlendDirection::HorizontalInv:
        value = 1.0f - nx;
        break;
      case BlendDirection::Vertical:
        value = ny;
        break;
      case BlendDirection::VerticalInv:
        value = 1.0f - ny;
        break;
      case BlendDirection::DiagonalRight:
        value = std::clamp(nx + ny - 1.0f, 0.0f, 1.0f);
        break;
      case BlendDirection::DiagonalRightInv:
        value = std::clamp(nx - ny, 0.0f, 1.0f);
        break;
      case BlendDirection::DiagonalLeft:
        value = std::clamp(ny - nx, 0.0f, 1.0f);
        break;
      case BlendDirection::DiagonalLeftInv:
        value = std::clamp(1.0f - nx - ny, 0.0f, 1.0f);
        break;
      case BlendDirection::LongDiagonalRight:
        value = std::clamp(nx + ny, 0.0f, 1.0f);
        break;
      case BlendDirection::LongDiagonalRightInv:
        value = std::clamp(1.0f + nx - ny, 0.0f, 1.0f);
        break;
      case BlendDirection::LongDiagonalLeft:
        value = std::clamp(1.0f - nx + ny, 0.0f, 1.0f);
        break;
      case BlendDirection::LongDiagonalLeftInv:
        value = std::clamp(2.0f - nx - ny, 0.0f, 1.0f);
        break;
      }

      size_t idx = static_cast<size_t>(y * BLEND_PATTERN_SIZE + x);
      pattern.alpha[idx] = static_cast<uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
    }
  }

  return pattern;
}

std::vector<BlendPattern> generateAllBlendPatterns() {
  std::vector<BlendPattern> patterns;
  patterns.reserve(NUM_BLEND_PATTERNS);

  for (int32_t i = 0; i < NUM_BLEND_PATTERNS; ++i) {
    patterns.push_back(generateBlendPattern(static_cast<BlendDirection>(i)));
  }

  return patterns;
}

BlendDirection blendDirectionFromInfo(const map::BlendTileInfo &info) {
  bool inverted = (info.inverted & map::INVERTED_MASK) != 0;

  if (info.horiz != 0) {
    return inverted ? BlendDirection::HorizontalInv : BlendDirection::Horizontal;
  }
  if (info.vert != 0) {
    return inverted ? BlendDirection::VerticalInv : BlendDirection::Vertical;
  }
  if (info.rightDiagonal != 0) {
    if (info.longDiagonal != 0) {
      return inverted ? BlendDirection::LongDiagonalRightInv : BlendDirection::LongDiagonalRight;
    }
    return inverted ? BlendDirection::DiagonalRightInv : BlendDirection::DiagonalRight;
  }
  if (info.leftDiagonal != 0) {
    if (info.longDiagonal != 0) {
      return inverted ? BlendDirection::LongDiagonalLeftInv : BlendDirection::LongDiagonalLeft;
    }
    return inverted ? BlendDirection::DiagonalLeftInv : BlendDirection::DiagonalLeft;
  }
  return BlendDirection::Horizontal;
}

bool cellHasBlend(const map::BlendTileInfo &info) {
  return info.horiz != 0 || info.vert != 0 || info.rightDiagonal != 0 || info.leftDiagonal != 0 ||
         info.longDiagonal != 0;
}

} // namespace w3d::terrain
