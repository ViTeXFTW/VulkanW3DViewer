#pragma once

#include <optional>
#include <string>

#include "data_chunk_reader.hpp"
#include "types.hpp"

namespace map {

constexpr uint16_t K_WORLDDICT_VERSION_1 = 1;

class WorldInfoParser {
public:
  static std::optional<WorldInfo> parse(DataChunkReader &reader, uint16_t version,
                                        std::string *outError = nullptr);
};

} // namespace map
