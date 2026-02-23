#include "render/terrain/terrain_minimap.hpp"

#include <algorithm>
#include <cmath>

namespace w3d::terrain {

// ── Colour palette ────────────────────────────────────────────────────────────
// Matches the low/high colour used in the terrain fragment shader fallback so
// that the minimap looks consistent with unlit terrain previews.

static constexpr float kLowR = 0.35f, kLowG = 0.55f, kLowB = 0.25f;
static constexpr float kHighR = 0.65f, kHighG = 0.55f, kHighB = 0.40f;

void MinimapGenerator::heightToColor(float t, uint8_t &r, uint8_t &g, uint8_t &b) {
  t = std::clamp(t, 0.0f, 1.0f);
  r = static_cast<uint8_t>(std::lround((kLowR + t * (kHighR - kLowR)) * 255.0f));
  g = static_cast<uint8_t>(std::lround((kLowG + t * (kHighG - kLowG)) * 255.0f));
  b = static_cast<uint8_t>(std::lround((kLowB + t * (kHighB - kLowB)) * 255.0f));
}

// ── Full-resolution generation ────────────────────────────────────────────────

MinimapGenerator::MinimapImage MinimapGenerator::generate(const map::HeightMap &heightMap) {
  if (!heightMap.isValid()) {
    return {};
  }

  const auto w = static_cast<uint32_t>(heightMap.width);
  const auto h = static_cast<uint32_t>(heightMap.height);

  MinimapImage img;
  img.width = w;
  img.height = h;
  img.pixels.resize(static_cast<size_t>(w) * h * 4u);

  for (uint32_t y = 0; y < h; ++y) {
    for (uint32_t x = 0; x < w; ++x) {
      uint8_t rawHeight =
          heightMap.data[static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x)];
      float t = static_cast<float>(rawHeight) / 255.0f;

      size_t idx = (static_cast<size_t>(y) * w + x) * 4u;
      heightToColor(t, img.pixels[idx + 0], img.pixels[idx + 1], img.pixels[idx + 2]);
      img.pixels[idx + 3] = 255u; // fully opaque
    }
  }

  return img;
}

// ── Scaled generation ─────────────────────────────────────────────────────────

MinimapGenerator::MinimapImage MinimapGenerator::generateScaled(const map::HeightMap &heightMap,
                                                                uint32_t targetWidth,
                                                                uint32_t targetHeight) {
  if (!heightMap.isValid() || targetWidth == 0 || targetHeight == 0) {
    return {};
  }

  const auto srcW = static_cast<uint32_t>(heightMap.width);
  const auto srcH = static_cast<uint32_t>(heightMap.height);

  // Clamp to source dimensions
  const uint32_t outW = std::min(targetWidth, srcW);
  const uint32_t outH = std::min(targetHeight, srcH);

  // If no downscaling needed, just generate at full resolution.
  if (outW == srcW && outH == srcH) {
    return generate(heightMap);
  }

  MinimapImage img;
  img.width = outW;
  img.height = outH;
  img.pixels.resize(static_cast<size_t>(outW) * outH * 4u);

  const float scaleX = static_cast<float>(srcW) / static_cast<float>(outW);
  const float scaleY = static_cast<float>(srcH) / static_cast<float>(outH);

  for (uint32_t y = 0; y < outH; ++y) {
    for (uint32_t x = 0; x < outW; ++x) {
      // Map output pixel centre to source coordinates (bilinear sampling)
      float sx = (static_cast<float>(x) + 0.5f) * scaleX - 0.5f;
      float sy = (static_cast<float>(y) + 0.5f) * scaleY - 0.5f;

      int32_t x0 =
          std::clamp(static_cast<int32_t>(std::floor(sx)), 0, static_cast<int32_t>(srcW) - 1);
      int32_t y0 =
          std::clamp(static_cast<int32_t>(std::floor(sy)), 0, static_cast<int32_t>(srcH) - 1);
      int32_t x1 = std::min(x0 + 1, static_cast<int32_t>(srcW) - 1);
      int32_t y1 = std::min(y0 + 1, static_cast<int32_t>(srcH) - 1);

      float fx = sx - std::floor(sx);
      float fy = sy - std::floor(sy);

      auto getH = [&](int32_t gx, int32_t gy) -> float {
        return static_cast<float>(
                   heightMap.data[static_cast<size_t>(gy) * srcW + static_cast<size_t>(gx)]) /
               255.0f;
      };

      float h00 = getH(x0, y0);
      float h10 = getH(x1, y0);
      float h01 = getH(x0, y1);
      float h11 = getH(x1, y1);

      float h = h00 * (1.0f - fx) * (1.0f - fy) + h10 * fx * (1.0f - fy) + h01 * (1.0f - fx) * fy +
                h11 * fx * fy;

      size_t idx = (static_cast<size_t>(y) * outW + x) * 4u;
      heightToColor(h, img.pixels[idx + 0], img.pixels[idx + 1], img.pixels[idx + 2]);
      img.pixels[idx + 3] = 255u;
    }
  }

  return img;
}

} // namespace w3d::terrain
