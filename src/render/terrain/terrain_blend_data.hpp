#pragma once

#include <cstdint>
#include <vector>

#include "lib/formats/map/types.hpp"

namespace w3d::terrain {

namespace CellBlendFlags {
constexpr uint8_t IsCliff = 0x01;
}

enum class BlendDirectionEncoding : uint8_t {
  None = 0,
  Horizontal = 1,
  HorizontalInv = 2,
  Vertical = 3,
  VerticalInv = 4,
  DiagonalRight = 5,
  DiagonalRightInv = 6,
  DiagonalLeft = 7,
  DiagonalLeftInv = 8,
  LongDiagonal = 9,
  LongDiagonalInv = 10,
  LongDiagonalAlt = 11,
  LongDiagonalAltInv = 12,
  // Phase 5.5: custom blend edge texture -- when this value is set, blendQuadrant holds
  // the GPU texture array layer index of the edge tile whose alpha channel drives blending.
  CustomEdge = 13,
};

struct CellBlendInfo {
  uint16_t baseTileIndex = 0;
  uint16_t baseQuadrant = 0;
  uint16_t blendTileIndex = 0;
  uint16_t blendQuadrant = 0;
  uint16_t extraTileIndex = 0;
  uint16_t extraQuadrant = 0;
  uint8_t blendDirection = 0;
  uint8_t extraDirection = 0;
  uint8_t flags = 0;
  uint8_t padding = 0;
};

static_assert(sizeof(CellBlendInfo) == 16, "CellBlendInfo must be 16 bytes for GPU alignment");

[[nodiscard]] BlendDirectionEncoding encodeBlendDirection(const map::BlendTileInfo &info);

// Build the per-cell GPU blend buffer from parsed BlendTileData.
// edgeTileLayerBase is the GPU texture array layer index at which edge tiles begin
// (i.e., the total number of base tiles). When customBlendEdgeClass entries exist,
// their tile layer index is computed as edgeTileLayerBase + (edge class tile index).
// Pass 0 if no edge tiles are present.
[[nodiscard]] std::vector<CellBlendInfo>
buildCellBlendBuffer(const map::BlendTileData &blendTileData, uint32_t edgeTileLayerBase = 0);

} // namespace w3d::terrain
