#include "lib/formats/w3d/hlod_model.hpp"

#include "lib/gfx/vulkan_context.hpp"
#include "lib/gfx/bounding_box.hpp"

#include <algorithm>
#include <cmath>

#include "render/mesh_converter.hpp"

namespace w3d {

HLodModel::~HLodModel() {
  destroy();
}

void HLodModel::destroy() {
  for (auto &mesh : meshGPU_) {
    mesh.vertexBuffer.destroy();
    mesh.indexBuffer.destroy();
  }
  meshGPU_.clear();
  meshVisibility_.clear();

  for (auto &mesh : skinnedMeshGPU_) {
    mesh.vertexBuffer.destroy();
    mesh.indexBuffer.destroy();
  }
  skinnedMeshGPU_.clear();
  skinnedMeshVisibility_.clear();

  lodLevels_.clear();
  aggregateCount_ = 0;
  skinnedAggregateCount_ = 0;
  currentLOD_ = 0;
  combinedBounds_ = gfx::BoundingBox{};
  name_.clear();
  hierarchyName_.clear();
}

std::unordered_map<std::string, size_t> HLodModel::buildMeshNameMap(const W3DFile &file) {
  std::unordered_map<std::string, size_t> nameMap;

  for (size_t i = 0; i < file.meshes.size(); ++i) {
    const auto &mesh = file.meshes[i];

    std::string fullName = mesh.header.containerName + "." + mesh.header.meshName;
    nameMap[fullName] = i;
    nameMap[mesh.header.meshName] = i;
  }

  return nameMap;
}

std::optional<size_t>
HLodModel::findMeshIndex(const std::unordered_map<std::string, size_t> &nameMap,
                         const W3DFile & /*file*/, const std::string &name) {
  auto it = nameMap.find(name);
  if (it != nameMap.end()) {
    return it->second;
  }

  size_t dotPos = name.find('.');
  if (dotPos != std::string::npos) {
    std::string shortName = name.substr(dotPos + 1);
    it = nameMap.find(shortName);
    if (it != nameMap.end()) {
      return it->second;
    }
  }

  return std::nullopt;
}

void HLodModel::load(gfx::VulkanContext &context, const W3DFile &file, const SkeletonPose *pose) {
  destroy();

  if (file.hlods.empty()) {
    auto meshNameMap = buildMeshNameMap(file);

    w3d_types::HLodLevelInfo level0;
    level0.maxScreenSize = 0.0f;

    for (size_t i = 0; i < file.meshes.size(); ++i) {
      w3d_types::HLodMeshInfo info;
      info.meshIndex = i;
      info.boneIndex = 0;
      info.name = file.meshes[i].header.meshName;
      level0.meshes.push_back(info);
    }

    lodLevels_.push_back(level0);

    for (size_t i = 0; i < file.meshes.size(); ++i) {
      auto converted = MeshConverter::convert(file.meshes[i]);
      if (converted.subMeshes.empty()) {
        continue;
      }

      for (size_t subIdx = 0; subIdx < converted.subMeshes.size(); ++subIdx) {
        const auto &subMesh = converted.subMeshes[subIdx];
        if (subMesh.vertices.empty() || subMesh.indices.empty()) {
          continue;
        }

        w3d_types::HLodMeshGPU gpuMesh;
        gpuMesh.baseName = converted.name;
        gpuMesh.name = converted.name;
        if (converted.subMeshes.size() > 1) {
          gpuMesh.name += "_sub" + std::to_string(subIdx);
        }
        gpuMesh.subMeshIndex = subIdx;
        gpuMesh.subMeshTotal = converted.subMeshes.size();
        gpuMesh.textureName = subMesh.textureName;
        gpuMesh.boneIndex = -1;
        gpuMesh.lodLevel = 0;
        gpuMesh.isAggregate = false;

        gpuMesh.cpuVertices = subMesh.vertices;
        gpuMesh.cpuIndices = subMesh.indices;

        gpuMesh.vertexBuffer.create(context, subMesh.vertices);
        gpuMesh.indexBuffer.create(context, subMesh.indices);

        combinedBounds_.expand(gfx::BoundingBox{subMesh.bounds.min, subMesh.bounds.max});
        lodLevels_[0].bounds.expand(gfx::BoundingBox{subMesh.bounds.min, subMesh.bounds.max});

        meshGPU_.push_back(std::move(gpuMesh));
      }
    }

    return;
  }

  const auto &hlod = file.hlods[0];
  name_ = hlod.name;
  hierarchyName_ = hlod.hierarchyName;

  auto meshNameMap = buildMeshNameMap(file);

  lodLevels_.resize(hlod.lodArrays.size());

  for (size_t lodIdx = 0; lodIdx < hlod.lodArrays.size(); ++lodIdx) {
    const auto &lodArray = hlod.lodArrays[lodIdx];
    auto &levelInfo = lodLevels_[lodIdx];

    levelInfo.maxScreenSize = lodArray.maxScreenSize;
    levelInfo.meshes.reserve(lodArray.subObjects.size());

    for (const auto &subObj : lodArray.subObjects) {
      auto meshIdx = findMeshIndex(meshNameMap, file, subObj.name);
      if (meshIdx.has_value()) {
        w3d_types::HLodMeshInfo info;
        info.meshIndex = meshIdx.value();
        info.boneIndex = subObj.boneIndex;
        info.name = subObj.name;
        levelInfo.meshes.push_back(info);
      }
    }
  }

  std::unordered_map<size_t, size_t> uploadedMeshes;

  for (const auto &subObj : hlod.aggregates) {
    auto meshIdx = findMeshIndex(meshNameMap, file, subObj.name);
    if (!meshIdx.has_value()) {
      continue;
    }

    auto converted = MeshConverter::convert(file.meshes[meshIdx.value()]);
    if (converted.subMeshes.empty()) {
      continue;
    }

    if (pose && subObj.boneIndex < pose->boneCount()) {
      glm::mat4 boneTransform = pose->boneTransform(subObj.boneIndex);
      MeshConverter::applyBoneTransform(converted, boneTransform);
    }

    for (size_t subIdx = 0; subIdx < converted.subMeshes.size(); ++subIdx) {
      const auto &subMesh = converted.subMeshes[subIdx];
      if (subMesh.vertices.empty() || subMesh.indices.empty()) {
        continue;
      }

      w3d_types::HLodMeshGPU gpuMesh;
      gpuMesh.baseName = subObj.name;
      gpuMesh.name = subObj.name;
      if (converted.subMeshes.size() > 1) {
        gpuMesh.name += "_sub" + std::to_string(subIdx);
      }
      gpuMesh.subMeshIndex = subIdx;
      gpuMesh.subMeshTotal = converted.subMeshes.size();
      gpuMesh.textureName = subMesh.textureName;
      gpuMesh.boneIndex = static_cast<int32_t>(subObj.boneIndex);
      gpuMesh.lodLevel = 0;
      gpuMesh.isAggregate = true;

      gpuMesh.cpuVertices = subMesh.vertices;
      gpuMesh.cpuIndices = subMesh.indices;

      gpuMesh.vertexBuffer.create(context, subMesh.vertices);
      gpuMesh.indexBuffer.create(context, subMesh.indices);

      combinedBounds_.expand(gfx::BoundingBox{subMesh.bounds.min, subMesh.bounds.max});

      meshGPU_.push_back(std::move(gpuMesh));
    }

    uploadedMeshes[meshIdx.value()] = meshGPU_.size();
  }

  aggregateCount_ = meshGPU_.size();

  for (size_t lodIdx = 0; lodIdx < lodLevels_.size(); ++lodIdx) {
    auto &levelInfo = lodLevels_[lodIdx];

    for (const auto &meshInfo : levelInfo.meshes) {
      auto it = uploadedMeshes.find(meshInfo.meshIndex);
      if (it != uploadedMeshes.end()) {}

      auto converted = MeshConverter::convert(file.meshes[meshInfo.meshIndex]);
      if (converted.subMeshes.empty()) {
        continue;
      }

      if (pose && meshInfo.boneIndex < pose->boneCount()) {
        glm::mat4 boneTransform = pose->boneTransform(meshInfo.boneIndex);
        MeshConverter::applyBoneTransform(converted, boneTransform);
      }

      for (size_t subIdx = 0; subIdx < converted.subMeshes.size(); ++subIdx) {
        const auto &subMesh = converted.subMeshes[subIdx];
        if (subMesh.vertices.empty() || subMesh.indices.empty()) {
          continue;
        }

        w3d_types::HLodMeshGPU gpuMesh;
        gpuMesh.baseName = meshInfo.name;
        gpuMesh.name = meshInfo.name;
        if (converted.subMeshes.size() > 1) {
          gpuMesh.name += "_sub" + std::to_string(subIdx);
        }
        gpuMesh.subMeshIndex = subIdx;
        gpuMesh.subMeshTotal = converted.subMeshes.size();
        gpuMesh.textureName = subMesh.textureName;
        gpuMesh.boneIndex = static_cast<int32_t>(meshInfo.boneIndex);
        gpuMesh.lodLevel = lodIdx;
        gpuMesh.isAggregate = false;

        gpuMesh.cpuVertices = subMesh.vertices;
        gpuMesh.cpuIndices = subMesh.indices;

        gpuMesh.vertexBuffer.create(context, subMesh.vertices);
        gpuMesh.indexBuffer.create(context, subMesh.indices);

        combinedBounds_.expand(gfx::BoundingBox{subMesh.bounds.min, subMesh.bounds.max});
        lodLevels_[0].bounds.expand(gfx::BoundingBox{subMesh.bounds.min, subMesh.bounds.max});

        meshGPU_.push_back(std::move(gpuMesh));
      }
    }
  }

  currentLOD_ = 0;

  // Initialize all meshes as visible
  meshVisibility_.resize(meshGPU_.size(), true);
}

void HLodModel::loadSkinned(gfx::VulkanContext &context, const W3DFile &file) {
  destroy();

  if (file.hlods.empty()) {
    w3d_types::HLodLevelInfo level0;
    level0.maxScreenSize = 0.0f;

    for (size_t i = 0; i < file.meshes.size(); ++i) {
      w3d_types::HLodMeshInfo info;
      info.meshIndex = i;
      info.boneIndex = 0;
      info.name = file.meshes[i].header.meshName;
      level0.meshes.push_back(info);
    }

    lodLevels_.push_back(level0);

    auto skinnedMeshes = MeshConverter::convertAllSkinned(file);
    for (auto &converted : skinnedMeshes) {
      for (size_t subIdx = 0; subIdx < converted.subMeshes.size(); ++subIdx) {
        auto &subMesh = converted.subMeshes[subIdx];
        if (subMesh.vertices.empty() || subMesh.indices.empty()) {
          continue;
        }

        w3d_types::HLodSkinnedMeshGPU gpuMesh;
        gpuMesh.baseName = converted.name;
        gpuMesh.name = converted.name;
        if (converted.subMeshes.size() > 1) {
          gpuMesh.name += "_sub" + std::to_string(subIdx);
        }
        gpuMesh.subMeshIndex = subIdx;
        gpuMesh.subMeshTotal = converted.subMeshes.size();
        gpuMesh.textureName = subMesh.textureName;
        gpuMesh.fallbackBoneIndex = converted.fallbackBoneIndex;
        gpuMesh.lodLevel = 0;
        gpuMesh.isAggregate = false;
        gpuMesh.hasSkinning = converted.hasSkinning;

        gpuMesh.cpuVertices = subMesh.vertices;
        gpuMesh.cpuIndices = subMesh.indices;

        gpuMesh.vertexBuffer.create(context, subMesh.vertices);
        gpuMesh.indexBuffer.create(context, subMesh.indices);

        combinedBounds_.expand(gfx::BoundingBox{subMesh.bounds.min, subMesh.bounds.max});
        lodLevels_[0].bounds.expand(gfx::BoundingBox{subMesh.bounds.min, subMesh.bounds.max});

        skinnedMeshGPU_.push_back(std::move(gpuMesh));
      }
    }

    // Initialize all skinned meshes as visible
    skinnedMeshVisibility_.resize(skinnedMeshGPU_.size(), true);

    return;
  }

  const auto &hlod = file.hlods[0];
  name_ = hlod.name;
  hierarchyName_ = hlod.hierarchyName;

  auto meshNameMap = buildMeshNameMap(file);

  lodLevels_.resize(hlod.lodArrays.size());

  for (size_t lodIdx = 0; lodIdx < hlod.lodArrays.size(); ++lodIdx) {
    const auto &lodArray = hlod.lodArrays[lodIdx];
    auto &levelInfo = lodLevels_[lodIdx];

    levelInfo.maxScreenSize = lodArray.maxScreenSize;
    levelInfo.meshes.reserve(lodArray.subObjects.size());

    for (const auto &subObj : lodArray.subObjects) {
      auto meshIdx = findMeshIndex(meshNameMap, file, subObj.name);
      if (meshIdx.has_value()) {
        w3d_types::HLodMeshInfo info;
        info.meshIndex = meshIdx.value();
        info.boneIndex = subObj.boneIndex;
        info.name = subObj.name;
        levelInfo.meshes.push_back(info);
      }
    }
  }

  for (const auto &subObj : hlod.aggregates) {
    auto meshIdx = findMeshIndex(meshNameMap, file, subObj.name);
    if (!meshIdx.has_value()) {
      continue;
    }

    auto converted = MeshConverter::convertSkinned(file.meshes[meshIdx.value()],
                                                   static_cast<int32_t>(subObj.boneIndex));
    if (converted.subMeshes.empty()) {
      continue;
    }

    for (size_t subIdx = 0; subIdx < converted.subMeshes.size(); ++subIdx) {
      auto &subMesh = converted.subMeshes[subIdx];
      if (subMesh.vertices.empty() || subMesh.indices.empty()) {
        continue;
      }

      w3d_types::HLodSkinnedMeshGPU gpuMesh;
      gpuMesh.baseName = subObj.name;
      gpuMesh.name = subObj.name;
      if (converted.subMeshes.size() > 1) {
        gpuMesh.name += "_sub" + std::to_string(subIdx);
      }
      gpuMesh.subMeshIndex = subIdx;
      gpuMesh.subMeshTotal = converted.subMeshes.size();
      gpuMesh.textureName = subMesh.textureName;
      gpuMesh.fallbackBoneIndex = static_cast<int32_t>(subObj.boneIndex);
      gpuMesh.lodLevel = 0;
      gpuMesh.isAggregate = true;
      gpuMesh.hasSkinning = converted.hasSkinning;

      gpuMesh.cpuVertices = subMesh.vertices;
      gpuMesh.cpuIndices = subMesh.indices;

      gpuMesh.vertexBuffer.create(context, subMesh.vertices);
      gpuMesh.indexBuffer.create(context, subMesh.indices);

      combinedBounds_.expand(gfx::BoundingBox{subMesh.bounds.min, subMesh.bounds.max});

      skinnedMeshGPU_.push_back(std::move(gpuMesh));
    }
  }

  skinnedAggregateCount_ = skinnedMeshGPU_.size();

  for (size_t lodIdx = 0; lodIdx < lodLevels_.size(); ++lodIdx) {
    auto &levelInfo = lodLevels_[lodIdx];

    for (const auto &meshInfo : levelInfo.meshes) {
      auto converted = MeshConverter::convertSkinned(file.meshes[meshInfo.meshIndex],
                                                     static_cast<int32_t>(meshInfo.boneIndex));
      if (converted.subMeshes.empty()) {
        continue;
      }

      for (size_t subIdx = 0; subIdx < converted.subMeshes.size(); ++subIdx) {
        auto &subMesh = converted.subMeshes[subIdx];
        if (subMesh.vertices.empty() || subMesh.indices.empty()) {
          continue;
        }

        w3d_types::HLodSkinnedMeshGPU gpuMesh;
        gpuMesh.baseName = meshInfo.name;
        gpuMesh.name = meshInfo.name;
        if (converted.subMeshes.size() > 1) {
          gpuMesh.name += "_sub" + std::to_string(subIdx);
        }
        gpuMesh.subMeshIndex = subIdx;
        gpuMesh.subMeshTotal = converted.subMeshes.size();
        gpuMesh.textureName = subMesh.textureName;
        gpuMesh.fallbackBoneIndex = static_cast<int32_t>(meshInfo.boneIndex);
        gpuMesh.lodLevel = lodIdx;
        gpuMesh.isAggregate = false;
        gpuMesh.hasSkinning = converted.hasSkinning;

        gpuMesh.cpuVertices = subMesh.vertices;
        gpuMesh.cpuIndices = subMesh.indices;

        gpuMesh.vertexBuffer.create(context, subMesh.vertices);
        gpuMesh.indexBuffer.create(context, subMesh.indices);

        combinedBounds_.expand(gfx::BoundingBox{subMesh.bounds.min, subMesh.bounds.max});
        levelInfo.bounds.expand(gfx::BoundingBox{subMesh.bounds.min, subMesh.bounds.max});

        skinnedMeshGPU_.push_back(std::move(gpuMesh));
      }
    }
  }

  currentLOD_ = 0;

  // Initialize all skinned meshes as visible
  skinnedMeshVisibility_.resize(skinnedMeshGPU_.size(), true);
}

void HLodModel::setCurrentLOD(size_t level) {
  if (level < lodLevels_.size()) {
    currentLOD_ = level;
  }
}

float HLodModel::calculateScreenSize(float radius, float distance, float screenHeight,
                                     float fovY) const {
  if (distance <= 0.0f || radius <= 0.0f) {
    return 0.0f;
  }

  float angularSize = 2.0f * std::atan(radius / distance);

  float screenSize = (angularSize / fovY) * screenHeight;

  return screenSize;
}

void HLodModel::updateLOD(float screenHeight, float fovY, float cameraDistance) {
  if (selectionMode_ != w3d_types::LODSelectionMode::Auto || lodLevels_.empty()) {
    return;
  }

  float radius = combinedBounds_.radius();
  currentScreenSize_ = calculateScreenSize(radius, cameraDistance, screenHeight, fovY);

  currentLOD_ = 0;

  for (size_t i = 0; i < lodLevels_.size(); ++i) {
    if (lodLevels_[i].maxScreenSize > 0.0f && currentScreenSize_ < lodLevels_[i].maxScreenSize) {
      currentLOD_ = i;
    }
  }
}

size_t HLodModel::triangleCount(size_t meshIndex) const {
  if (meshIndex >= meshGPU_.size()) {
    return 0;
  }
  return meshGPU_[meshIndex].cpuIndices.size() / 3;
}

bool HLodModel::getTriangle(size_t meshIndex, size_t triangleIndex, glm::vec3 &v0, glm::vec3 &v1,
                            glm::vec3 &v2) const {
  if (meshIndex >= meshGPU_.size()) {
    return false;
  }

  const auto &mesh = meshGPU_[meshIndex];
  size_t baseIdx = triangleIndex * 3;

  if (baseIdx + 2 >= mesh.cpuIndices.size()) {
    return false;
  }

  uint32_t i0 = mesh.cpuIndices[baseIdx];
  uint32_t i1 = mesh.cpuIndices[baseIdx + 1];
  uint32_t i2 = mesh.cpuIndices[baseIdx + 2];

  if (i0 >= mesh.cpuVertices.size() || i1 >= mesh.cpuVertices.size() ||
      i2 >= mesh.cpuVertices.size()) {
    return false;
  }

  v0 = mesh.cpuVertices[i0].position;
  v1 = mesh.cpuVertices[i1].position;
  v2 = mesh.cpuVertices[i2].position;

  return true;
}

size_t HLodModel::skinnedTriangleCount(size_t meshIndex) const {
  if (meshIndex >= skinnedMeshGPU_.size()) {
    return 0;
  }
  return skinnedMeshGPU_[meshIndex].cpuIndices.size() / 3;
}

bool HLodModel::getSkinnedTriangle(size_t meshIndex, size_t triangleIndex, glm::vec3 &v0,
                                   glm::vec3 &v1, glm::vec3 &v2) const {
  if (meshIndex >= skinnedMeshGPU_.size()) {
    return false;
  }

  const auto &mesh = skinnedMeshGPU_[meshIndex];
  size_t baseIdx = triangleIndex * 3;

  if (baseIdx + 2 >= mesh.cpuIndices.size()) {
    return false;
  }

  uint32_t i0 = mesh.cpuIndices[baseIdx];
  uint32_t i1 = mesh.cpuIndices[baseIdx + 1];
  uint32_t i2 = mesh.cpuIndices[baseIdx + 2];

  if (i0 >= mesh.cpuVertices.size() || i1 >= mesh.cpuVertices.size() ||
      i2 >= mesh.cpuVertices.size()) {
    return false;
  }

  v0 = mesh.cpuVertices[i0].position;
  v1 = mesh.cpuVertices[i1].position;
  v2 = mesh.cpuVertices[i2].position;

  return true;
}

const std::string &HLodModel::meshName(size_t index) const {
  static const std::string empty;
  if (index >= meshGPU_.size()) {
    return empty;
  }
  return meshGPU_[index].name;
}

const std::string &HLodModel::skinnedMeshName(size_t index) const {
  static const std::string empty;
  if (index >= skinnedMeshGPU_.size()) {
    return empty;
  }
  return skinnedMeshGPU_[index].name;
}

bool HLodModel::isMeshVisible(size_t meshIndex) const {
  if (meshIndex >= meshGPU_.size()) {
    return false;
  }
  // Check user visibility first
  if (meshIndex < meshVisibility_.size() && !meshVisibility_[meshIndex]) {
    return false;
  }
  const auto &mesh = meshGPU_[meshIndex];
  return mesh.isAggregate || mesh.lodLevel == currentLOD_;
}

bool HLodModel::isSkinnedMeshVisible(size_t meshIndex) const {
  if (meshIndex >= skinnedMeshGPU_.size()) {
    return false;
  }
  // Check user visibility first
  if (meshIndex < skinnedMeshVisibility_.size() && !skinnedMeshVisibility_[meshIndex]) {
    return false;
  }
  const auto &mesh = skinnedMeshGPU_[meshIndex];
  return mesh.isAggregate || mesh.lodLevel == currentLOD_;
}

std::vector<size_t> HLodModel::visibleMeshIndices() const {
  std::vector<size_t> indices;
  indices.reserve(meshGPU_.size());

  for (size_t i = 0; i < meshGPU_.size(); ++i) {
    if (isMeshVisible(i)) {
      indices.push_back(i);
    }
  }

  return indices;
}

std::vector<size_t> HLodModel::visibleSkinnedMeshIndices() const {
  std::vector<size_t> indices;
  indices.reserve(skinnedMeshGPU_.size());

  for (size_t i = 0; i < skinnedMeshGPU_.size(); ++i) {
    if (isSkinnedMeshVisible(i)) {
      indices.push_back(i);
    }
  }

  return indices;
}

bool HLodModel::isMeshHidden(size_t index) const {
  if (index >= meshVisibility_.size()) {
    return false;
  }
  return !meshVisibility_[index];
}

void HLodModel::setMeshHidden(size_t index, bool hidden) {
  if (index >= meshVisibility_.size()) {
    meshVisibility_.resize(index + 1, true);
  }
  meshVisibility_[index] = !hidden;
}

void HLodModel::setAllMeshesHidden(bool hidden) {
  meshVisibility_.resize(meshGPU_.size(), !hidden);
}

bool HLodModel::isSkinnedMeshHidden(size_t index) const {
  if (index >= skinnedMeshVisibility_.size()) {
    return false;
  }
  return !skinnedMeshVisibility_[index];
}

void HLodModel::setSkinnedMeshHidden(size_t index, bool hidden) {
  if (index >= skinnedMeshVisibility_.size()) {
    skinnedMeshVisibility_.resize(index + 1, true);
  }
  skinnedMeshVisibility_[index] = !hidden;
}

void HLodModel::setAllSkinnedMeshesHidden(bool hidden) {
  skinnedMeshVisibility_.resize(skinnedMeshGPU_.size(), !hidden);
}

void HLodModel::draw(vk::CommandBuffer cmd) {
  for (size_t i = 0; i < aggregateCount_; ++i) {
    // Skip if user has hidden this mesh
    if (i < meshVisibility_.size() && !meshVisibility_[i]) {
      continue;
    }

    const auto &mesh = meshGPU_[i];

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

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }
}

bool HLodModel::isMeshHidden(size_t index) const {
  if (index >= meshVisibility_.size()) {
    return false;
  }
  return !meshVisibility_[index];
}

void HLodModel::setMeshHidden(size_t index, bool hidden) {
  if (index >= meshVisibility_.size()) {
    return;
  }
  meshVisibility_[index] = !hidden;
}

void HLodModel::setAllMeshesHidden(bool hidden) {
  std::fill(meshVisibility_.begin(), meshVisibility_.end(), !hidden);
}

bool HLodModel::isSkinnedMeshHidden(size_t index) const {
  if (index >= skinnedMeshVisibility_.size()) {
    return false;
  }
  return !skinnedMeshVisibility_[index];
}

void HLodModel::setSkinnedMeshHidden(size_t index, bool hidden) {
  if (index >= skinnedMeshVisibility_.size()) {
    return;
  }
  skinnedMeshVisibility_[index] = !hidden;
}

void HLodModel::setAllSkinnedMeshesHidden(bool hidden) {
  std::fill(skinnedMeshVisibility_.begin(), skinnedMeshVisibility_.end(), !hidden);
}

} // namespace w3d
