#pragma once

#include "lib/formats/map/types.hpp"

#include <cstdint>
#include <vector>

namespace w3d::terrain {

/**
 * Generates a CPU-side top-down minimap image from heightmap data.
 *
 * Phase 6.4 – Minimap/preview.
 *
 * The generated image uses RGBA8 pixel layout and is suitable for direct
 * upload to a Vulkan texture (via TextureManager::createTexture) or display
 * as an ImGui image.
 *
 * Colour encoding:
 *   - Without blend data: height-based gradient (dark green → light tan).
 *   - generateScaled() bilinearly downscales the full-resolution result.
 */
class MinimapGenerator {
public:
  struct MinimapImage {
    std::vector<uint8_t> pixels; // RGBA8, row-major (top-left origin)
    uint32_t width = 0;
    uint32_t height = 0;

    bool isValid() const { return !pixels.empty() && width > 0 && height > 0; }
  };

  /**
   * Generate a full-resolution minimap (one pixel per heightmap cell).
   *
   * Returns an invalid image if the heightmap has no data.
   */
  [[nodiscard]] static MinimapImage generate(const map::HeightMap &heightMap);

  /**
   * Generate a scaled-down minimap at the requested output dimensions.
   *
   * If the requested dimensions exceed the source size, the output is clamped
   * to the source dimensions.  Returns an invalid image if the heightmap has
   * no data or either target dimension is zero.
   */
  [[nodiscard]] static MinimapImage generateScaled(const map::HeightMap &heightMap,
                                                   uint32_t targetWidth, uint32_t targetHeight);

private:
  /** Blend between two terrain colours based on normalised height [0, 1]. */
  static void heightToColor(float t, uint8_t &r, uint8_t &g, uint8_t &b);
};

} // namespace w3d::terrain
