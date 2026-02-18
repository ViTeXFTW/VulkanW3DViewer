#pragma once

#include "chunk_reader.hpp"
#include "types.hpp"

namespace w3d {

class HierarchyParser {
public:
  // Parse a hierarchy from a chunk reader positioned at W3D_CHUNK_HIERARCHY
  // data
  static Hierarchy parse(ChunkReader &reader, uint32_t chunkSize);

private:
  static Pivot parsePivot(ChunkReader &reader);
};

} // namespace w3d
