#pragma once

#include <glm/glm.hpp>

#include <array>

#include "lib/gfx/bounding_box.hpp"

namespace w3d::gfx {

struct Plane {
  glm::vec3 normal{0.0f, 0.0f, 0.0f};
  float distance = 0.0f;

  float distanceToPoint(const glm::vec3 &point) const { return glm::dot(normal, point) + distance; }
};

class Frustum {
public:
  enum Side { Left = 0, Right, Bottom, Top, Near, Far, Count };

  Frustum() = default;

  void extractFromVP(const glm::mat4 &viewProj);

  [[nodiscard]] bool isBoxVisible(const BoundingBox &box) const;

  [[nodiscard]] const Plane &plane(int index) const { return planes_[index]; }

private:
  std::array<Plane, Count> planes_;
};

} // namespace w3d::gfx
