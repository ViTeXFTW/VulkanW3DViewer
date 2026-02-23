#include "render/water/water_mesh.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <optional>

namespace w3d::water {

glm::vec3 triggerPointToWorld(const glm::ivec3 &point) {
  return glm::vec3{
      static_cast<float>(point.x) * map::MAP_XY_FACTOR,
      static_cast<float>(point.z) * map::MAP_HEIGHT_SCALE,
      static_cast<float>(point.y) * map::MAP_XY_FACTOR,
  };
}

namespace {

// Signed area of a 2-D triangle (positive = CCW).
float signedTriangleArea(glm::vec2 a, glm::vec2 b, glm::vec2 c) {
  return 0.5f * ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
}

// True if point p lies strictly inside triangle (a, b, c) (CCW winding).
bool pointInTriangle(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c) {
  float d0 = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
  float d1 = (c.x - b.x) * (p.y - b.y) - (c.y - b.y) * (p.x - b.x);
  float d2 = (a.x - c.x) * (p.y - c.y) - (a.y - c.y) * (p.x - c.x);

  bool hasNeg = (d0 < 0.0f) || (d1 < 0.0f) || (d2 < 0.0f);
  bool hasPos = (d0 > 0.0f) || (d1 > 0.0f) || (d2 > 0.0f);
  return !(hasNeg && hasPos);
}

// Return true if vertex at index `i` is an ear of the polygon.
// `indices` is the current list of active vertex indices into `poly2d`.
bool isEar(const std::vector<glm::vec2> &poly2d, const std::vector<size_t> &indices, size_t i) {
  size_t n = indices.size();
  if (n < 3) {
    return false;
  }

  size_t prev = (i + n - 1) % n;
  size_t next = (i + 1) % n;

  glm::vec2 a = poly2d[indices[prev]];
  glm::vec2 b = poly2d[indices[i]];
  glm::vec2 c = poly2d[indices[next]];

  // The ear triangle must be counter-clockwise (convex vertex).
  if (signedTriangleArea(a, b, c) <= 0.0f) {
    return false;
  }

  // No other vertex may lie inside the ear triangle.
  for (size_t j = 0; j < n; ++j) {
    if (j == prev || j == i || j == next) {
      continue;
    }
    if (pointInTriangle(poly2d[indices[j]], a, b, c)) {
      return false;
    }
  }
  return true;
}

} // namespace

std::vector<uint32_t> earClipTriangulate(const std::vector<glm::vec2> &poly2d) {
  size_t n = poly2d.size();
  if (n < 3) {
    return {};
  }

  // Ensure the polygon is CCW; if not, reverse it.
  float area = 0.0f;
  for (size_t i = 0; i < n; ++i) {
    size_t j = (i + 1) % n;
    area += poly2d[i].x * poly2d[j].y;
    area -= poly2d[j].x * poly2d[i].y;
  }

  std::vector<size_t> indices(n);
  for (size_t i = 0; i < n; ++i) {
    indices[i] = i;
  }

  if (area < 0.0f) {
    // CW polygon: reverse to make CCW.
    std::reverse(indices.begin(), indices.end());
  }

  std::vector<uint32_t> result;
  result.reserve((n - 2) * 3);

  size_t remaining = n;
  size_t safetyLimit = n * n + n; // prevent infinite loops on degenerate input
  size_t current = 0;

  while (remaining > 3 && safetyLimit-- > 0) {
    bool earFound = false;
    for (size_t i = 0; i < remaining; ++i) {
      if (isEar(poly2d, indices, i)) {
        size_t prev = (i + remaining - 1) % remaining;
        size_t next = (i + 1) % remaining;

        result.push_back(static_cast<uint32_t>(indices[prev]));
        result.push_back(static_cast<uint32_t>(indices[i]));
        result.push_back(static_cast<uint32_t>(indices[next]));

        indices.erase(indices.begin() + static_cast<std::ptrdiff_t>(i));
        --remaining;
        earFound = true;
        break;
      }
    }

    // If no ear was found (degenerate polygon), bail out.
    if (!earFound) {
      break;
    }
  }

  // Add the remaining triangle.
  if (remaining == 3) {
    result.push_back(static_cast<uint32_t>(indices[0]));
    result.push_back(static_cast<uint32_t>(indices[1]));
    result.push_back(static_cast<uint32_t>(indices[2]));
  }

  return result;
}

std::optional<WaterPolygon> generateWaterPolygon(const map::PolygonTrigger &trigger) {
  if (!trigger.isWaterArea || trigger.points.size() < 3) {
    return std::nullopt;
  }

  WaterPolygon poly;
  poly.name = trigger.name;

  // Convert trigger points to world-space vertices.
  poly.vertices.reserve(trigger.points.size());

  gfx::BoundingBox bounds;

  for (const auto &pt : trigger.points) {
    glm::vec3 world = triggerPointToWorld(pt);

    // Use the average Z of all points as water height (they should be equal, but
    // guard against minor inconsistencies in real map data).
    poly.waterHeight += world.y;

    WaterVertex v;
    v.position = world;
    // UV is world-space XZ normalised by MAP_XY_FACTOR so one texel = one map cell.
    v.texCoord = glm::vec2{world.x / map::MAP_XY_FACTOR, world.z / map::MAP_XY_FACTOR};

    bounds.expand(world);
    poly.vertices.push_back(v);
  }

  poly.waterHeight /= static_cast<float>(poly.vertices.size());

  // Flatten all vertices to the averaged water height so the surface is perfectly flat.
  for (auto &v : poly.vertices) {
    v.position.y = poly.waterHeight;
  }

  // Project vertices into 2-D (XZ plane) for triangulation.
  std::vector<glm::vec2> poly2d;
  poly2d.reserve(poly.vertices.size());
  for (const auto &v : poly.vertices) {
    poly2d.emplace_back(v.position.x, v.position.z);
  }

  poly.indices = earClipTriangulate(poly2d);
  if (poly.indices.empty()) {
    return std::nullopt;
  }

  // Recompute bounding box with the flattened positions.
  bounds = gfx::BoundingBox{};
  for (const auto &v : poly.vertices) {
    bounds.expand(v.position);
  }
  poly.bounds = bounds;

  return poly;
}

WaterMeshData generateWaterMeshes(const std::vector<map::PolygonTrigger> &triggers) {
  WaterMeshData data;

  for (const auto &trigger : triggers) {
    if (!trigger.isWaterArea) {
      continue;
    }

    auto poly = generateWaterPolygon(trigger);
    if (!poly) {
      continue;
    }

    data.totalBounds.expand(poly->bounds);
    data.polygons.push_back(std::move(*poly));
  }

  return data;
}

} // namespace w3d::water
