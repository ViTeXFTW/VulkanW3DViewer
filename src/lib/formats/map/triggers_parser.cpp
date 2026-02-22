#include "triggers_parser.hpp"

namespace map {

std::optional<std::vector<PolygonTrigger>>
TriggersParser::parse(DataChunkReader &reader, uint16_t version, std::string *outError) {
  if (version < K_TRIGGERS_VERSION_1 || version > K_TRIGGERS_VERSION_4) {
    if (outError) {
      *outError = "Unsupported PolygonTriggers version: " + std::to_string(version);
    }
    return std::nullopt;
  }

  auto countOpt = reader.readInt(outError);
  if (!countOpt) {
    return std::nullopt;
  }
  int32_t count = *countOpt;

  std::vector<PolygonTrigger> triggers;
  triggers.reserve(count);

  for (int32_t i = 0; i < count; ++i) {
    PolygonTrigger trigger;

    auto name = reader.readAsciiString(outError);
    if (!name) {
      return std::nullopt;
    }
    trigger.name = *name;

    auto id = reader.readInt(outError);
    if (!id) {
      return std::nullopt;
    }
    trigger.id = *id;

    if (version >= K_TRIGGERS_VERSION_2) {
      auto isWaterArea = reader.readByte(outError);
      if (!isWaterArea) {
        return std::nullopt;
      }
      trigger.isWaterArea = (*isWaterArea != 0);
    }

    if (version >= K_TRIGGERS_VERSION_3) {
      auto isRiver = reader.readByte(outError);
      if (!isRiver) {
        return std::nullopt;
      }
      trigger.isRiver = (*isRiver != 0);

      auto riverStart = reader.readInt(outError);
      if (!riverStart) {
        return std::nullopt;
      }
      trigger.riverStart = *riverStart;
    }

    auto numPoints = reader.readInt(outError);
    if (!numPoints) {
      return std::nullopt;
    }
    int32_t pointCount = *numPoints;

    trigger.points.reserve(pointCount);
    for (int32_t j = 0; j < pointCount; ++j) {
      auto x = reader.readInt(outError);
      if (!x) {
        return std::nullopt;
      }
      auto y = reader.readInt(outError);
      if (!y) {
        return std::nullopt;
      }
      auto z = reader.readInt(outError);
      if (!z) {
        return std::nullopt;
      }

      trigger.points.emplace_back(*x, *y, *z);
    }

    triggers.push_back(std::move(trigger));
  }

  return triggers;
}

} // namespace map
