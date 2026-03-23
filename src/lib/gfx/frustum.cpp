#include "lib/gfx/frustum.hpp"

namespace w3d::gfx {

void Frustum::extractFromVP(const glm::mat4 &vp) {
  // Griggs-Hartmann method: extract planes from columns of view-projection matrix
  // Each plane is a row combination of the VP matrix
  auto row = [&](int r) -> glm::vec4 { return glm::vec4(vp[0][r], vp[1][r], vp[2][r], vp[3][r]); };

  glm::vec4 r0 = row(0);
  glm::vec4 r1 = row(1);
  glm::vec4 r2 = row(2);
  glm::vec4 r3 = row(3);

  auto setPlane = [&](int side, const glm::vec4 &coeffs) {
    float len = glm::length(glm::vec3(coeffs));
    if (len > 0.0f) {
      planes_[side].normal = glm::vec3(coeffs) / len;
      planes_[side].distance = coeffs.w / len;
    }
  };

  setPlane(Left, r3 + r0);
  setPlane(Right, r3 - r0);
  setPlane(Bottom, r3 + r1);
  setPlane(Top, r3 - r1);
  setPlane(Near, r3 + r2);
  setPlane(Far, r3 - r2);
}

bool Frustum::isBoxVisible(const BoundingBox &box) const {
  if (!box.valid()) {
    return false;
  }

  for (int i = 0; i < Count; ++i) {
    const auto &p = planes_[i];

    // Find the positive vertex (the corner most aligned with the plane normal)
    glm::vec3 pVertex;
    pVertex.x = (p.normal.x >= 0.0f) ? box.max.x : box.min.x;
    pVertex.y = (p.normal.y >= 0.0f) ? box.max.y : box.min.y;
    pVertex.z = (p.normal.z >= 0.0f) ? box.max.z : box.min.z;

    if (p.distanceToPoint(pVertex) < 0.0f) {
      return false;
    }
  }

  return true;
}

} // namespace w3d::gfx
