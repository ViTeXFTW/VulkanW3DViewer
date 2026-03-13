#include "map_info_panel.hpp"

#include "../ui_context.hpp"
#include "lib/formats/map/types.hpp"
#include "render/terrain/terrain_renderable.hpp"
#include "render/water/water_renderable.hpp"

#include <imgui.h>

namespace w3d {

void MapInfoPanel::draw(UIContext &ctx) {
  if (!ctx.hasMap()) {
    ImGui::TextDisabled("No map loaded");
    return;
  }

  const auto &mapFile = *ctx.loadedMap;

  if (!ctx.loadedMapPath.empty()) {
    ImGui::Text("File: %s", ctx.loadedMapPath.c_str());
    ImGui::Separator();
  }

  // Heightmap info
  if (mapFile.hasHeightMap()) {
    ImGui::Text("Heightmap");
    ImGui::Indent();
    ImGui::Text("Dimensions: %d x %d", mapFile.heightMap.width, mapFile.heightMap.height);
    float worldWidth = static_cast<float>(mapFile.heightMap.width) * map::MAP_XY_FACTOR;
    float worldHeight = static_cast<float>(mapFile.heightMap.height) * map::MAP_XY_FACTOR;
    ImGui::Text("World Size: %.0f x %.0f", worldWidth, worldHeight);
    ImGui::Text("Border: %d", mapFile.heightMap.borderSize);
    ImGui::Unindent();
  }

  // Blend tile info
  if (mapFile.hasBlendTiles()) {
    ImGui::Text("Terrain Textures");
    ImGui::Indent();
    ImGui::Text("Texture Classes: %zu", mapFile.blendTiles.textureClasses.size());
    if (ImGui::TreeNode("Texture List")) {
      for (const auto &tc : mapFile.blendTiles.textureClasses) {
        ImGui::BulletText("%s (%d tiles)", tc.name.c_str(), tc.numTiles);
      }
      ImGui::TreePop();
    }
    ImGui::Text("Blend Tiles: %d", mapFile.blendTiles.numBlendedTiles);
    ImGui::Text("Cliff Info: %d", mapFile.blendTiles.numCliffInfo);
    ImGui::Unindent();
  }

  // Objects
  ImGui::Text("Objects: %zu", mapFile.objects.size());
  if (!mapFile.objects.empty()) {
    ImGui::Indent();
    size_t renderableCount = 0;
    size_t roadCount = 0;
    size_t bridgeCount = 0;
    for (const auto &obj : mapFile.objects) {
      if (obj.shouldRender())
        renderableCount++;
      if (obj.isRoadPoint())
        roadCount++;
      if (obj.isBridgePoint())
        bridgeCount++;
    }
    ImGui::Text("Renderable: %zu", renderableCount);
    if (roadCount > 0)
      ImGui::Text("Road Points: %zu", roadCount);
    if (bridgeCount > 0)
      ImGui::Text("Bridge Points: %zu", bridgeCount);
    ImGui::Unindent();
  }

  // Triggers / water areas
  if (!mapFile.triggers.empty()) {
    ImGui::Text("Polygon Triggers: %zu", mapFile.triggers.size());
    ImGui::Indent();
    size_t waterCount = 0;
    size_t riverCount = 0;
    for (const auto &t : mapFile.triggers) {
      if (t.isWaterArea)
        waterCount++;
      if (t.isRiver)
        riverCount++;
    }
    if (waterCount > 0)
      ImGui::Text("Water Areas: %zu", waterCount);
    if (riverCount > 0)
      ImGui::Text("Rivers: %zu", riverCount);
    ImGui::Unindent();
  }

  // Lighting
  if (mapFile.hasLighting()) {
    const char *todNames[] = {"Invalid", "Morning", "Afternoon", "Evening", "Night"};
    int tod = static_cast<int>(mapFile.lighting.currentTimeOfDay);
    if (tod >= 0 && tod <= 4) {
      ImGui::Text("Lighting: %s", todNames[tod]);
    }
  }

  // Sides
  if (!mapFile.sides.sides.empty()) {
    ImGui::Text("Players: %zu", mapFile.sides.sides.size());
    ImGui::Text("Teams: %zu", mapFile.sides.teams.size());
  }

  // Terrain rendering stats
  ImGui::Separator();
  if (ctx.terrainRenderable && ctx.terrainRenderable->hasData()) {
    ImGui::Text("Terrain Chunks: %u / %u visible", ctx.terrainRenderable->visibleChunkCount(),
                ctx.terrainRenderable->totalChunkCount());
    ImGui::Text("Atlas: %s", ctx.terrainRenderable->hasAtlas() ? "Yes" : "No (height gradient)");
  }
  if (ctx.waterRenderable && ctx.waterRenderable->hasData()) {
    ImGui::Text("Water Polygons: %u", ctx.waterRenderable->polygonCount());
  }
}

} // namespace w3d
