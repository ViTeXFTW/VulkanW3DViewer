#include "objects_parser.hpp"

namespace map {

std::optional<std::vector<MapObject>>
ObjectsParser::parse(DataChunkReader &reader, uint16_t version, std::string *outError) {
  if (version < K_OBJECTS_VERSION_1 || version > K_OBJECTS_VERSION_3) {
    if (outError) {
      *outError = "Unsupported ObjectsList version: " + std::to_string(version);
    }
    return std::nullopt;
  }

  std::vector<MapObject> objects;

  while (reader.remainingInChunk() > 0 && !reader.atEnd()) {
    auto chunkHeader = reader.openChunk(outError);
    if (!chunkHeader) {
      return std::nullopt;
    }

    auto chunkName = reader.lookupName(chunkHeader->id);
    if (!chunkName) {
      if (outError) {
        *outError = "Unknown chunk ID: " + std::to_string(chunkHeader->id);
      }
      reader.closeChunk();
      return std::nullopt;
    }

    if (*chunkName == "Object") {
      auto object = parseObject(reader, chunkHeader->version, outError);
      if (!object) {
        reader.closeChunk();
        return std::nullopt;
      }
      objects.push_back(*object);
    }

    reader.closeChunk();
  }

  return objects;
}

std::optional<MapObject> ObjectsParser::parseObject(DataChunkReader &reader, uint16_t version,
                                                    std::string *outError) {
  MapObject object;

  auto x = reader.readReal(outError);
  if (!x)
    return std::nullopt;
  auto y = reader.readReal(outError);
  if (!y)
    return std::nullopt;

  if (version >= K_OBJECTS_VERSION_3) {
    auto z = reader.readReal(outError);
    if (!z)
      return std::nullopt;
    object.position = glm::vec3(*x, *y, *z);
  } else {
    object.position = glm::vec3(*x, *y, 0.0f);
  }

  auto angle = reader.readReal(outError);
  if (!angle)
    return std::nullopt;
  object.angle = *angle;

  auto flags = reader.readInt(outError);
  if (!flags)
    return std::nullopt;
  object.flags = static_cast<uint32_t>(*flags);

  auto templateName = reader.readAsciiString(outError);
  if (!templateName)
    return std::nullopt;
  object.templateName = *templateName;

  if (version >= K_OBJECTS_VERSION_2) {
    auto properties = reader.readDict(outError);
    if (!properties)
      return std::nullopt;
    object.properties = *properties;
  }

  return object;
}

} // namespace map
