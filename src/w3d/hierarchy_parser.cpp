#include "hierarchy_parser.hpp"

namespace w3d {

Hierarchy HierarchyParser::parse(ChunkReader& reader, uint32_t chunkSize) {
  Hierarchy hierarchy;
  size_t endPos = reader.position() + chunkSize;

  while (reader.position() < endPos) {
    auto header = reader.readChunkHeader();
    uint32_t dataSize = header.dataSize();

    switch (header.type) {
      case ChunkType::HIERARCHY_HEADER: {
        hierarchy.version = reader.read<uint32_t>();
        hierarchy.name = reader.readFixedString(W3D_NAME_LEN);
        uint32_t numPivots = reader.read<uint32_t>();
        hierarchy.center = reader.readVector3();
        hierarchy.pivots.reserve(numPivots);
        break;
      }

      case ChunkType::PIVOTS: {
        // Each pivot is: 16 chars name + uint32 parent + 3 float translation
        // + 3 float euler + 4 float quaternion = 60 bytes
        size_t count = dataSize / 60;
        hierarchy.pivots.reserve(count);
        for (size_t i = 0; i < count; ++i) {
          hierarchy.pivots.push_back(parsePivot(reader));
        }
        break;
      }

      case ChunkType::PIVOT_FIXUPS: {
        // Each fixup is 3 floats (12 bytes) per pivot
        size_t count = dataSize / (3 * sizeof(float));
        hierarchy.pivotFixups.reserve(count);
        for (size_t i = 0; i < count; ++i) {
          hierarchy.pivotFixups.push_back(reader.readVector3());
        }
        break;
      }

      default:
        reader.skip(dataSize);
        break;
    }
  }

  return hierarchy;
}

Pivot HierarchyParser::parsePivot(ChunkReader& reader) {
  Pivot pivot;
  pivot.name = reader.readFixedString(W3D_NAME_LEN);
  pivot.parentIndex = reader.read<uint32_t>();
  pivot.translation = reader.readVector3();
  pivot.eulerAngles = reader.readVector3();
  pivot.rotation = reader.readQuaternion();
  return pivot;
}

}  // namespace w3d
