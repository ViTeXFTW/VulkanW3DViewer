#include "scene_node.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <limits>

namespace w3d::scene {

void SceneNode::setPosition(const glm::vec3 &position) {
  position_ = position;
}

void SceneNode::setRotationY(float radians) {
  rotationY_ = radians;
}

void SceneNode::setScale(const glm::vec3 &scale) {
  scale_ = scale;
}

glm::mat4 SceneNode::worldTransform() const {
  glm::mat4 t = glm::translate(glm::mat4(1.0f), position_);
  t = glm::rotate(t, rotationY_, glm::vec3(0.0f, 1.0f, 0.0f));
  t = glm::scale(t, scale_);
  return t;
}

void SceneNode::setLocalBounds(const gfx::BoundingBox &bounds) {
  localBounds_ = bounds;
}

gfx::BoundingBox SceneNode::worldBounds() const {
  if (!localBounds_.valid()) {
    return {};
  }

  glm::mat4 transform = worldTransform();

  std::array<glm::vec3, 8> corners = {
      glm::vec3{localBounds_.min.x, localBounds_.min.y, localBounds_.min.z},
      glm::vec3{localBounds_.max.x, localBounds_.min.y, localBounds_.min.z},
      glm::vec3{localBounds_.min.x, localBounds_.max.y, localBounds_.min.z},
      glm::vec3{localBounds_.max.x, localBounds_.max.y, localBounds_.min.z},
      glm::vec3{localBounds_.min.x, localBounds_.min.y, localBounds_.max.z},
      glm::vec3{localBounds_.max.x, localBounds_.min.y, localBounds_.max.z},
      glm::vec3{localBounds_.min.x, localBounds_.max.y, localBounds_.max.z},
      glm::vec3{localBounds_.max.x, localBounds_.max.y, localBounds_.max.z},
  };

  gfx::BoundingBox result;
  for (const auto &corner : corners) {
    glm::vec4 world = transform * glm::vec4(corner, 1.0f);
    result.expand(glm::vec3(world));
  }
  return result;
}

} // namespace w3d::scene
