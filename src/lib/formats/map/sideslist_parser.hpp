#pragma once

#include <optional>
#include <string>

#include "data_chunk_reader.hpp"
#include "types.hpp"

namespace map {

constexpr uint16_t K_SIDES_DATA_VERSION_1 = 1;
constexpr uint16_t K_SIDES_DATA_VERSION_2 = 2;
constexpr uint16_t K_SIDES_DATA_VERSION_3 = 3;

class SidesListParser {
public:
  static std::optional<SidesList> parse(DataChunkReader &reader, uint16_t version,
                                        std::string *outError = nullptr);

private:
  static std::optional<BuildListEntry> parseBuildListEntry(DataChunkReader &reader,
                                                           uint16_t version, std::string *outError);
  static std::optional<std::vector<PlayerScript>> parsePlayerScriptsList(DataChunkReader &reader,
                                                                         std::string *outError);
};

} // namespace map
