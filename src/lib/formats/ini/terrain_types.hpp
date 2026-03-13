#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ini {

enum class TerrainClass : int32_t {
  None = 0,
  Desert1,
  Desert2,
  Desert3,
  EasternEurope1,
  EasternEurope2,
  EasternEurope3,
  Swiss1,
  Swiss2,
  Swiss3,
  Snow1,
  Snow2,
  Snow3,
  Dirt,
  Grass,
  Transition,
  Rock,
  Sand,
  Cliff,
  Wood,
  BlendEdges,
  LiveDesert,
  DryDesert,
  AccentSand,
  TropicalBeach,
  BeachPark,
  RuggedMountain,
  CobblestoneGrass,
  AccentGrass,
  Residential,
  RuggedSnow,
  FlatSnow,
  Field,
  Asphalt,
  Concrete,
  China,
  AccentRock,
  Urban,
  NumClasses
};

inline const std::vector<std::string> &terrainClassNames() {
  static const std::vector<std::string> names = {
      "NONE",
      "DESERT_1",
      "DESERT_2",
      "DESERT_3",
      "EASTERN_EUROPE_1",
      "EASTERN_EUROPE_2",
      "EASTERN_EUROPE_3",
      "SWISS_1",
      "SWISS_2",
      "SWISS_3",
      "SNOW_1",
      "SNOW_2",
      "SNOW_3",
      "DIRT",
      "GRASS",
      "TRANSITION",
      "ROCK",
      "SAND",
      "CLIFF",
      "WOOD",
      "BLEND_EDGE",
      "DESERT_LIVE",
      "DESERT_DRY",
      "SAND_ACCENT",
      "BEACH_TROPICAL",
      "BEACH_PARK",
      "MOUNTAIN_RUGGED",
      "GRASS_COBBLESTONE",
      "GRASS_ACCENT",
      "RESIDENTIAL",
      "SNOW_RUGGED",
      "SNOW_FLAT",
      "FIELD",
      "ASPHALT",
      "CONCRETE",
      "CHINA",
      "ROCK_ACCENT",
      "URBAN",
  };
  return names;
}

struct TerrainType {
  std::string name;
  std::string texture;
  bool blendEdgeTexture = false;
  TerrainClass terrainClass = TerrainClass::None;
  bool restrictConstruction = false;
};

class TerrainTypeCollection {
public:
  TerrainTypeCollection() = default;

  void loadFromINI(std::string_view iniText, std::string *outError = nullptr);

  [[nodiscard]] const TerrainType *findByName(const std::string &name) const;

  [[nodiscard]] const std::vector<TerrainType> &terrainTypes() const { return types_; }

  [[nodiscard]] size_t size() const { return types_.size(); }

  [[nodiscard]] bool empty() const { return types_.empty(); }

  void clear() { types_.clear(); }

private:
  TerrainType &findOrCreate(const std::string &name);

  std::vector<TerrainType> types_;
};

} // namespace ini
