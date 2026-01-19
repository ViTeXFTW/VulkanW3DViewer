#include "animation_parser.hpp"

namespace w3d {

Animation AnimationParser::parse(ChunkReader &reader, uint32_t chunkSize) {
  Animation anim;
  size_t endPos = reader.position() + chunkSize;

  while (reader.position() < endPos) {
    auto header = reader.readChunkHeader();
    uint32_t dataSize = header.dataSize();
    size_t chunkEnd = reader.position() + dataSize;

    switch (header.type) {
    case ChunkType::ANIMATION_HEADER: {
      anim.version = reader.read<uint32_t>();
      anim.name = reader.readFixedString(W3D_NAME_LEN);
      anim.hierarchyName = reader.readFixedString(W3D_NAME_LEN);
      anim.numFrames = reader.read<uint32_t>();
      anim.frameRate = reader.read<uint32_t>();
      break;
    }

    case ChunkType::ANIMATION_CHANNEL:
      anim.channels.push_back(parseAnimChannel(reader, dataSize));
      break;

    case ChunkType::BIT_CHANNEL:
      anim.bitChannels.push_back(parseBitChannel(reader, dataSize));
      break;

    default:
      reader.skip(dataSize);
      break;
    }

    // Ensure we're at the right position for the next chunk
    reader.seek(chunkEnd);
  }

  return anim;
}

CompressedAnimation AnimationParser::parseCompressed(ChunkReader &reader, uint32_t chunkSize) {
  CompressedAnimation anim;
  size_t endPos = reader.position() + chunkSize;

  while (reader.position() < endPos) {
    auto header = reader.readChunkHeader();
    uint32_t dataSize = header.dataSize();
    size_t chunkEnd = reader.position() + dataSize;

    switch (header.type) {
    case ChunkType::COMPRESSED_ANIMATION_HEADER: {
      anim.version = reader.read<uint32_t>();
      anim.name = reader.readFixedString(W3D_NAME_LEN);
      anim.hierarchyName = reader.readFixedString(W3D_NAME_LEN);
      anim.numFrames = reader.read<uint32_t>();
      anim.frameRate = reader.read<uint16_t>();
      anim.flavor = reader.read<uint16_t>();
      break;
    }

    case ChunkType::COMPRESSED_ANIMATION_CHANNEL:
      anim.channels.push_back(parseCompressedChannel(reader, dataSize));
      break;

    case ChunkType::COMPRESSED_BIT_CHANNEL:
      anim.bitChannels.push_back(parseBitChannel(reader, dataSize));
      break;

    default:
      reader.skip(dataSize);
      break;
    }

    // Ensure we're at the right position for the next chunk
    reader.seek(chunkEnd);
  }

  return anim;
}

AnimChannel AnimationParser::parseAnimChannel(ChunkReader &reader, uint32_t dataSize) {
  AnimChannel channel;
  size_t startPos = reader.position();

  channel.firstFrame = reader.read<uint16_t>();
  channel.lastFrame = reader.read<uint16_t>();
  channel.vectorLen = reader.read<uint16_t>();
  channel.flags = reader.read<uint16_t>();
  channel.pivot = reader.read<uint16_t>();
  reader.skip(2); // padding

  // Calculate number of data values
  uint32_t numFrames = channel.lastFrame - channel.firstFrame + 1;
  uint32_t numValues = numFrames * channel.vectorLen;

  // Read the animation data
  channel.data.reserve(numValues);
  for (uint32_t i = 0; i < numValues; ++i) {
    channel.data.push_back(reader.read<float>());
  }

  // Ensure we've read exactly the right amount
  size_t bytesRead = reader.position() - startPos;
  if (bytesRead < dataSize) {
    reader.skip(dataSize - bytesRead);
  }

  return channel;
}

BitChannel AnimationParser::parseBitChannel(ChunkReader &reader, uint32_t dataSize) {
  BitChannel channel;
  size_t startPos = reader.position();

  channel.firstFrame = reader.read<uint16_t>();
  channel.lastFrame = reader.read<uint16_t>();
  channel.flags = reader.read<uint16_t>();
  channel.pivot = reader.read<uint16_t>();
  channel.defaultVal = reader.read<float>();

  // Calculate number of data bytes (1 bit per frame, packed into bytes)
  uint32_t numFrames = channel.lastFrame - channel.firstFrame + 1;
  uint32_t numBytes = (numFrames + 7) / 8;

  // Read the bit data
  channel.data = reader.readArray<uint8_t>(numBytes);

  // Ensure we've read exactly the right amount
  size_t bytesRead = reader.position() - startPos;
  if (bytesRead < dataSize) {
    reader.skip(dataSize - bytesRead);
  }

  return channel;
}

CompressedAnimChannel AnimationParser::parseCompressedChannel(ChunkReader &reader,
                                                              uint32_t dataSize) {
  CompressedAnimChannel channel;
  size_t startPos = reader.position();

  channel.numTimeCodes = reader.read<uint32_t>();
  channel.pivot = reader.read<uint16_t>();
  channel.vectorLen = reader.read<uint8_t>();
  channel.flags = reader.read<uint8_t>();
  reader.skip(4); // padding/reserved

  // Read time codes
  channel.timeCodes.reserve(channel.numTimeCodes);
  for (uint32_t i = 0; i < channel.numTimeCodes; ++i) {
    channel.timeCodes.push_back(reader.read<uint16_t>());
  }

  // Pad to 4-byte boundary if needed
  if (channel.numTimeCodes % 2 != 0) {
    reader.skip(2);
  }

  // Read data values (one vector per time code)
  uint32_t numValues = channel.numTimeCodes * channel.vectorLen;
  channel.data.reserve(numValues);
  for (uint32_t i = 0; i < numValues; ++i) {
    channel.data.push_back(reader.read<float>());
  }

  // Ensure we've read exactly the right amount
  size_t bytesRead = reader.position() - startPos;
  if (bytesRead < dataSize) {
    reader.skip(dataSize - bytesRead);
  }

  return channel;
}

} // namespace w3d
