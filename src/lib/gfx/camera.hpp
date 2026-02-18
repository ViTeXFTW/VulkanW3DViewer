#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;

namespace w3d::gfx {

class Camera {
public:
  Camera() = default;

  void setTarget(const glm::vec3 &target, float distance);

  void update(GLFWwindow *window);

  void onScroll(float yOffset);

  glm::mat4 viewMatrix() const;

  glm::vec3 position() const;

  void setDistance(float distance);
  void setYaw(float yaw);
  void setPitch(float pitch);

  float distance() const { return distance_; }
  float yaw() const { return yaw_; }
  float pitch() const { return pitch_; }
  const glm::vec3 &target() const { return target_; }

private:
  glm::vec3 target_{0.0f, 0.0f, 0.0f};
  float distance_ = 5.0f;
  float yaw_ = 0.0f;
  float pitch_ = 0.3f;

  bool dragging_ = false;
  double lastMouseX_ = 0.0;
  double lastMouseY_ = 0.0;

  static constexpr float kRotationSpeed = 0.005f;
  static constexpr float kZoomSpeed = 0.15f;
  static constexpr float kMinDistance = 0.1f;
  static constexpr float kMaxDistance = 10000.0f;
  static constexpr float kMinPitch = -1.5f;
  static constexpr float kMaxPitch = 1.5f;
};

} // namespace w3d::gfx
