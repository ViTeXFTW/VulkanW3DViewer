#include "worldinfo_parser.hpp"

namespace map {

std::optional<WorldInfo> WorldInfoParser::parse(DataChunkReader &reader, uint16_t version,
                                                std::string *outError) {
  if (version != K_WORLDDICT_VERSION_1) {
    if (outError) {
      *outError = "Unsupported WorldInfo version: " + std::to_string(version);
    }
    return std::nullopt;
  }

  WorldInfo info;

  auto dict = reader.readDict(outError);
  if (!dict) {
    return std::nullopt;
  }

  info.properties = std::move(*dict);

  auto weatherIt = info.properties.find("weather");
  if (weatherIt != info.properties.end() && weatherIt->second.type == DataType::Int) {
    info.weather = static_cast<Weather>(weatherIt->second.intValue);
  }

  return info;
}

} // namespace map
