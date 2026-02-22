#pragma once

#include <optional>
#include <string>

#include "data_chunk_reader.hpp"
#include "types.hpp"

namespace map {

constexpr uint16_t K_LIGHTING_VERSION_1 = 1;
constexpr uint16_t K_LIGHTING_VERSION_2 = 2;
constexpr uint16_t K_LIGHTING_VERSION_3 = 3;

class LightingParser {
public:
  static std::optional<GlobalLighting> parse(DataChunkReader &reader, uint16_t version,
                                             std::string *outError = nullptr);
};

} // namespace map
