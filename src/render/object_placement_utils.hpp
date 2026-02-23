#pragma once

#include <glm/glm.hpp>

#include <string>

#include "lib/formats/map/types.hpp"

namespace w3d {

struct ObjectPlacementUtils {
  [[nodiscard]] static std::string templateNameToW3DName(const std::string &templateName);

  [[nodiscard]] static glm::vec3 mapPositionToVulkan(const glm::vec3 &mapPosition);

  [[nodiscard]] static bool isRoadPoint(uint32_t flags);
  [[nodiscard]] static bool isBridgePoint(uint32_t flags);
  [[nodiscard]] static bool shouldRender(uint32_t flags);
};

} // namespace w3d
