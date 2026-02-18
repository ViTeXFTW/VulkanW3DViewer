#pragma once

#include "chunk_reader.hpp"
#include "types.hpp"

namespace w3d {

class AnimationParser {
public:
  // Parse a standard animation from W3D_CHUNK_ANIMATION data
  static Animation parse(ChunkReader &reader, uint32_t chunkSize);

  // Parse a compressed animation from W3D_CHUNK_COMPRESSED_ANIMATION data
  static CompressedAnimation parseCompressed(ChunkReader &reader, uint32_t chunkSize);

private:
  static AnimChannel parseAnimChannel(ChunkReader &reader, uint32_t dataSize);
  static BitChannel parseBitChannel(ChunkReader &reader, uint32_t dataSize);
  static CompressedAnimChannel parseCompressedChannel(ChunkReader &reader, uint32_t dataSize);
};

} // namespace w3d
