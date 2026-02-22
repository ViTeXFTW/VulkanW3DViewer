#include "sideslist_parser.hpp"

namespace map {

std::optional<BuildListEntry> SidesListParser::parseBuildListEntry(DataChunkReader &reader,
                                                                   uint16_t version,
                                                                   std::string *outError) {
  BuildListEntry entry;

  auto buildingName = reader.readAsciiString(outError);
  if (!buildingName) {
    return std::nullopt;
  }
  entry.buildingName = std::move(*buildingName);

  auto templateName = reader.readAsciiString(outError);
  if (!templateName) {
    return std::nullopt;
  }
  entry.templateName = std::move(*templateName);

  auto x = reader.readReal(outError);
  auto y = reader.readReal(outError);
  auto z = reader.readReal(outError);
  if (!x || !y || !z) {
    return std::nullopt;
  }
  entry.location = glm::vec3(*x, *y, *z);

  auto angle = reader.readReal(outError);
  if (!angle) {
    return std::nullopt;
  }
  entry.angle = *angle;

  auto initiallyBuilt = reader.readByte(outError);
  if (!initiallyBuilt) {
    return std::nullopt;
  }
  entry.initiallyBuilt = (*initiallyBuilt != 0);

  auto numRebuilds = reader.readInt(outError);
  if (!numRebuilds) {
    return std::nullopt;
  }
  entry.numRebuilds = *numRebuilds;

  if (version >= K_SIDES_DATA_VERSION_3) {
    auto script = reader.readAsciiString(outError);
    if (!script) {
      return std::nullopt;
    }
    entry.script = std::move(*script);

    auto health = reader.readInt(outError);
    if (!health) {
      return std::nullopt;
    }
    entry.health = *health;

    auto isWhiner = reader.readByte(outError);
    if (!isWhiner) {
      return std::nullopt;
    }
    entry.isWhiner = (*isWhiner != 0);

    auto isUnsellable = reader.readByte(outError);
    if (!isUnsellable) {
      return std::nullopt;
    }
    entry.isUnsellable = (*isUnsellable != 0);

    auto isRepairable = reader.readByte(outError);
    if (!isRepairable) {
      return std::nullopt;
    }
    entry.isRepairable = (*isRepairable != 0);
  }

  return entry;
}

std::optional<std::vector<PlayerScript>>
SidesListParser::parsePlayerScriptsList(DataChunkReader &reader, std::string *outError) {
  std::vector<PlayerScript> scripts;

  auto header = reader.openChunk(outError);
  if (!header) {
    return std::nullopt;
  }

  auto chunkName = reader.lookupName(header->id);
  if (!chunkName || *chunkName != "PlayerScriptsList") {
    if (outError) {
      *outError = "Expected PlayerScriptsList chunk, got: " + (chunkName ? *chunkName : "unknown");
    }
    return std::nullopt;
  }

  auto numPlayers = reader.readInt(outError);
  if (!numPlayers) {
    reader.closeChunk();
    return std::nullopt;
  }

  for (int32_t i = 0; i < *numPlayers; ++i) {
    auto numScripts = reader.readInt(outError);
    if (!numScripts) {
      reader.closeChunk();
      return std::nullopt;
    }

    for (int32_t j = 0; j < *numScripts; ++j) {
      PlayerScript script;

      auto name = reader.readAsciiString(outError);
      if (!name) {
        reader.closeChunk();
        return std::nullopt;
      }
      script.name = std::move(*name);

      auto scriptText = reader.readAsciiString(outError);
      if (!scriptText) {
        reader.closeChunk();
        return std::nullopt;
      }
      script.script = std::move(*scriptText);

      scripts.push_back(std::move(script));
    }
  }

  reader.closeChunk();
  return scripts;
}

std::optional<SidesList> SidesListParser::parse(DataChunkReader &reader, uint16_t version,
                                                std::string *outError) {
  if (version < K_SIDES_DATA_VERSION_1 || version > K_SIDES_DATA_VERSION_3) {
    if (outError) {
      *outError = "Unsupported SidesList version: " + std::to_string(version);
    }
    return std::nullopt;
  }

  SidesList sidesList;

  auto numSides = reader.readInt(outError);
  if (!numSides) {
    return std::nullopt;
  }

  for (int32_t i = 0; i < *numSides; ++i) {
    Side side;

    auto dict = reader.readDict(outError);
    if (!dict) {
      return std::nullopt;
    }
    side.properties = std::move(*dict);

    auto nameIt = side.properties.find("playerName");
    if (nameIt != side.properties.end() && nameIt->second.type == DataType::AsciiString) {
      side.name = nameIt->second.stringValue;
    }

    auto buildListCount = reader.readInt(outError);
    if (!buildListCount) {
      return std::nullopt;
    }

    for (int32_t j = 0; j < *buildListCount; ++j) {
      auto entry = parseBuildListEntry(reader, version, outError);
      if (!entry) {
        return std::nullopt;
      }
      side.buildList.push_back(std::move(*entry));
    }

    sidesList.sides.push_back(std::move(side));
  }

  if (version >= K_SIDES_DATA_VERSION_2) {
    auto numTeams = reader.readInt(outError);
    if (!numTeams) {
      return std::nullopt;
    }

    for (int32_t i = 0; i < *numTeams; ++i) {
      Team team;

      auto dict = reader.readDict(outError);
      if (!dict) {
        return std::nullopt;
      }
      team.properties = std::move(*dict);

      auto nameIt = team.properties.find("teamName");
      if (nameIt != team.properties.end() && nameIt->second.type == DataType::AsciiString) {
        team.name = nameIt->second.stringValue;
      }

      sidesList.teams.push_back(std::move(team));
    }
  }

  auto scripts = parsePlayerScriptsList(reader, outError);
  if (!scripts) {
    return std::nullopt;
  }
  sidesList.playerScripts = std::move(*scripts);

  return sidesList;
}

} // namespace map
