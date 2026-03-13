#include "lib/formats/ini/water_settings.hpp"

#include <algorithm>
#include <cctype>

namespace ini {

namespace {

int32_t findTimeOfDayIndex(const std::string &name) {
  const auto &names = timeOfDayNames();
  for (size_t i = 0; i < names.size(); ++i) {
    std::string lower1 = names[i];
    std::string lower2 = name;
    std::transform(lower1.begin(), lower1.end(), lower1.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    std::transform(lower2.begin(), lower2.end(), lower2.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (lower1 == lower2) {
      return static_cast<int32_t>(i);
    }
  }
  return -1;
}

} // namespace

void WaterSettings::loadFromINI(std::string_view iniText, std::string *outError) {
  IniParser parser;

  parser.registerBlock("WaterSet", [this](IniParser &p, const std::string &blockName) {
    auto index = findTimeOfDayIndex(blockName);
    if (index < 0 || index >= TIME_OF_DAY_COUNT) {
      return;
    }

    auto &setting = perTimeOfDay[index];

    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"SkyTexture",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->skyTextureFile = fp.parseAsciiString();
         }},
        {"WaterTexture",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->waterTextureFile = fp.parseAsciiString();
         }},
        {"Vertex00Color",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->vertex00Diffuse = fp.parseRGBAColorInt();
         }},
        {"Vertex10Color",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->vertex10Diffuse = fp.parseRGBAColorInt();
         }},
        {"Vertex01Color",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->vertex01Diffuse = fp.parseRGBAColorInt();
         }},
        {"Vertex11Color",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->vertex11Diffuse = fp.parseRGBAColorInt();
         }},
        {"DiffuseColor",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->waterDiffuseColor = fp.parseRGBAColorInt();
         }},
        {"TransparentDiffuseColor",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->transparentWaterDiffuse = fp.parseRGBAColorInt();
         }},
        {"UScrollPerMS",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->uScrollPerMs = fp.parseReal();
         }},
        {"VScrollPerMS",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->vScrollPerMs = fp.parseReal();
         }},
        {"SkyTexelsPerUnit",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->skyTexelsPerUnit = fp.parseReal();
         }},
        {"WaterRepeatCount",
         [](IniParser &fp, void *obj) {
           static_cast<WaterSetting *>(obj)->waterRepeatCount = fp.parseInt();
         }},
    };

    p.parseBlock(fields, &setting);
  });

  parser.registerBlock("WaterTransparency", [this](IniParser &p,
                                                   const std::string & /*blockName*/) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"TransparentWaterDepth",
         [](IniParser &fp, void *obj) {
           static_cast<WaterTransparencySetting *>(obj)->transparentWaterDepth = fp.parseReal();
         }},
        {"TransparentWaterMinOpacity",
         [](IniParser &fp, void *obj) {
           static_cast<WaterTransparencySetting *>(obj)->minWaterOpacity = fp.parseReal();
         }},
        {"StandingWaterColor",
         [](IniParser &fp, void *obj) {
           static_cast<WaterTransparencySetting *>(obj)->standingWaterColor = fp.parseRGBColor();
         }},
        {"StandingWaterTexture",
         [](IniParser &fp, void *obj) {
           static_cast<WaterTransparencySetting *>(obj)->standingWaterTexture =
               fp.parseAsciiString();
         }},
        {"AdditiveBlending",
         [](IniParser &fp, void *obj) {
           static_cast<WaterTransparencySetting *>(obj)->additiveBlend = fp.parseBool();
         }},
        {"RadarWaterColor",
         [](IniParser &fp, void *obj) {
           static_cast<WaterTransparencySetting *>(obj)->radarColor = fp.parseRGBColor();
         }},
        {"SkyboxTextureN",
         [](IniParser &fp, void *obj) {
           static_cast<WaterTransparencySetting *>(obj)->skyboxTextureN = fp.parseAsciiString();
         }},
        {"SkyboxTextureE",
         [](IniParser &fp, void *obj) {
           static_cast<WaterTransparencySetting *>(obj)->skyboxTextureE = fp.parseAsciiString();
         }},
        {"SkyboxTextureS",
         [](IniParser &fp, void *obj) {
           static_cast<WaterTransparencySetting *>(obj)->skyboxTextureS = fp.parseAsciiString();
         }},
        {"SkyboxTextureW",
         [](IniParser &fp, void *obj) {
           static_cast<WaterTransparencySetting *>(obj)->skyboxTextureW = fp.parseAsciiString();
         }},
        {"SkyboxTextureT",
         [](IniParser &fp, void *obj) {
           static_cast<WaterTransparencySetting *>(obj)->skyboxTextureT = fp.parseAsciiString();
         }},
    };

    p.parseBlock(fields, &transparency);
  });

  auto error = parser.parse(iniText);
  if (error.has_value() && outError) {
    *outError = *error;
  }
}

} // namespace ini
