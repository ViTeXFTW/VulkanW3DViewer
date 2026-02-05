#include "lib/gfx/camera.hpp"

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

#include <imgui.h>

namespace w3d::gfx {

void Camera::setTarget(const glm::vec3 &target, float distance) {
  target_ = target;
  distance_ = std::clamp(distance, kMinDistance, kMaxDistance);
}

void Camera::update(GLFWwindow *window) {
  if (ImGui::GetIO().WantCaptureMouse) {
    dragging_ = false;
    return;
  }

  double mouseX, mouseY;
  glfwGetCursorPos(window, &mouseX, &mouseY);

  bool leftButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

  if (leftButton) {
    if (dragging_) {
      double deltaX = mouseX - lastMouseX_;
      double deltaY = mouseY - lastMouseY_;

      yaw_ -= static_cast<float>(deltaX) * kRotationSpeed;
      pitch_ -= static_cast<float>(deltaY) * kRotationSpeed;

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
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }

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

} // namespace w3d::gfx
