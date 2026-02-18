#pragma once

#include "lib/gfx/buffer.hpp"
#include "lib/gfx/pipeline.hpp"
#include "lib/gfx/vulkan_context.hpp"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <vector>

#include "lib/gfx/bounding_box.hpp"
#include "lib/gfx/renderable.hpp"
#include "render/skeleton.hpp"

namespace w3d {

struct W3DFile;

namespace w3d_types {

struct HLodMeshInfo {
  size_t meshIndex;
  uint32_t boneIndex;
  std::string name;
};

struct HLodLevelInfo {
  float maxScreenSize;
  std::vector<HLodMeshInfo> meshes;
  gfx::BoundingBox bounds;
};

struct HLodMeshGPU {
  gfx::VertexBuffer<gfx::Vertex> vertexBuffer;
  gfx::IndexBuffer indexBuffer;
  std::string name;
  std::string textureName;
  int32_t boneIndex = -1;
  size_t lodLevel = 0;
  bool isAggregate = false;

  std::vector<gfx::Vertex> cpuVertices;
  std::vector<uint32_t> cpuIndices;

  std::string baseName;
  size_t subMeshIndex = 0;
  size_t subMeshTotal = 1;
};

struct HLodSkinnedMeshGPU {
  gfx::VertexBuffer<gfx::SkinnedVertex> vertexBuffer;
  gfx::IndexBuffer indexBuffer;
  std::string name;
  std::string textureName;
  int32_t fallbackBoneIndex = -1;
  size_t lodLevel = 0;
  bool isAggregate = false;
  bool hasSkinning = false;

  std::vector<gfx::SkinnedVertex> cpuVertices;
  std::vector<uint32_t> cpuIndices;

  std::string baseName;
  size_t subMeshIndex = 0;
  size_t subMeshTotal = 1;
};

enum class LODSelectionMode { Auto, Manual };

} // namespace w3d_types

class HLodModel : public gfx::IRenderable {
public:
  HLodModel() = default;
  ~HLodModel();

  HLodModel(const HLodModel &) = delete;
  HLodModel &operator=(const HLodModel &) = delete;

  void load(gfx::VulkanContext &context, const W3DFile &file, const SkeletonPose *pose);

  void loadSkinned(gfx::VulkanContext &context, const W3DFile &file);

  void destroy();

  bool hasData() const { return !meshGPU_.empty() || !skinnedMeshGPU_.empty(); }

  bool hasSkinning() const { return !skinnedMeshGPU_.empty(); }

  size_t skinnedMeshCount() const { return skinnedMeshGPU_.size(); }

  const std::string &name() const { return name_; }

  const std::string &hierarchyName() const { return hierarchyName_; }

  size_t lodCount() const { return lodLevels_.size(); }

  const w3d_types::HLodLevelInfo &lodLevel(size_t index) const { return lodLevels_[index]; }

  w3d_types::LODSelectionMode selectionMode() const { return selectionMode_; }
  void setSelectionMode(w3d_types::LODSelectionMode mode) { selectionMode_ = mode; }

  size_t currentLOD() const { return currentLOD_; }
  void setCurrentLOD(size_t level);

  void updateLOD(float screenHeight, float fovY, float cameraDistance);

  float currentScreenSize() const { return currentScreenSize_; }

  size_t aggregateCount() const { return aggregateCount_; }

  size_t totalMeshCount() const { return meshGPU_.size(); }

  void draw(vk::CommandBuffer cmd) override;

  template <typename BindTextureFunc>
  void drawWithTextures(vk::CommandBuffer cmd, BindTextureFunc bindTexture) const;

  template <typename BeforeDrawFunc>
  void drawWithHover(vk::CommandBuffer cmd, int hoverMeshIndex, const glm::vec3 &tintColor,
                     BeforeDrawFunc beforeDraw) const;

  const std::vector<w3d_types::HLodMeshGPU> &meshes() const { return meshGPU_; }

  size_t triangleCount(size_t meshIndex) const;
  bool getTriangle(size_t meshIndex, size_t triangleIndex, glm::vec3 &v0, glm::vec3 &v1,
                   glm::vec3 &v2) const;

  size_t skinnedTriangleCount(size_t meshIndex) const;
  bool getSkinnedTriangle(size_t meshIndex, size_t triangleIndex, glm::vec3 &v0, glm::vec3 &v1,
                          glm::vec3 &v2) const;

  const std::string &meshName(size_t index) const;
  const std::string &skinnedMeshName(size_t index) const;

  // Mesh visibility based on LOD (original behavior)
  bool isMeshVisible(size_t meshIndex) const;
  bool isSkinnedMeshVisible(size_t meshIndex) const;

  // User-controllable mesh hiding API
  bool isMeshHidden(size_t index) const;
  void setMeshHidden(size_t index, bool hidden);
  void setAllMeshesHidden(bool hidden);

  bool isSkinnedMeshHidden(size_t index) const;
  void setSkinnedMeshHidden(size_t index, bool hidden);
  void setAllSkinnedMeshesHidden(bool hidden);

  std::vector<size_t> visibleMeshIndices() const;
  std::vector<size_t> visibleSkinnedMeshIndices() const;

  template <typename UpdateModelMatrixFunc>
  void drawWithBoneTransforms(vk::CommandBuffer cmd, const SkeletonPose *pose,
                              UpdateModelMatrixFunc updateModelMatrix) const;

  template <typename BindTextureFunc>
  void drawSkinnedWithTextures(vk::CommandBuffer cmd, BindTextureFunc bindTexture) const;

  template <typename BeforeDrawFunc>
  void drawSkinnedWithHover(vk::CommandBuffer cmd, int hoverMeshIndex, const glm::vec3 &tintColor,
                            BeforeDrawFunc beforeDraw) const;

  const std::vector<w3d_types::HLodSkinnedMeshGPU> &skinnedMeshes() const {
    return skinnedMeshGPU_;
  }

  const gfx::BoundingBox &bounds() const override { return combinedBounds_; }

  const char *typeName() const override { return "HLodModel"; }

  bool isValid() const override { return hasData(); }

private:
  std::unordered_map<std::string, size_t> buildMeshNameMap(const W3DFile &file);

  std::optional<size_t> findMeshIndex(const std::unordered_map<std::string, size_t> &nameMap,
                                      const W3DFile &file, const std::string &name);

  float calculateScreenSize(float radius, float distance, float screenHeight, float fovY) const;

  template <typename MeshT, typename BeforeDrawFunc>
  void drawMeshesImpl(vk::CommandBuffer cmd, const std::vector<MeshT> &meshes,
                      size_t aggregateCount, BeforeDrawFunc beforeDraw) const;

  std::string name_;
  std::string hierarchyName_;

  std::vector<w3d_types::HLodLevelInfo> lodLevels_;
  std::vector<w3d_types::HLodMeshGPU> meshGPU_;
  std::vector<w3d_types::HLodSkinnedMeshGPU> skinnedMeshGPU_;
  size_t aggregateCount_ = 0;
  size_t skinnedAggregateCount_ = 0;

  // User-controllable mesh visibility state
  std::vector<bool> meshVisibility_;
  std::vector<bool> skinnedMeshVisibility_;

  w3d_types::LODSelectionMode selectionMode_ = w3d_types::LODSelectionMode::Auto;
  size_t currentLOD_ = 0;
  float currentScreenSize_ = 0.0f;

  gfx::BoundingBox combinedBounds_;
};

template <typename MeshT, typename BeforeDrawFunc>
void HLodModel::drawMeshesImpl(vk::CommandBuffer cmd, const std::vector<MeshT> &meshes,
                               size_t aggregateCount, BeforeDrawFunc beforeDraw) const {
  for (size_t i = 0; i < aggregateCount; ++i) {
    const auto &mesh = meshes[i];
    beforeDraw(mesh);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }

  for (size_t i = aggregateCount; i < meshes.size(); ++i) {
    const auto &mesh = meshes[i];

    if (mesh.lodLevel != currentLOD_) {
      continue;
    }

    beforeDraw(mesh);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }
}

template <typename BindTextureFunc>
void HLodModel::drawWithTextures(vk::CommandBuffer cmd, BindTextureFunc bindTexture) const {
  drawMeshesImpl(cmd, meshGPU_, aggregateCount_,
                 [&](const w3d_types::HLodMeshGPU &mesh) { bindTexture(mesh.textureName); });
}

template <typename BeforeDrawFunc>
void HLodModel::drawWithHover(vk::CommandBuffer cmd, int hoverMeshIndex, const glm::vec3 &tintColor,
                              BeforeDrawFunc beforeDraw) const {
  for (size_t i = 0; i < aggregateCount_; ++i) {
    // Skip if user has hidden this mesh
    if (i < meshVisibility_.size() && !meshVisibility_[i]) {
      continue;
    }

    const auto &mesh = meshGPU_[i];
    glm::vec3 tint = (static_cast<int>(i) == hoverMeshIndex) ? tintColor : glm::vec3(1.0f);
    beforeDraw(i, mesh.textureName, tint);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }

  for (size_t i = aggregateCount_; i < meshGPU_.size(); ++i) {
    // Skip if user has hidden this mesh
    if (i < meshVisibility_.size() && !meshVisibility_[i]) {
      continue;
    }

    const auto &mesh = meshGPU_[i];

    if (mesh.lodLevel != currentLOD_) {
      continue;
    }

    glm::vec3 tint = (static_cast<int>(i) == hoverMeshIndex) ? tintColor : glm::vec3(1.0f);
    beforeDraw(i, mesh.textureName, tint);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }
}

template <typename UpdateModelMatrixFunc>
void HLodModel::drawWithBoneTransforms(vk::CommandBuffer cmd, const SkeletonPose *pose,
                                       UpdateModelMatrixFunc updateModelMatrix) const {
  drawMeshesImpl(cmd, meshGPU_, aggregateCount_, [&](const w3d_types::HLodMeshGPU &mesh) {
    glm::mat4 boneTransform(1.0f);
    if (pose && mesh.boneIndex >= 0 && static_cast<size_t>(mesh.boneIndex) < pose->boneCount()) {
      boneTransform = pose->boneTransform(static_cast<size_t>(mesh.boneIndex));
    }
    updateModelMatrix(boneTransform);
  });
}

template <typename BindTextureFunc>
void HLodModel::drawSkinnedWithTextures(vk::CommandBuffer cmd, BindTextureFunc bindTexture) const {
  drawMeshesImpl(cmd, skinnedMeshGPU_, skinnedAggregateCount_,
                 [&](const w3d_types::HLodSkinnedMeshGPU &mesh) { bindTexture(mesh.textureName); });
}

template <typename BeforeDrawFunc>
void HLodModel::drawSkinnedWithHover(vk::CommandBuffer cmd, int hoverMeshIndex,
                                     const glm::vec3 &tintColor, BeforeDrawFunc beforeDraw) const {
  for (size_t i = 0; i < skinnedAggregateCount_; ++i) {
    // Skip if user has hidden this mesh
    if (i < skinnedMeshVisibility_.size() && !skinnedMeshVisibility_[i]) {
      continue;
    }

    const auto &mesh = skinnedMeshGPU_[i];
    glm::vec3 tint = (static_cast<int>(i) == hoverMeshIndex) ? tintColor : glm::vec3(1.0f);
    beforeDraw(i, mesh.textureName, tint);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }

  for (size_t i = skinnedAggregateCount_; i < skinnedMeshGPU_.size(); ++i) {
    // Skip if user has hidden this mesh
    if (i < skinnedMeshVisibility_.size() && !skinnedMeshVisibility_[i]) {
      continue;
    }

    const auto &mesh = skinnedMeshGPU_[i];

    if (mesh.lodLevel != currentLOD_) {
      continue;
    }

    glm::vec3 tint = (static_cast<int>(i) == hoverMeshIndex) ? tintColor : glm::vec3(1.0f);
    beforeDraw(i, mesh.textureName, tint);

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }
}

} // namespace w3d
