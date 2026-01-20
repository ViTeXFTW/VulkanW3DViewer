#pragma once

#include "chunk_reader.hpp"
#include "types.hpp"

namespace w3d {

class HLodParser {
public:
  // Parse an HLod from W3D_CHUNK_HLOD data
  static HLod parse(ChunkReader &reader, uint32_t chunkSize);

  // Parse a Box from W3D_CHUNK_BOX data
  static Box parseBox(ChunkReader &reader, uint32_t chunkSize);

private:
  static HLodArray parseLodArray(ChunkReader &reader, uint32_t chunkSize);
  static HLodSubObject parseSubObject(ChunkReader &reader, uint32_t dataSize);
};

} // namespace w3d
