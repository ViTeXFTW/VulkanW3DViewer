# API Reference

Quick reference for the main classes and functions in VulkanW3DViewer.

## Overview

This page provides a high-level API overview. For detailed documentation, see the header files in the source code.

## W3D Module

### Loading Files

```cpp
#include "w3d/w3d.hpp"

// Load a W3D file
w3d::W3DFile file = w3d::load("model.w3d");

// Access components
for (const auto& mesh : file.meshes) {
  // Process mesh
}

for (const auto& hierarchy : file.hierarchies) {
  // Process skeleton
}

for (const auto& animation : file.animations) {
  // Process animation
}
```

### W3DFile Structure

```cpp
namespace w3d {

struct W3DFile {
  std::vector<Mesh> meshes;
  std::vector<Hierarchy> hierarchies;
  std::vector<Animation> animations;
  std::vector<CompressedAnimation> compressedAnimations;
  std::vector<HLod> hlods;
  std::vector<Box> boxes;
};

}
```

### Mesh

```cpp
namespace w3d {

struct Mesh {
  MeshHeader header;

  // Geometry
  std::vector<Vector3> vertices;
  std::vector<Vector3> normals;
  std::vector<Vector2> texCoords;
  std::vector<Triangle> triangles;

  // Skinning
  std::vector<VertexInfluence> vertexInfluences;

  // Materials
  std::vector<TextureDef> textures;
  std::vector<MaterialPass> materialPasses;
};

struct MeshHeader {
  uint32_t version;
  uint32_t attributes;
  std::string meshName;
  uint32_t numTris;
  uint32_t numVertices;
  Vector3 min, max;      // Bounding box
  float sphRadius;       // Bounding sphere
};

}
```

### Hierarchy

```cpp
namespace w3d {

struct Hierarchy {
  std::string name;
  std::vector<Pivot> pivots;
};

struct Pivot {
  std::string name;
  uint32_t parentIndex;  // 0xFFFFFFFF = root
  Vector3 translation;
  Quaternion rotation;
};

}
```

### Animation

```cpp
namespace w3d {

struct Animation {
  std::string name;
  std::string hierarchyName;
  uint32_t numFrames;
  uint32_t frameRate;
  std::vector<AnimChannel> channels;
};

struct AnimChannel {
  uint16_t pivot;         // Bone index
  uint16_t flags;         // Channel type
  std::vector<float> data;
};

}
```

### HLod

```cpp
namespace w3d {

struct HLod {
  std::string name;
  std::string hierarchyName;
  std::vector<HLodArray> lodArrays;
};

struct HLodArray {
  float maxScreenSize;
  std::vector<HLodSubObject> subObjects;
};

struct HLodSubObject {
  uint32_t boneIndex;
  std::string name;
};

}
```

## Core Module

### VulkanContext

```cpp
namespace w3d {

class VulkanContext {
public:
  VulkanContext(GLFWwindow* window);

  // Frame management
  uint32_t beginFrame();
  void endFrame(uint32_t imageIndex);

  // Accessors
  vk::Device device() const;
  vk::Queue graphicsQueue() const;
  vk::CommandBuffer commandBuffer() const;
  vk::Extent2D swapchainExtent() const;
};

}
```

### Buffer

```cpp
namespace w3d {

class Buffer {
public:
  // Create with immediate upload
  Buffer(VulkanContext& context,
         size_t size,
         vk::BufferUsageFlags usage,
         const void* data = nullptr);

  vk::Buffer buffer() const;
  size_t size() const;

  void* map();
  void unmap();
};

}
```

### Pipeline

```cpp
namespace w3d {

class Pipeline {
public:
  Pipeline(VulkanContext& context,
           const std::string& vertShader,
           const std::string& fragShader);

  void bind(vk::CommandBuffer cmd);

  vk::PipelineLayout layout() const;
  vk::DescriptorSet descriptorSet() const;
};

}
```

## Render Module

### HLodModel

```cpp
namespace w3d {

class HLodModel {
public:
  HLodModel(VulkanContext& context,
            const W3DFile& file,
            const HLod& hlod);

  void draw(vk::CommandBuffer cmd,
            const Pipeline& pipeline);

  void setAnimation(const Animation* anim);
  void update(float deltaTime);

  int getLODCount() const;
  int getCurrentLOD() const;
  void setForcedLOD(int lod);  // -1 for auto
};

}
```

### Camera

```cpp
namespace w3d {

class Camera {
public:
  Camera();

  // Transforms
  glm::mat4 viewMatrix() const;
  glm::mat4 projectionMatrix(float aspect) const;

  // Controls
  void onMouseDrag(float dx, float dy);
  void onScroll(float delta);

  // Settings
  void setFOV(float degrees);
  void setTarget(const glm::vec3& target);
  void setDistance(float distance);
};

}
```

### AnimationPlayer

```cpp
namespace w3d {

class AnimationPlayer {
public:
  AnimationPlayer();

  void setAnimation(const Animation* anim);

  void play();
  void pause();
  void stop();

  void setSpeed(float speed);
  void setLoop(bool loop);
  void setFrame(float frame);

  void update(float deltaTime);

  bool isPlaying() const;
  float getCurrentFrame() const;
  float getDuration() const;
};

}
```

### Skeleton

```cpp
namespace w3d {

class Skeleton {
public:
  Skeleton(const Hierarchy& hierarchy);

  void resetToRestPose();
  void applyAnimation(const Animation& anim, float frame);

  const glm::mat4& getBoneMatrix(int index) const;
  int getBoneCount() const;
};

}
```

## UI Module

### UIManager

```cpp
namespace w3d {

class UIManager {
public:
  UIManager(UIContext& context);

  void update();
  void render();

  template<typename T>
  T* getWindow();
};

}
```

### UIContext

```cpp
namespace w3d {

struct UIContext {
  HLodModel* currentModel = nullptr;
  Camera* camera = nullptr;
  AnimationPlayer* animPlayer = nullptr;

  bool showSkeleton = false;
  bool showBoundingBox = false;
  int forcedLOD = -1;

  void log(const std::string& message);
};

}
```

## Utility Functions

### Math Conversion

```cpp
// W3D quaternion to GLM
glm::quat toGlmQuat(const w3d::Quaternion& q) {
  return glm::quat(q.w, q.x, q.y, q.z);
}

// W3D vector to GLM
glm::vec3 toGlmVec3(const w3d::Vector3& v) {
  return glm::vec3(v.x, v.y, v.z);
}

// UV conversion (V-flip)
glm::vec2 toVulkanUV(const w3d::Vector2& uv) {
  return glm::vec2(uv.u, 1.0f - uv.v);
}
```

### Chunk Type Names

```cpp
const char* w3d::ChunkTypeName(ChunkType type);

// Usage
std::cout << ChunkTypeName(ChunkType::MESH);  // "MESH"
```

## Error Handling

Most functions throw `std::runtime_error` on failure:

```cpp
try {
  auto file = w3d::load("model.w3d");
} catch (const std::runtime_error& e) {
  std::cerr << "Load failed: " << e.what() << std::endl;
}
```

Optional results use `std::optional`:

```cpp
std::optional<Mesh*> findMesh(const std::string& name);

if (auto mesh = findMesh("body")) {
  // Use *mesh
}
```
