#include "hlod_model.hpp"

#include "core/vulkan_context.hpp"

#include <algorithm>
#include <cmath>

#include "mesh_converter.hpp"

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

  for (auto &mesh : skinnedMeshGPU_) {
    mesh.vertexBuffer.destroy();
    mesh.indexBuffer.destroy();
  }
  skinnedMeshGPU_.clear();

  lodLevels_.clear();
  aggregateCount_ = 0;
  skinnedAggregateCount_ = 0;
  currentLOD_ = 0;
  combinedBounds_ = BoundingBox{};
  name_.clear();
  hierarchyName_.clear();
}

std::unordered_map<std::string, size_t> HLodModel::buildMeshNameMap(const W3DFile &file) {
  std::unordered_map<std::string, size_t> nameMap;

  for (size_t i = 0; i < file.meshes.size(); ++i) {
    const auto &mesh = file.meshes[i];

    // Store both full name and short name
    std::string fullName = mesh.header.containerName + "." + mesh.header.meshName;
    nameMap[fullName] = i;
    nameMap[mesh.header.meshName] = i;
  }

  return nameMap;
}

std::optional<size_t>
HLodModel::findMeshIndex(const std::unordered_map<std::string, size_t> &nameMap,
                         const W3DFile & /*file*/, const std::string &name) {
  // Try exact match first
  auto it = nameMap.find(name);
  if (it != nameMap.end()) {
    return it->second;
  }

  // Try matching just the mesh name portion (after the dot)
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

void HLodModel::load(VulkanContext &context, const W3DFile &file, const SkeletonPose *pose) {
  destroy();

  // Check if we have an HLod to process
  if (file.hlods.empty()) {
    // No HLod - fall back to loading all meshes as a single LOD level
    // This handles simple mesh files without hierarchy
    auto meshNameMap = buildMeshNameMap(file);

    HLodLevelInfo level0;
    level0.maxScreenSize = 0.0f; // Highest detail

    for (size_t i = 0; i < file.meshes.size(); ++i) {
      HLodMeshInfo info;
      info.meshIndex = i;
      info.boneIndex = 0;
      info.name = file.meshes[i].header.meshName;
      level0.meshes.push_back(info);
    }

    lodLevels_.push_back(level0);

    // Convert and upload all meshes
    for (size_t i = 0; i < file.meshes.size(); ++i) {
      auto converted = MeshConverter::convert(file.meshes[i]);
      if (converted.subMeshes.empty()) {
        continue;
      }

      // Create one GPU mesh for each sub-mesh (each with different texture)
      for (size_t subIdx = 0; subIdx < converted.subMeshes.size(); ++subIdx) {
        const auto &subMesh = converted.subMeshes[subIdx];
        if (subMesh.vertices.empty() || subMesh.indices.empty()) {
          continue;
        }

        HLodMeshGPU gpuMesh;
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

        // Store CPU copies for ray-triangle intersection
        gpuMesh.cpuVertices = subMesh.vertices;
        gpuMesh.cpuIndices = subMesh.indices;

        gpuMesh.vertexBuffer.create(context, subMesh.vertices);
        gpuMesh.indexBuffer.create(context, subMesh.indices);

        combinedBounds_.expand(subMesh.bounds);
        lodLevels_[0].bounds.expand(subMesh.bounds);

        meshGPU_.push_back(std::move(gpuMesh));
      }
    }

    return;
  }

  // Use first HLod
  const auto &hlod = file.hlods[0];
  name_ = hlod.name;
  hierarchyName_ = hlod.hierarchyName;

  auto meshNameMap = buildMeshNameMap(file);

  // Process LOD levels (stored in reverse order in W3D - highest detail first)
  // LOD arrays are stored with index 0 being highest detail (smallest maxScreenSize)
  lodLevels_.resize(hlod.lodArrays.size());

  for (size_t lodIdx = 0; lodIdx < hlod.lodArrays.size(); ++lodIdx) {
    const auto &lodArray = hlod.lodArrays[lodIdx];
    auto &levelInfo = lodLevels_[lodIdx];

    levelInfo.maxScreenSize = lodArray.maxScreenSize;
    levelInfo.meshes.reserve(lodArray.subObjects.size());

    for (const auto &subObj : lodArray.subObjects) {
      auto meshIdx = findMeshIndex(meshNameMap, file, subObj.name);
      if (meshIdx.has_value()) {
        HLodMeshInfo info;
        info.meshIndex = meshIdx.value();
        info.boneIndex = subObj.boneIndex;
        info.name = subObj.name;
        levelInfo.meshes.push_back(info);
      }
    }
  }

  // Track which meshes have been uploaded to avoid duplicates
  std::unordered_map<size_t, size_t> uploadedMeshes; // meshIndex -> gpuIndex

  // Process aggregates first (always rendered)
  for (const auto &subObj : hlod.aggregates) {
    auto meshIdx = findMeshIndex(meshNameMap, file, subObj.name);
    if (!meshIdx.has_value()) {
      continue;
    }

    auto converted = MeshConverter::convert(file.meshes[meshIdx.value()]);
    if (converted.subMeshes.empty()) {
      continue;
    }

    // Apply bone transform if available
    if (pose && subObj.boneIndex < pose->boneCount()) {
      glm::mat4 boneTransform = pose->boneTransform(subObj.boneIndex);
      MeshConverter::applyBoneTransform(converted, boneTransform);
    }

    // Create one GPU mesh for each sub-mesh
    for (size_t subIdx = 0; subIdx < converted.subMeshes.size(); ++subIdx) {
      const auto &subMesh = converted.subMeshes[subIdx];
      if (subMesh.vertices.empty() || subMesh.indices.empty()) {
        continue;
      }

      HLodMeshGPU gpuMesh;
      gpuMesh.baseName = subObj.name;
      gpuMesh.name = subObj.name;
      if (converted.subMeshes.size() > 1) {
        gpuMesh.name += "_sub" + std::to_string(subIdx);
      }
      gpuMesh.subMeshIndex = subIdx;
      gpuMesh.subMeshTotal = converted.subMeshes.size();
      gpuMesh.textureName = subMesh.textureName;
      gpuMesh.boneIndex = static_cast<int32_t>(subObj.boneIndex);
      gpuMesh.lodLevel = 0; // Aggregates don't have a specific LOD level
      gpuMesh.isAggregate = true;

      // Store CPU copies for ray-triangle intersection
      gpuMesh.cpuVertices = subMesh.vertices;
      gpuMesh.cpuIndices = subMesh.indices;

      gpuMesh.vertexBuffer.create(context, subMesh.vertices);
      gpuMesh.indexBuffer.create(context, subMesh.indices);

      combinedBounds_.expand(subMesh.bounds);

      meshGPU_.push_back(std::move(gpuMesh));
    }

    uploadedMeshes[meshIdx.value()] = meshGPU_.size();
  }

  aggregateCount_ = meshGPU_.size();

  // Process each LOD level's meshes
  for (size_t lodIdx = 0; lodIdx < lodLevels_.size(); ++lodIdx) {
    auto &levelInfo = lodLevels_[lodIdx];

    for (const auto &meshInfo : levelInfo.meshes) {
      // Check if this mesh is already uploaded (might be shared across LOD levels)
      // Note: Different LOD levels typically have different meshes, but we track anyway
      auto it = uploadedMeshes.find(meshInfo.meshIndex);
      if (it != uploadedMeshes.end()) {
        // Mesh already uploaded - update its LOD level if this is a higher detail LOD
        // For simplicity, we'll upload separate copies for each LOD level
        // This allows different bone transforms per LOD if needed
      }

      auto converted = MeshConverter::convert(file.meshes[meshInfo.meshIndex]);
      if (converted.subMeshes.empty()) {
        continue;
      }

      // Apply bone transform if available
      if (pose && meshInfo.boneIndex < pose->boneCount()) {
        glm::mat4 boneTransform = pose->boneTransform(meshInfo.boneIndex);
        MeshConverter::applyBoneTransform(converted, boneTransform);
      }

      // Create one GPU mesh for each sub-mesh
      for (size_t subIdx = 0; subIdx < converted.subMeshes.size(); ++subIdx) {
        const auto &subMesh = converted.subMeshes[subIdx];
        if (subMesh.vertices.empty() || subMesh.indices.empty()) {
          continue;
        }

        HLodMeshGPU gpuMesh;
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

        // Store CPU copies for ray-triangle intersection
        gpuMesh.cpuVertices = subMesh.vertices;
        gpuMesh.cpuIndices = subMesh.indices;

        gpuMesh.vertexBuffer.create(context, subMesh.vertices);
        gpuMesh.indexBuffer.create(context, subMesh.indices);

        combinedBounds_.expand(subMesh.bounds);
        levelInfo.bounds.expand(subMesh.bounds);

        meshGPU_.push_back(std::move(gpuMesh));
      }
    }
  }

  // Default to highest detail LOD
  currentLOD_ = 0;
}

void HLodModel::loadSkinned(VulkanContext &context, const W3DFile &file) {
  destroy();

  // Check if we have an HLod to process
  if (file.hlods.empty()) {
    // No HLod - fall back to loading all meshes as a single LOD level
    HLodLevelInfo level0;
    level0.maxScreenSize = 0.0f;

    for (size_t i = 0; i < file.meshes.size(); ++i) {
      HLodMeshInfo info;
      info.meshIndex = i;
      info.boneIndex = 0;
      info.name = file.meshes[i].header.meshName;
      level0.meshes.push_back(info);
    }

    lodLevels_.push_back(level0);

    // Convert all meshes to skinned format
    auto skinnedMeshes = MeshConverter::convertAllSkinned(file);
    for (auto &converted : skinnedMeshes) {
      for (size_t subIdx = 0; subIdx < converted.subMeshes.size(); ++subIdx) {
        auto &subMesh = converted.subMeshes[subIdx];
        if (subMesh.vertices.empty() || subMesh.indices.empty()) {
          continue;
        }

        HLodSkinnedMeshGPU gpuMesh;
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

        // Store CPU copies for ray-triangle intersection
        gpuMesh.cpuVertices = subMesh.vertices;
        gpuMesh.cpuIndices = subMesh.indices;

        gpuMesh.vertexBuffer.create(context, subMesh.vertices);
        gpuMesh.indexBuffer.create(context, subMesh.indices);

        combinedBounds_.expand(subMesh.bounds);
        lodLevels_[0].bounds.expand(subMesh.bounds);

        skinnedMeshGPU_.push_back(std::move(gpuMesh));
      }
    }

    return;
  }

  // Use first HLod
  const auto &hlod = file.hlods[0];
  name_ = hlod.name;
  hierarchyName_ = hlod.hierarchyName;

  auto meshNameMap = buildMeshNameMap(file);

  // Process LOD levels
  lodLevels_.resize(hlod.lodArrays.size());

  for (size_t lodIdx = 0; lodIdx < hlod.lodArrays.size(); ++lodIdx) {
    const auto &lodArray = hlod.lodArrays[lodIdx];
    auto &levelInfo = lodLevels_[lodIdx];

    levelInfo.maxScreenSize = lodArray.maxScreenSize;
    levelInfo.meshes.reserve(lodArray.subObjects.size());

    for (const auto &subObj : lodArray.subObjects) {
      auto meshIdx = findMeshIndex(meshNameMap, file, subObj.name);
      if (meshIdx.has_value()) {
        HLodMeshInfo info;
        info.meshIndex = meshIdx.value();
        info.boneIndex = subObj.boneIndex;
        info.name = subObj.name;
        levelInfo.meshes.push_back(info);
      }
    }
  }

  // Process aggregates first (always rendered)
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

      HLodSkinnedMeshGPU gpuMesh;
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

      // Store CPU copies for ray-triangle intersection
      gpuMesh.cpuVertices = subMesh.vertices;
      gpuMesh.cpuIndices = subMesh.indices;

      gpuMesh.vertexBuffer.create(context, subMesh.vertices);
      gpuMesh.indexBuffer.create(context, subMesh.indices);

      combinedBounds_.expand(subMesh.bounds);

      skinnedMeshGPU_.push_back(std::move(gpuMesh));
    }
  }

  skinnedAggregateCount_ = skinnedMeshGPU_.size();

  // Process each LOD level's meshes
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

        HLodSkinnedMeshGPU gpuMesh;
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

        // Store CPU copies for ray-triangle intersection
        gpuMesh.cpuVertices = subMesh.vertices;
        gpuMesh.cpuIndices = subMesh.indices;

        gpuMesh.vertexBuffer.create(context, subMesh.vertices);
        gpuMesh.indexBuffer.create(context, subMesh.indices);

        combinedBounds_.expand(subMesh.bounds);
        levelInfo.bounds.expand(subMesh.bounds);

        skinnedMeshGPU_.push_back(std::move(gpuMesh));
      }
    }
  }

  // Default to highest detail LOD
  currentLOD_ = 0;
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

  // Calculate the projected size of a sphere on screen
  // This approximates the screen coverage of the bounding sphere

  // Angular size of the object (in radians)
  float angularSize = 2.0f * std::atan(radius / distance);

  // Convert to screen pixels using the vertical FOV
  // The screen height corresponds to fovY radians
  float screenSize = (angularSize / fovY) * screenHeight;

  return screenSize;
}

void HLodModel::updateLOD(float screenHeight, float fovY, float cameraDistance) {
  if (selectionMode_ != LODSelectionMode::Auto || lodLevels_.empty()) {
    return;
  }

  // Calculate screen size based on model bounds
  float radius = combinedBounds_.radius();
  currentScreenSize_ = calculateScreenSize(radius, cameraDistance, screenHeight, fovY);

  // Select appropriate LOD level
  // LOD arrays are ordered from highest detail (index 0) to lowest detail
  // maxScreenSize indicates the maximum screen size at which this LOD should be used
  // Larger maxScreenSize = lower detail LOD (used when object is smaller on screen)

  // Start with highest detail
  currentLOD_ = 0;

  // Walk through LOD levels from highest to lowest detail
  // Switch to lower detail if our screen size is below the threshold
  for (size_t i = 0; i < lodLevels_.size(); ++i) {
    // maxScreenSize of 0 means "always use this LOD" (highest detail)
    if (lodLevels_[i].maxScreenSize > 0.0f && currentScreenSize_ < lodLevels_[i].maxScreenSize) {
      // This LOD is appropriate for our screen size
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
  const auto &mesh = meshGPU_[meshIndex];
  // Aggregates are always visible, otherwise check LOD level
  return mesh.isAggregate || mesh.lodLevel == currentLOD_;
}

bool HLodModel::isSkinnedMeshVisible(size_t meshIndex) const {
  if (meshIndex >= skinnedMeshGPU_.size()) {
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

void HLodModel::draw(vk::CommandBuffer cmd) const {
  // Draw aggregates first (always rendered)
  for (size_t i = 0; i < aggregateCount_; ++i) {
    const auto &mesh = meshGPU_[i];

    vk::Buffer vertexBuffers[] = {mesh.vertexBuffer.buffer()};
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    cmd.bindIndexBuffer(mesh.indexBuffer.buffer(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(mesh.indexBuffer.indexCount(), 1, 0, 0, 0);
  }

  // Draw current LOD level meshes
  for (size_t i = aggregateCount_; i < meshGPU_.size(); ++i) {
    const auto &mesh = meshGPU_[i];

    // Skip if not in current LOD level
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

} // namespace w3d
