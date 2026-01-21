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
  lodLevels_.clear();
  aggregateCount_ = 0;
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
      if (converted.vertices.empty() || converted.indices.empty()) {
        continue;
      }

      HLodMeshGPU gpuMesh;
      gpuMesh.name = converted.name;
      gpuMesh.boneIndex = -1;
      gpuMesh.lodLevel = 0;
      gpuMesh.isAggregate = false;

      gpuMesh.vertexBuffer.create(context, converted.vertices);
      gpuMesh.indexBuffer.create(context, converted.indices);

      combinedBounds_.expand(converted.bounds);
      lodLevels_[0].bounds.expand(converted.bounds);

      meshGPU_.push_back(std::move(gpuMesh));
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
    if (converted.vertices.empty() || converted.indices.empty()) {
      continue;
    }

    // Apply bone transform if available
    if (pose && subObj.boneIndex < pose->boneCount()) {
      glm::mat4 boneTransform = pose->boneTransform(subObj.boneIndex);
      MeshConverter::applyBoneTransform(converted, boneTransform);
    }

    HLodMeshGPU gpuMesh;
    gpuMesh.name = subObj.name;
    gpuMesh.boneIndex = static_cast<int32_t>(subObj.boneIndex);
    gpuMesh.lodLevel = 0; // Aggregates don't have a specific LOD level
    gpuMesh.isAggregate = true;

    gpuMesh.vertexBuffer.create(context, converted.vertices);
    gpuMesh.indexBuffer.create(context, converted.indices);

    combinedBounds_.expand(converted.bounds);

    uploadedMeshes[meshIdx.value()] = meshGPU_.size();
    meshGPU_.push_back(std::move(gpuMesh));
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
      if (converted.vertices.empty() || converted.indices.empty()) {
        continue;
      }

      // Apply bone transform if available
      if (pose && meshInfo.boneIndex < pose->boneCount()) {
        glm::mat4 boneTransform = pose->boneTransform(meshInfo.boneIndex);
        MeshConverter::applyBoneTransform(converted, boneTransform);
      }

      HLodMeshGPU gpuMesh;
      gpuMesh.name = meshInfo.name;
      gpuMesh.boneIndex = static_cast<int32_t>(meshInfo.boneIndex);
      gpuMesh.lodLevel = lodIdx;
      gpuMesh.isAggregate = false;

      gpuMesh.vertexBuffer.create(context, converted.vertices);
      gpuMesh.indexBuffer.create(context, converted.indices);

      combinedBounds_.expand(converted.bounds);
      levelInfo.bounds.expand(converted.bounds);

      meshGPU_.push_back(std::move(gpuMesh));
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
