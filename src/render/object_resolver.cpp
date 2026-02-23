#include "object_resolver.hpp"

#include <algorithm>
#include <cctype>

#include "lib/formats/big/asset_registry.hpp"
#include "lib/formats/big/big_archive_manager.hpp"
#include "lib/formats/w3d/loader.hpp"
#include "lib/gfx/texture.hpp"

namespace w3d {

std::optional<std::filesystem::path>
ObjectResolver::findW3DPath(const std::string &w3dName) const {
  if (!assetRegistry_)
    return std::nullopt;

  std::string lower = w3dName;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  std::string archivePath = assetRegistry_->getModelArchivePath(lower);
  if (archivePath.empty()) {
    archivePath = assetRegistry_->getModelArchivePath(w3dName);
  }
  if (archivePath.empty())
    return std::nullopt;

  if (!bigArchiveManager_)
    return std::nullopt;

  return bigArchiveManager_->extractToCache(archivePath);
}

HLodModel *ObjectResolver::resolve(const std::string &templateName, gfx::VulkanContext &context,
                                   gfx::TextureManager &textureManager) {
  auto it = modelCache_.find(templateName);
  if (it != modelCache_.end()) {
    return it->second.get();
  }

  std::string w3dName = templateNameToW3DName(templateName);
  if (w3dName.empty())
    return nullptr;

  auto cachedPath = findW3DPath(w3dName);
  if (!cachedPath)
    return nullptr;

  std::string loadError;
  auto w3dFile = Loader::load(*cachedPath, &loadError);
  if (!w3dFile)
    return nullptr;

  auto model = std::make_unique<HLodModel>();
  model->load(context, *w3dFile, nullptr);

  if (!model->hasData())
    return nullptr;

  HLodModel *ptr = model.get();
  modelCache_[templateName] = std::move(model);
  (void)textureManager;
  return ptr;
}

void ObjectResolver::clear() {
  modelCache_.clear();
}

} // namespace w3d
