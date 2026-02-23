#include "object_placement_utils.hpp"

namespace w3d {

std::string ObjectPlacementUtils::templateNameToW3DName(const std::string &templateName) {
  if (templateName.empty()) {
    return "";
  }
  size_t pos = templateName.rfind('/');
  if (pos == std::string::npos) {
    return templateName;
  }
  if (pos + 1 >= templateName.size()) {
    return "";
  }
  return templateName.substr(pos + 1);
}

glm::vec3 ObjectPlacementUtils::mapPositionToVulkan(const glm::vec3 &mapPosition) {
  return {mapPosition.x, mapPosition.z, mapPosition.y};
}

bool ObjectPlacementUtils::isRoadPoint(uint32_t flags) {
  return (flags & (map::FLAG_ROAD_POINT1 | map::FLAG_ROAD_POINT2)) != 0;
}

bool ObjectPlacementUtils::isBridgePoint(uint32_t flags) {
  return (flags & (map::FLAG_BRIDGE_POINT1 | map::FLAG_BRIDGE_POINT2)) != 0;
}

bool ObjectPlacementUtils::shouldRender(uint32_t flags) {
  if ((flags & map::FLAG_DONT_RENDER) != 0)
    return false;
  if (isRoadPoint(flags))
    return false;
  if (isBridgePoint(flags))
    return false;
  return true;
}

} // namespace w3d
