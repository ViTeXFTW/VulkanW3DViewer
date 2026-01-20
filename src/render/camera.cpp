#include "camera.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace w3d {

void Camera::setTarget(const glm::vec3& target, float distance) {
  target_ = target;
  distance_ = std::clamp(distance, kMinDistance, kMaxDistance);
}

void Camera::update(GLFWwindow* window) {
  // Don't process input if ImGui wants the mouse
  if (ImGui::GetIO().WantCaptureMouse) {
    dragging_ = false;
    return;
  }

  double mouseX, mouseY;
  glfwGetCursorPos(window, &mouseX, &mouseY);

  bool leftButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

  if (leftButton) {
    if (dragging_) {
      // Calculate delta
      double deltaX = mouseX - lastMouseX_;
      double deltaY = mouseY - lastMouseY_;

      // Update yaw and pitch
      yaw_ -= static_cast<float>(deltaX) * kRotationSpeed;
      pitch_ -= static_cast<float>(deltaY) * kRotationSpeed;

      // Clamp pitch to avoid gimbal lock
      pitch_ = std::clamp(pitch_, kMinPitch, kMaxPitch);
    }
    dragging_ = true;
  } else {
    dragging_ = false;
  }

  lastMouseX_ = mouseX;
  lastMouseY_ = mouseY;
}

void Camera::onScroll(float yOffset) {
  // Don't process if ImGui wants the mouse
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  // Logarithmic zoom for better feel at different scales
  float zoomFactor = 1.0f - yOffset * kZoomSpeed;
  distance_ *= zoomFactor;
  distance_ = std::clamp(distance_, kMinDistance, kMaxDistance);
}

glm::mat4 Camera::viewMatrix() const {
  glm::vec3 pos = position();
  glm::vec3 up{0.0f, 1.0f, 0.0f};
  return glm::lookAt(pos, target_, up);
}

glm::vec3 Camera::position() const {
  // Convert spherical coordinates to Cartesian
  // yaw: rotation around Y axis
  // pitch: rotation above/below horizontal plane
  float x = distance_ * std::cos(pitch_) * std::sin(yaw_);
  float y = distance_ * std::sin(pitch_);
  float z = distance_ * std::cos(pitch_) * std::cos(yaw_);

  return target_ + glm::vec3{x, y, z};
}

void Camera::setDistance(float distance) {
  distance_ = std::clamp(distance, kMinDistance, kMaxDistance);
}

void Camera::setYaw(float yaw) {
  yaw_ = yaw;
}

void Camera::setPitch(float pitch) {
  pitch_ = std::clamp(pitch, kMinPitch, kMaxPitch);
}

} // namespace w3d
