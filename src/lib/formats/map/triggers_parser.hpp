#pragma once

#include <optional>
#include <string>
#include <vector>

#include "data_chunk_reader.hpp"
#include "types.hpp"

namespace map {

constexpr uint16_t K_TRIGGERS_VERSION_1 = 1;
constexpr uint16_t K_TRIGGERS_VERSION_2 = 2;
constexpr uint16_t K_TRIGGERS_VERSION_3 = 3;
constexpr uint16_t K_TRIGGERS_VERSION_4 = 4;

class TriggersParser {
public:
  static std::optional<std::vector<PolygonTrigger>> parse(DataChunkReader &reader, uint16_t version,
                                                          std::string *outError = nullptr);
};

} // namespace map
