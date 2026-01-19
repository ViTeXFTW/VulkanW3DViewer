#include "hlod_parser.hpp"

namespace w3d {

HLod HLodParser::parse(ChunkReader& reader, uint32_t chunkSize) {
  HLod hlod;
  size_t endPos = reader.position() + chunkSize;

  while (reader.position() < endPos) {
    auto header = reader.readChunkHeader();
    uint32_t dataSize = header.dataSize();

    switch (header.type) {
      case ChunkType::HLOD_HEADER: {
        hlod.version = reader.read<uint32_t>();
        hlod.lodCount = reader.read<uint32_t>();
        hlod.name = reader.readFixedString(W3D_NAME_LEN);
        hlod.hierarchyName = reader.readFixedString(W3D_NAME_LEN);
        hlod.lodArrays.reserve(hlod.lodCount);
        break;
      }

      case ChunkType::HLOD_LOD_ARRAY:
        hlod.lodArrays.push_back(parseLodArray(reader, dataSize));
        continue;  // Skip seek

      case ChunkType::HLOD_AGGREGATE_ARRAY: {
        // Container for aggregate sub-objects
        auto subReader = reader.subReader(dataSize);
        while (!subReader.atEnd()) {
          auto subHeader = subReader.readChunkHeader();
          if (subHeader.type == ChunkType::HLOD_SUB_OBJECT) {
            hlod.aggregates.push_back(
                parseSubObject(subReader, subHeader.dataSize()));
          } else {
            subReader.skip(subHeader.dataSize());
          }
        }
        continue;
      }

      case ChunkType::HLOD_PROXY_ARRAY: {
        // Container for proxy sub-objects
        auto subReader = reader.subReader(dataSize);
        while (!subReader.atEnd()) {
          auto subHeader = subReader.readChunkHeader();
          if (subHeader.type == ChunkType::HLOD_SUB_OBJECT) {
            hlod.proxies.push_back(
                parseSubObject(subReader, subHeader.dataSize()));
          } else {
            subReader.skip(subHeader.dataSize());
          }
        }
        continue;
      }

      default:
        reader.skip(dataSize);
        break;
    }
  }

  return hlod;
}

HLodArray HLodParser::parseLodArray(ChunkReader& reader, uint32_t chunkSize) {
  HLodArray lodArray;
  size_t endPos = reader.position() + chunkSize;

  while (reader.position() < endPos) {
    auto header = reader.readChunkHeader();
    uint32_t dataSize = header.dataSize();

    switch (header.type) {
      case ChunkType::HLOD_SUB_OBJECT_ARRAY_HEADER: {
        lodArray.modelCount = reader.read<uint32_t>();
        lodArray.maxScreenSize = reader.read<float>();
        lodArray.subObjects.reserve(lodArray.modelCount);
        break;
      }

      case ChunkType::HLOD_SUB_OBJECT:
        lodArray.subObjects.push_back(parseSubObject(reader, dataSize));
        continue;  // Skip seek

      default:
        reader.skip(dataSize);
        break;
    }
  }

  return lodArray;
}

HLodSubObject HLodParser::parseSubObject(ChunkReader& reader,
                                         uint32_t /*dataSize*/) {
  HLodSubObject subObj;
  subObj.boneIndex = reader.read<uint32_t>();
  // Name is double the normal length (32 chars)
  subObj.name = reader.readFixedString(W3D_NAME_LEN * 2);
  return subObj;
}

Box HLodParser::parseBox(ChunkReader& reader, uint32_t /*chunkSize*/) {
  Box box;

  // Box chunk contains:
  // - version (uint32)
  // - attributes (uint32)
  // - name (32 chars - 2x W3D_NAME_LEN)
  // - color (RGB + pad = 4 bytes)
  // - center (3 floats)
  // - extent (3 floats)

  box.version = reader.read<uint32_t>();
  box.attributes = reader.read<uint32_t>();
  box.name = reader.readFixedString(W3D_NAME_LEN * 2);
  box.color = reader.readRGB();
  box.center = reader.readVector3();
  box.extent = reader.readVector3();

  return box;
}

}  // namespace w3d
