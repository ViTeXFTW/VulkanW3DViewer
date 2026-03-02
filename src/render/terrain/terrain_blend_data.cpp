#include "render/terrain/terrain_blend_data.hpp"

#include "render/terrain/terrain_atlas.hpp"

namespace w3d::terrain {

BlendDirectionEncoding encodeBlendDirection(const map::BlendTileInfo &info) {
  bool inverted = (info.inverted & map::INVERTED_MASK) != 0;
  bool flipped = (info.inverted & map::FLIPPED_MASK) != 0;

  if (info.horiz) {
    return inverted ? BlendDirectionEncoding::HorizontalInv : BlendDirectionEncoding::Horizontal;
  }
  if (info.vert) {
    return inverted ? BlendDirectionEncoding::VerticalInv : BlendDirectionEncoding::Vertical;
  }
  if (info.rightDiagonal) {
    return inverted ? BlendDirectionEncoding::DiagonalRightInv
                    : BlendDirectionEncoding::DiagonalRight;
  }
  if (info.leftDiagonal) {
    return inverted ? BlendDirectionEncoding::DiagonalLeftInv
                    : BlendDirectionEncoding::DiagonalLeft;
  }
  if (info.longDiagonal) {
    if (flipped) {
      return inverted ? BlendDirectionEncoding::LongDiagonalAltInv
                      : BlendDirectionEncoding::LongDiagonalAlt;
    }
    return inverted ? BlendDirectionEncoding::LongDiagonalInv
                    : BlendDirectionEncoding::LongDiagonal;
  }

  return BlendDirectionEncoding::None;
}

std::vector<CellBlendInfo> buildCellBlendBuffer(const map::BlendTileData &blendTileData,
                                                uint32_t edgeTileLayerBase) {
  if (blendTileData.dataSize <= 0 ||
      static_cast<int32_t>(blendTileData.tileNdxes.size()) != blendTileData.dataSize) {
    return {};
  }

  std::vector<CellBlendInfo> cells(static_cast<size_t>(blendTileData.dataSize));

  for (int32_t i = 0; i < blendTileData.dataSize; ++i) {
    CellBlendInfo &cell = cells[static_cast<size_t>(i)];

    int16_t baseNdx = blendTileData.tileNdxes[static_cast<size_t>(i)];
    cell.baseTileIndex = static_cast<uint16_t>(decodeTileIndex(baseNdx));
    cell.baseQuadrant = static_cast<uint16_t>(decodeQuadrant(baseNdx));

    int16_t blendRef = blendTileData.blendTileNdxes[static_cast<size_t>(i)];
    if (blendRef > 0 && static_cast<size_t>(blendRef) <= blendTileData.blendTileInfos.size()) {
      const map::BlendTileInfo &binfo =
          blendTileData.blendTileInfos[static_cast<size_t>(blendRef) - 1];

      // Phase 5.5: check for custom blend edge class.
      // When present, blendQuadrant holds the GPU layer index of the edge tile whose
      // alpha channel replaces the procedural gradient.
      if (binfo.customBlendEdgeClass >= 0 && static_cast<size_t>(binfo.customBlendEdgeClass) <
                                                 blendTileData.edgeTextureClasses.size()) {
        const map::TextureClass &edgeClass =
            blendTileData.edgeTextureClasses[static_cast<size_t>(binfo.customBlendEdgeClass)];
        // Store the GPU layer index: edgeTileLayerBase + firstTile of the edge class.
        uint32_t edgeLayer = edgeTileLayerBase + static_cast<uint32_t>(edgeClass.firstTile);
        cell.blendTileIndex =
            static_cast<uint16_t>(decodeTileIndex(static_cast<int16_t>(binfo.blendNdx)));
        cell.blendQuadrant = static_cast<uint16_t>(edgeLayer);
        cell.blendDirection = static_cast<uint8_t>(BlendDirectionEncoding::CustomEdge);
      } else {
        int32_t blendNdx = binfo.blendNdx;
        cell.blendTileIndex =
            static_cast<uint16_t>(decodeTileIndex(static_cast<int16_t>(blendNdx)));
        cell.blendQuadrant = static_cast<uint16_t>(decodeQuadrant(static_cast<int16_t>(blendNdx)));
        cell.blendDirection = static_cast<uint8_t>(encodeBlendDirection(binfo));
      }
    }

    int16_t extraRef = blendTileData.extraBlendTileNdxes[static_cast<size_t>(i)];
    if (extraRef > 0 && static_cast<size_t>(extraRef) <= blendTileData.blendTileInfos.size()) {
      const map::BlendTileInfo &einfo =
          blendTileData.blendTileInfos[static_cast<size_t>(extraRef) - 1];

      if (einfo.customBlendEdgeClass >= 0 && static_cast<size_t>(einfo.customBlendEdgeClass) <
                                                 blendTileData.edgeTextureClasses.size()) {
        const map::TextureClass &edgeClass =
            blendTileData.edgeTextureClasses[static_cast<size_t>(einfo.customBlendEdgeClass)];
        uint32_t edgeLayer = edgeTileLayerBase + static_cast<uint32_t>(edgeClass.firstTile);
        cell.extraTileIndex =
            static_cast<uint16_t>(decodeTileIndex(static_cast<int16_t>(einfo.blendNdx)));
        cell.extraQuadrant = static_cast<uint16_t>(edgeLayer);
        cell.extraDirection = static_cast<uint8_t>(BlendDirectionEncoding::CustomEdge);
      } else {
        int32_t extraNdx = einfo.blendNdx;
        cell.extraTileIndex =
            static_cast<uint16_t>(decodeTileIndex(static_cast<int16_t>(extraNdx)));
        cell.extraQuadrant = static_cast<uint16_t>(decodeQuadrant(static_cast<int16_t>(extraNdx)));
        cell.extraDirection = static_cast<uint8_t>(encodeBlendDirection(einfo));
      }
    }

    int16_t cliffRef = blendTileData.cliffInfoNdxes[static_cast<size_t>(i)];
    if (cliffRef > 0) {
      cell.flags |= CellBlendFlags::IsCliff;
    }
  }

  return cells;
}

} // namespace w3d::terrain
