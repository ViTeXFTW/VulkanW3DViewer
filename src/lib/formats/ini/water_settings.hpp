#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "lib/formats/ini/ini_parser.hpp"

namespace ini {

constexpr int32_t TIME_OF_DAY_COUNT = 5;

enum class TimeOfDay : int32_t { Invalid = 0, Morning = 1, Afternoon = 2, Evening = 3, Night = 4 };

inline const std::vector<std::string> &timeOfDayNames() {
  static const std::vector<std::string> names = {
      "INVALID", "Morning", "Afternoon", "Evening", "Night",
  };
  return names;
}

struct WaterSetting {
  std::string skyTextureFile;
  std::string waterTextureFile;
  int32_t waterRepeatCount = 0;
  float skyTexelsPerUnit = 0.0f;
  RGBAColorInt vertex00Diffuse;
  RGBAColorInt vertex10Diffuse;
  RGBAColorInt vertex11Diffuse;
  RGBAColorInt vertex01Diffuse;
  RGBAColorInt waterDiffuseColor;
  RGBAColorInt transparentWaterDiffuse;
  float uScrollPerMs = 0.0f;
  float vScrollPerMs = 0.0f;
};

struct WaterTransparencySetting {
  float transparentWaterDepth = 3.0f;
  float minWaterOpacity = 1.0f;
  RGBColor standingWaterColor{1.0f, 1.0f, 1.0f};
  RGBColor radarColor{0.55f, 0.55f, 1.0f};
  bool additiveBlend = false;
  std::string standingWaterTexture = "TWWater01.tga";
  std::string skyboxTextureN = "TSMorningN.tga";
  std::string skyboxTextureE = "TSMorningE.tga";
  std::string skyboxTextureS = "TSMorningS.tga";
  std::string skyboxTextureW = "TSMorningW.tga";
  std::string skyboxTextureT = "TSMorningT.tga";
};

struct WaterSettings {
  WaterSetting perTimeOfDay[TIME_OF_DAY_COUNT];
  WaterTransparencySetting transparency;

  void loadFromINI(std::string_view iniText, std::string *outError = nullptr);

  [[nodiscard]] const WaterSetting &getForTimeOfDay(TimeOfDay tod) const {
    auto index = static_cast<int32_t>(tod);
    if (index < 0 || index >= TIME_OF_DAY_COUNT) {
      return perTimeOfDay[0];
    }
    return perTimeOfDay[index];
  }
};

} // namespace ini
