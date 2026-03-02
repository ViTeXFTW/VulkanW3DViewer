#pragma once

#include <optional>
#include <string>
#include <vector>

#include "data_chunk_reader.hpp"
#include "types.hpp"

namespace map {

constexpr uint16_t K_OBJECTS_VERSION_1 = 1;
constexpr uint16_t K_OBJECTS_VERSION_2 = 2;
constexpr uint16_t K_OBJECTS_VERSION_3 = 3;

class ObjectsParser {
public:
  static std::optional<std::vector<MapObject>> parse(DataChunkReader &reader, uint16_t version,
                                                     std::string *outError = nullptr);

private:
  static std::optional<MapObject> parseObject(DataChunkReader &reader, uint16_t version,
                                              std::string *outError);
};

} // namespace map
