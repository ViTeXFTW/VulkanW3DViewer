#pragma once

#include "lib/gfx/vulkan_context.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#include "lib/formats/w3d/hlod_model.hpp"
#include "lib/formats/w3d/loader.hpp"
#include "lib/gfx/camera.hpp"
#include "lib/gfx/texture.hpp"
#include "render/animation_player.hpp"
#include "render/bone_buffer.hpp"
#include "render/renderable_mesh.hpp"
#include "render/skeleton.hpp"
#include "render/skeleton_renderer.hpp"

namespace w3d::big {
class AssetRegistry;
class BigArchiveManager;
} // namespace w3d::big

namespace w3d {

// Using declarations for gfx types
using gfx::Camera;
using gfx::TextureManager;
using gfx::VulkanContext;

/**
 * Result of a model loading operation.
 */
struct ModelLoadResult {
  bool success = false;
  bool useHLodModel = false;
  bool useSkinnedRendering = false;
  std::string error;
};

/**
 * Callback for logging messages during model loading.
 */
using LogCallback = std::function<void(const std::string &)>;

/**
 * Handles loading W3D files and uploading to GPU for rendering.
 */
class ModelLoader {
public:
  ModelLoader() = default;
  ~ModelLoader() = default;

  // Non-copyable
  ModelLoader(const ModelLoader &) = delete;
  ModelLoader &operator=(const ModelLoader &) = delete;

  /**
   * Set custom texture path for texture loading.
   */
  void setTexturePath(const std::string &path);

  /**
   * Enable/disable debug mode for verbose logging.
   */
  void setDebugMode(bool debug) { debugMode_ = debug; }

  /**
   * Set asset registry for path resolution.
   * @param registry Pointer to asset registry (must outlive loader)
   */
  void setAssetRegistry(big::AssetRegistry *registry) { assetRegistry_ = registry; }

  /**
   * Set BIG archive manager for extraction.
   * @param manager Pointer to archive manager (must outlive loader)
   */
  void setBigArchiveManager(big::BigArchiveManager *manager) { bigArchiveManager_ = manager; }

  /**
   * Load a W3D file and upload to GPU.
   *
   * @param path Path to the W3D file
   * @param context Vulkan context for GPU operations
   * @param textureManager Texture manager for loading textures
   * @param boneMatrixBuffer Buffer for bone transformation matrices
   * @param renderableMesh Output for simple mesh rendering
   * @param hlodModel Output for HLod model rendering
   * @param skeletonPose Output skeleton pose
   * @param skeletonRenderer Output skeleton renderer
   * @param animationPlayer Output animation player
   * @param camera Camera to center on loaded model
   * @param logCallback Callback for logging messages
   * @return Result indicating success/failure and rendering mode
   */
  ModelLoadResult load(const std::filesystem::path &path, VulkanContext &context,
                       TextureManager &textureManager, BoneMatrixBuffer &boneMatrixBuffer,
                       RenderableMesh &renderableMesh, HLodModel &hlodModel,
                       SkeletonPose &skeletonPose, SkeletonRenderer &skeletonRenderer,
                       AnimationPlayer &animationPlayer, Camera &camera,
                       LogCallback logCallback = nullptr);

  /**
   * Get the currently loaded file.
   */
  const std::optional<W3DFile> &loadedFile() const { return loadedFile_; }

  /**
   * Get the path of the currently loaded file.
   */
  const std::string &loadedFilePath() const { return loadedFilePath_; }

private:
  void loadTextures(const W3DFile &file, TextureManager &textureManager, LogCallback logCallback);

  std::optional<W3DFile> loadedFile_;
  std::string loadedFilePath_;
  std::string customTexturePath_;
  bool debugMode_ = false;
  big::AssetRegistry *assetRegistry_ = nullptr;
  big::BigArchiveManager *bigArchiveManager_ = nullptr;
};

} // namespace w3d
