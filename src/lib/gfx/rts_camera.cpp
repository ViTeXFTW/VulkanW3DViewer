#include "lib/gfx/rts_camera.hpp"

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

#include <imgui.h>

namespace w3d::gfx {

RTSCamera::RTSCamera() {
  position_ = glm::vec3(0.0f, height_, 0.0f);
}

void RTSCamera::update(GLFWwindow *window, float deltaTime) {
  if (ImGui::GetIO().WantCaptureKeyboard && ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  handleKeyboardInput(window, deltaTime);
  handleMouseEdgeScroll(window, deltaTime);
  handleRotation(window, deltaTime);
}

void RTSCamera::handleKeyboardInput(GLFWwindow *window, float deltaTime) {
  if (ImGui::GetIO().WantCaptureKeyboard) {
    return;
  }

  float moveAmount = movementSpeed_ * deltaTime;

  glm::vec3 forward(std::sin(yaw_), 0.0f, std::cos(yaw_));
  glm::vec3 right(std::cos(yaw_), 0.0f, -std::sin(yaw_));

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    position_ += forward * moveAmount;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    position_ -= forward * moveAmount;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    position_ -= right * moveAmount;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    position_ += right * moveAmount;
  }
}

void RTSCamera::handleMouseEdgeScroll(GLFWwindow *window, float deltaTime) {
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  int windowWidth, windowHeight;
  glfwGetWindowSize(window, &windowWidth, &windowHeight);

  double mouseX, mouseY;
  glfwGetCursorPos(window, &mouseX, &mouseY);

  float scrollAmount = edgeScrollSpeed_ * deltaTime;
  glm::vec3 forward(std::sin(yaw_), 0.0f, std::cos(yaw_));
  glm::vec3 right(std::cos(yaw_), 0.0f, -std::sin(yaw_));

  if (mouseX < edgeScrollMargin_) {
    position_ -= right * scrollAmount;
  } else if (mouseX > windowWidth - edgeScrollMargin_) {
    position_ += right * scrollAmount;
  }

  if (mouseY < edgeScrollMargin_) {
    position_ += forward * scrollAmount;
  } else if (mouseY > windowHeight - edgeScrollMargin_) {
    position_ -= forward * scrollAmount;
  }
}

void RTSCamera::handleRotation(GLFWwindow *window, float deltaTime) {
  if (ImGui::GetIO().WantCaptureKeyboard) {
    return;
  }

  float rotateAmount = rotationSpeed_ * deltaTime;

  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    yaw_ += rotateAmount;
  }
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
    yaw_ -= rotateAmount;
  }

  while (yaw_ > glm::two_pi<float>()) {
    yaw_ -= glm::two_pi<float>();
  }
  while (yaw_ < 0.0f) {
    yaw_ += glm::two_pi<float>();
  }
}

void RTSCamera::onScroll(float yOffset) {
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  height_ -= yOffset * zoomSpeed_;
  height_ = std::clamp(height_, kMinHeight, kMaxHeight);

  position_.y = height_;
}

glm::mat4 RTSCamera::viewMatrix() const {
  glm::vec3 cameraPos = position();

  glm::vec3 forward(std::sin(yaw_), 0.0f, std::cos(yaw_));
  glm::vec3 lookAt = position_ + forward;

  glm::vec3 up(0.0f, 1.0f, 0.0f);

  return glm::lookAt(cameraPos, lookAt, up);
}

glm::vec3 RTSCamera::position() const {
  float offsetY = height_ * std::tan(pitch_);

  glm::vec3 forward(std::sin(yaw_), 0.0f, std::cos(yaw_));

  glm::vec3 cameraPos = position_;
  cameraPos -= forward * offsetY;
  cameraPos.y = height_;

  return cameraPos;
}

void RTSCamera::setPosition(const glm::vec3 &pos) {
  position_ = pos;
  position_.y = height_;
}

void RTSCamera::setYaw(float yaw) {
  yaw_ = yaw;
}

void RTSCamera::setPitch(float pitch) {
  pitch_ = std::clamp(pitch, kMinPitch, kMaxPitch);
}

void RTSCamera::setHeight(float height) {
  height_ = std::clamp(height, kMinHeight, kMaxHeight);
  position_.y = height_;
}

} // namespace w3d::gfx
