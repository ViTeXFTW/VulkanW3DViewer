#include "lib/formats/ini/terrain_types.hpp"

#include <cstddef>

#include "lib/formats/ini/ini_parser.hpp"

namespace ini {

void TerrainTypeCollection::loadFromINI(std::string_view iniText, std::string *outError) {
  IniParser parser;

  parser.registerBlock("Terrain", [this](IniParser &p, const std::string &blockName) {
    auto &terrain = findOrCreate(blockName);

    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Texture",
         [](IniParser &fp, void *obj) {
           static_cast<TerrainType *>(obj)->texture = fp.parseAsciiString();
         }},
        {"BlendEdges",
         [](IniParser &fp, void *obj) {
           static_cast<TerrainType *>(obj)->blendEdgeTexture = fp.parseBool();
         }},
        {"Class",
         [](IniParser &fp, void *obj) {
           auto index = fp.parseIndexList(terrainClassNames());
           static_cast<TerrainType *>(obj)->terrainClass = static_cast<TerrainClass>(index);
         }},
        {"RestrictConstruction",
         [](IniParser &fp, void *obj) {
           static_cast<TerrainType *>(obj)->restrictConstruction = fp.parseBool();
         }},
    };

    p.parseBlock(fields, &terrain);
  });

  auto error = parser.parse(iniText);
  if (error.has_value() && outError) {
    *outError = *error;
  }
}

const TerrainType *TerrainTypeCollection::findByName(const std::string &name) const {
  for (const auto &type : types_) {
    if (type.name == name) {
      return &type;
    }
  }
  return nullptr;
}

TerrainType &TerrainTypeCollection::findOrCreate(const std::string &name) {
  for (auto &type : types_) {
    if (type.name == name) {
      return type;
    }
  }
  types_.push_back(TerrainType{.name = name});
  return types_.back();
}

} // namespace ini
