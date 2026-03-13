#include "lighting_parser.hpp"

namespace map {

static std::optional<Light> parseLight(DataChunkReader &reader, std::string *outError) {
  Light light;

  auto ambientR = reader.readReal(outError);
  if (!ambientR) {
    return std::nullopt;
  }
  auto ambientG = reader.readReal(outError);
  if (!ambientG) {
    return std::nullopt;
  }
  auto ambientB = reader.readReal(outError);
  if (!ambientB) {
    return std::nullopt;
  }
  light.ambient = glm::vec3(*ambientR, *ambientG, *ambientB);

  auto diffuseR = reader.readReal(outError);
  if (!diffuseR) {
    return std::nullopt;
  }
  auto diffuseG = reader.readReal(outError);
  if (!diffuseG) {
    return std::nullopt;
  }
  auto diffuseB = reader.readReal(outError);
  if (!diffuseB) {
    return std::nullopt;
  }
  light.diffuse = glm::vec3(*diffuseR, *diffuseG, *diffuseB);

  auto lightPosX = reader.readReal(outError);
  if (!lightPosX) {
    return std::nullopt;
  }
  auto lightPosY = reader.readReal(outError);
  if (!lightPosY) {
    return std::nullopt;
  }
  auto lightPosZ = reader.readReal(outError);
  if (!lightPosZ) {
    return std::nullopt;
  }
  light.lightPos = glm::vec3(*lightPosX, *lightPosY, *lightPosZ);

  return light;
}

std::optional<GlobalLighting> LightingParser::parse(DataChunkReader &reader, uint16_t version,
                                                    std::string *outError) {
  if (version < K_LIGHTING_VERSION_1 || version > K_LIGHTING_VERSION_3) {
    if (outError) {
      *outError = "Unsupported GlobalLighting version: " + std::to_string(version);
    }
    return std::nullopt;
  }

  GlobalLighting lighting;

  auto timeOfDayInt = reader.readInt(outError);
  if (!timeOfDayInt) {
    return std::nullopt;
  }
  lighting.currentTimeOfDay = static_cast<TimeOfDay>(*timeOfDayInt);

  for (int32_t i = 0; i < NUM_TIME_OF_DAY_SLOTS; ++i) {
    TimeOfDayLighting &slot = lighting.timeOfDaySlots[i];

    auto terrainLight0 = parseLight(reader, outError);
    if (!terrainLight0) {
      return std::nullopt;
    }
    slot.terrainLights[0] = *terrainLight0;

    auto objectLight0 = parseLight(reader, outError);
    if (!objectLight0) {
      return std::nullopt;
    }
    slot.objectLights[0] = *objectLight0;

    if (version >= K_LIGHTING_VERSION_2) {
      for (int32_t j = 1; j <= 2; ++j) {
        auto objectLight = parseLight(reader, outError);
        if (!objectLight) {
          return std::nullopt;
        }
        slot.objectLights[j] = *objectLight;
      }
    }

    if (version >= K_LIGHTING_VERSION_3) {
      for (int32_t j = 1; j <= 2; ++j) {
        auto terrainLight = parseLight(reader, outError);
        if (!terrainLight) {
          return std::nullopt;
        }
        slot.terrainLights[j] = *terrainLight;
      }
    }
  }

  if (reader.remainingInChunk() >= 4) {
    auto shadowColorInt = reader.readInt(outError);
    if (!shadowColorInt) {
      return std::nullopt;
    }
    lighting.shadowColor = static_cast<uint32_t>(*shadowColorInt);
  }

  return lighting;
}

} // namespace map
