#pragma once

#include <glm/glm.hpp>

#include "lib/gfx/bounding_box.hpp"

namespace w3d::scene {

class SceneNode {
public:
  SceneNode() = default;
  virtual ~SceneNode() = default;

  SceneNode(const SceneNode &) = delete;
  SceneNode &operator=(const SceneNode &) = delete;

  void setPosition(const glm::vec3 &position);
  void setRotationY(float radians);
  void setScale(const glm::vec3 &scale);

  const glm::vec3 &position() const { return position_; }
  float rotationY() const { return rotationY_; }
  const glm::vec3 &scale() const { return scale_; }

  glm::mat4 worldTransform() const;

  void setLocalBounds(const gfx::BoundingBox &bounds);
  const gfx::BoundingBox &localBounds() const { return localBounds_; }

  gfx::BoundingBox worldBounds() const;

  bool isVisible() const { return visible_; }
  void setVisible(bool v) { visible_ = v; }

  virtual const char *typeName() const = 0;

protected:
  glm::vec3 position_{0.0f, 0.0f, 0.0f};
  float rotationY_ = 0.0f;
  glm::vec3 scale_{1.0f, 1.0f, 1.0f};
  gfx::BoundingBox localBounds_;
  bool visible_ = true;
};

} // namespace w3d::scene
