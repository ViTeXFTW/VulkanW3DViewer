#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "lib/formats/map/types.hpp"
#include "lib/gfx/bounding_box.hpp"

namespace w3d::water {

// A single water surface vertex.
// texCoord stores normalized world-space XZ position for UV scrolling in the shader.
struct WaterVertex {
  glm::vec3 position;
  glm::vec2 texCoord; // world-space XZ / MAP_XY_FACTOR for tiling
};

// A single triangulated water polygon ready for GPU upload.
struct WaterPolygon {
  std::vector<WaterVertex> vertices;
  std::vector<uint32_t> indices;
  gfx::BoundingBox bounds;
  float waterHeight = 0.0f; // world-space Y of the water surface
  std::string name;         // from PolygonTrigger
};

// All water polygons generated from a map's polygon triggers.
struct WaterMeshData {
  std::vector<WaterPolygon> polygons;
  gfx::BoundingBox totalBounds;
};

// Convert a PolygonTrigger point to a world-space 3-D position.
//
// Polygon trigger coordinates are stored as raw integers in "map cell" units:
//   world X = point.x * MAP_XY_FACTOR
//   world Z = point.y * MAP_XY_FACTOR   (map Y → 3-D Z, south direction)
//   world Y = point.z * MAP_HEIGHT_SCALE (map Z → 3-D Y, vertical height)
[[nodiscard]] glm::vec3 triggerPointToWorld(const glm::ivec3 &point);

// Triangulate a simple (possibly concave) polygon using ear-clipping.
// Returns an index list referencing the original vertex array.
// Returns an empty list if the polygon has fewer than 3 vertices.
[[nodiscard]] std::vector<uint32_t> earClipTriangulate(const std::vector<glm::vec2> &poly2d);

// Generate a flat water mesh for a single water PolygonTrigger.
// Returns an empty optional if the trigger is not a valid water area.
[[nodiscard]] std::optional<WaterPolygon>
generateWaterPolygon(const map::PolygonTrigger &trigger);

// Generate water meshes for all water PolygonTriggers in a map file.
[[nodiscard]] WaterMeshData
generateWaterMeshes(const std::vector<map::PolygonTrigger> &triggers);

} // namespace w3d::water
