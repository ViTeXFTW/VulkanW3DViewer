#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <limits>

namespace w3d {

struct BoundingBox {
  glm::vec3 min{std::numeric_limits<float>::max()};
  glm::vec3 max{std::numeric_limits<float>::lowest()};

  void expand(const glm::vec3& point) {
    min = glm::min(min, point);
    max = glm::max(max, point);
  }

  void expand(const BoundingBox& other) {
    if (other.valid()) {
      min = glm::min(min, other.min);
      max = glm::max(max, other.max);
    }
  }

  glm::vec3 center() const { return (min + max) * 0.5f; }

  glm::vec3 size() const { return max - min; }

  float radius() const { return glm::length(size()) * 0.5f; }

  bool valid() const { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }
};

} // namespace w3d
