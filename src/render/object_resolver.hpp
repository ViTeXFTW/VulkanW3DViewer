#pragma once

#include "lib/gfx/vulkan_context.hpp"

#include <glm/glm.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "lib/formats/map/types.hpp"
#include "lib/formats/w3d/hlod_model.hpp"
#include "lib/gfx/texture.hpp"
#include "render/object_placement_utils.hpp"

namespace w3d::big {
class AssetRegistry;
class BigArchiveManager;
} // namespace w3d::big

namespace w3d {

class ObjectResolver {
public:
  ObjectResolver() = default;
  ~ObjectResolver() = default;

  ObjectResolver(const ObjectResolver &) = delete;
  ObjectResolver &operator=(const ObjectResolver &) = delete;

  void setAssetRegistry(big::AssetRegistry *registry) { assetRegistry_ = registry; }
  void setBigArchiveManager(big::BigArchiveManager *manager) { bigArchiveManager_ = manager; }

  [[nodiscard]] HLodModel *resolve(const std::string &templateName, gfx::VulkanContext &context,
                                   gfx::TextureManager &textureManager);

  void clear();

  [[nodiscard]] size_t cacheSize() const { return modelCache_.size(); }

  [[nodiscard]] static std::string templateNameToW3DName(const std::string &templateName) {
    return ObjectPlacementUtils::templateNameToW3DName(templateName);
  }

  [[nodiscard]] static glm::vec3 mapPositionToVulkan(const glm::vec3 &mapPosition) {
    return ObjectPlacementUtils::mapPositionToVulkan(mapPosition);
  }

  [[nodiscard]] static bool isRoadPoint(uint32_t flags) {
    return ObjectPlacementUtils::isRoadPoint(flags);
  }

  [[nodiscard]] static bool isBridgePoint(uint32_t flags) {
    return ObjectPlacementUtils::isBridgePoint(flags);
  }

  [[nodiscard]] static bool shouldRender(uint32_t flags) {
    return ObjectPlacementUtils::shouldRender(flags);
  }

private:
  [[nodiscard]] std::optional<std::filesystem::path> findW3DPath(const std::string &w3dName) const;

  std::unordered_map<std::string, std::unique_ptr<HLodModel>> modelCache_;
  big::AssetRegistry *assetRegistry_ = nullptr;
  big::BigArchiveManager *bigArchiveManager_ = nullptr;
};

} // namespace w3d
