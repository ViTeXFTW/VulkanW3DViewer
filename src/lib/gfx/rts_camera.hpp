#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;

namespace w3d::gfx {

class RTSCamera {
public:
  RTSCamera();

  void update(GLFWwindow *window, float deltaTime);

  void onScroll(float yOffset);

  glm::mat4 viewMatrix() const;

  glm::vec3 position() const;

  void setPosition(const glm::vec3 &pos);
  void setYaw(float yaw);
  void setPitch(float pitch);
  void setHeight(float height);

  float yaw() const { return yaw_; }
  float pitch() const { return pitch_; }
  float height() const { return height_; }

  void setMovementSpeed(float speed) { movementSpeed_ = speed; }
  void setRotationSpeed(float speed) { rotationSpeed_ = speed; }
  void setZoomSpeed(float speed) { zoomSpeed_ = speed; }
  void setEdgeScrollMargin(float margin) { edgeScrollMargin_ = margin; }
  void setEdgeScrollSpeed(float speed) { edgeScrollSpeed_ = speed; }

  float movementSpeed() const { return movementSpeed_; }
  float rotationSpeed() const { return rotationSpeed_; }
  float zoomSpeed() const { return zoomSpeed_; }

private:
  void handleKeyboardInput(GLFWwindow *window, float deltaTime);
  void handleMouseEdgeScroll(GLFWwindow *window, float deltaTime);
  void handleRotation(GLFWwindow *window, float deltaTime);

  glm::vec3 position_{0.0f, 0.0f, 0.0f};
  float yaw_ = 0.0f;
  float pitch_ = 1.047f;
  float height_ = 50.0f;

  float movementSpeed_ = 50.0f;
  float rotationSpeed_ = 1.5f;
  float zoomSpeed_ = 10.0f;
  float edgeScrollMargin_ = 10.0f;
  float edgeScrollSpeed_ = 30.0f;

  static constexpr float kMinHeight = 5.0f;
  static constexpr float kMaxHeight = 500.0f;
  static constexpr float kMinPitch = 0.1f;
  static constexpr float kMaxPitch = 1.4f;
  static constexpr float kDefaultPitch = 1.047f;
};

} // namespace w3d::gfx
