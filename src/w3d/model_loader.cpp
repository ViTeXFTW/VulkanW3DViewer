#include "model_loader.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>

namespace w3d {

void ModelLoader::setTexturePath(const std::string &path) {
  customTexturePath_ = path;
}

void ModelLoader::loadTextures(const W3DFile &file, TextureManager &textureManager,
                               LogCallback logCallback) {
  size_t texturesLoaded = 0;
  size_t texturesMissing = 0;
  std::set<std::string> uniqueTextures;

  for (const auto &mesh : file.meshes) {
    for (const auto &tex : mesh.textures) {
      // Skip if we already processed this texture
      if (uniqueTextures.count(tex.name) > 0) {
        continue;
      }
      uniqueTextures.insert(tex.name);

#ifdef W3D_DEBUG
      if (debugMode_) {
        std::cerr << "[DEBUG] Loading texture: " << tex.name << "\n";
      }
#endif

      uint32_t texIdx = textureManager.loadTexture(tex.name);
      if (texIdx > 0) {
        texturesLoaded++;
#ifdef W3D_DEBUG
        if (debugMode_) {
          std::cerr << "[DEBUG]   -> Loaded as index " << texIdx << "\n";
        }
#endif
      } else {
        texturesMissing++;
#ifdef W3D_DEBUG
        if (debugMode_) {
          std::cerr << "[DEBUG]   -> NOT FOUND\n";
        }
#endif
      }
    }
  }

  if (logCallback) {
    logCallback("Textures: " + std::to_string(texturesLoaded) + " loaded, " +
                std::to_string(texturesMissing) + " missing");
  }

#ifdef W3D_DEBUG
  if (debugMode_) {
    std::cerr << "[DEBUG] Total textures in manager: " << textureManager.textureCount() << "\n";
  }
#endif
}

ModelLoadResult ModelLoader::load(const std::filesystem::path &path, VulkanContext &context,
                                  TextureManager &textureManager,
                                  BoneMatrixBuffer &boneMatrixBuffer,
                                  RenderableMesh &renderableMesh, HLodModel &hlodModel,
                                  SkeletonPose &skeletonPose, SkeletonRenderer &skeletonRenderer,
                                  AnimationPlayer &animationPlayer, Camera &camera,
                                  LogCallback logCallback) {
  ModelLoadResult result;

  if (logCallback) {
    logCallback("Loading: " + path.string());
  }

  std::string error;
  auto file = Loader::load(path, &error);

  if (!file) {
    result.error = "Failed to load: " + error;
    return result;
  }

  loadedFile_ = std::move(file);
  loadedFilePath_ = path.string();

  if (logCallback) {
    logCallback("Successfully loaded: " + path.filename().string());

    // Output the description to log
    std::string description = Loader::describe(*loadedFile_);
    std::istringstream stream(description);
    std::string line;
    while (std::getline(stream, line)) {
      logCallback(line);
    }
  }

  // Compute skeleton pose first (needed for mesh positioning)
  context.device().waitIdle();
  if (!loadedFile_->hierarchies.empty()) {
    skeletonPose.computeRestPose(loadedFile_->hierarchies[0]);

    // Initialize skeleton renderer for all frames with rest pose
    for (uint32_t i = 0; i < SkeletonRenderer::FRAME_COUNT; ++i) {
      skeletonRenderer.updateFromPose(context, i, skeletonPose);
    }

    // Initialize bone matrix buffer with rest pose transforms (all frames)
    if (skeletonPose.isValid()) {
      auto skinningMatrices = skeletonPose.getSkinningMatrices();
      for (uint32_t i = 0; i < BoneMatrixBuffer::FRAME_COUNT; ++i) {
        boneMatrixBuffer.update(i, skinningMatrices);
      }
    }

    if (logCallback) {
      logCallback("Loaded skeleton with " + std::to_string(skeletonPose.boneCount()) + " bones");
    }
  }

  // Load animations if present
  animationPlayer.clear();
  if (!loadedFile_->animations.empty() || !loadedFile_->compressedAnimations.empty()) {
    animationPlayer.load(*loadedFile_);
    if (logCallback) {
      logCallback("Loaded " + std::to_string(animationPlayer.animationCount()) + " animation(s)");
    }
  }

  const SkeletonPose *posePtr = skeletonPose.isValid() ? &skeletonPose : nullptr;

  // Load textures referenced by meshes
  loadTextures(*loadedFile_, textureManager, logCallback);

  // Check if file has HLod data - use HLodModel for proper LOD support
  if (!loadedFile_->hlods.empty()) {
    result.useHLodModel = true;
    renderableMesh.destroy(); // Clean up old mesh data

    // Use skinned rendering if we have a hierarchy (for animation support)
    if (!loadedFile_->hierarchies.empty()) {
      result.useSkinnedRendering = true;
      hlodModel.loadSkinned(context, *loadedFile_);
      if (logCallback) {
        logCallback("Using GPU skinned rendering");
      }
    } else {
      result.useSkinnedRendering = false;
      hlodModel.load(context, *loadedFile_, nullptr);
      if (logCallback) {
        logCallback("Using static rendering (no skeleton)");
      }
    }

    const auto &hlod = loadedFile_->hlods[0];
    if (logCallback) {
      logCallback("Loaded HLod: " + hlod.name);
      logCallback("  LOD levels: " + std::to_string(hlodModel.lodCount()));
      logCallback("  Aggregates: " + std::to_string(hlodModel.aggregateCount()));
      logCallback("  Total GPU meshes: " + std::to_string(hlodModel.totalMeshCount()));
      if (result.useSkinnedRendering) {
        logCallback("  Skinned meshes: " + std::to_string(hlodModel.skinnedMeshCount()));
      }

      // Log LOD level details
      for (size_t i = 0; i < hlodModel.lodCount(); ++i) {
        const auto &level = hlodModel.lodLevel(i);
        std::string lodInfo =
            "  LOD " + std::to_string(i) + ": " + std::to_string(level.meshes.size()) +
            " meshes, maxScreenSize=" + std::to_string(static_cast<int>(level.maxScreenSize));
        logCallback(lodInfo);
      }
    }

    if (hlodModel.hasData()) {
      const auto &bounds = hlodModel.bounds();
      camera.setTarget(bounds.center(), bounds.radius() * 2.5f);
    }
  } else {
    // No HLod - use simple mesh rendering
    result.useHLodModel = false;
    hlodModel.destroy(); // Clean up old HLod data

    renderableMesh.loadWithPose(context, *loadedFile_, posePtr);

    if (renderableMesh.hasData()) {
      const auto &bounds = renderableMesh.bounds();
      camera.setTarget(bounds.center(), bounds.radius() * 2.5f);
      if (logCallback) {
        logCallback("Uploaded " + std::to_string(renderableMesh.meshCount()) +
                    " meshes to GPU (no HLod)");
      }
    }
  }

  // Center on skeleton if no mesh data
  bool hasMeshData = (result.useHLodModel && hlodModel.hasData()) ||
                     (!result.useHLodModel && renderableMesh.hasData());
  if (!hasMeshData && skeletonPose.isValid()) {
    glm::vec3 center(0.0f);
    float maxDist = 1.0f;
    for (size_t i = 0; i < skeletonPose.boneCount(); ++i) {
      glm::vec3 pos = skeletonPose.bonePosition(i);
      center += pos;
      maxDist = std::max(maxDist, glm::length(pos));
    }
    center /= static_cast<float>(skeletonPose.boneCount());
    camera.setTarget(center, maxDist * 2.5f);
  }

  result.success = true;
  return result;
}

} // namespace w3d
