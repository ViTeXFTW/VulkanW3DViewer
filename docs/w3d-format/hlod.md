# HLod

W3D Hierarchical Level of Detail format documentation.

## Overview

HLod (Hierarchical Level of Detail) defines how complex models are assembled from multiple meshes and how they switch between detail levels.

## HLod Structure

```
HLOD (0x00000700) - Container
├── HLOD_HEADER (0x00000701)
├── HLOD_LOD_ARRAY (0x00000702) - Multiple
│   ├── HLOD_SUB_OBJECT_ARRAY_HEADER (0x00000703)
│   └── HLOD_SUB_OBJECT (0x00000704) - Multiple
├── HLOD_AGGREGATE_ARRAY (0x00000705) - Optional
└── HLOD_PROXY_ARRAY (0x00000706) - Optional
```

## HLod Header

`HLOD_HEADER` (0x00000701)

```cpp
struct HLodHeader {
  uint32_t version;           // Format version
  uint32_t lodCount;          // Number of LOD levels
  char modelName[16];         // HLod name
  char hierarchyName[16];     // Associated hierarchy
};
```

## LOD Arrays

Each LOD level contains a set of meshes to render at that detail level.

### LOD Array Header

`HLOD_SUB_OBJECT_ARRAY_HEADER` (0x00000703)

```cpp
struct HLodSubObjectArrayHeader {
  uint32_t modelCount;        // Number of sub-objects
  float maxScreenSize;        // Maximum screen size for this LOD
};
```

### Screen Size

The `maxScreenSize` determines when to switch LOD levels:

- **Higher values**: More detailed (closer)
- **Lower values**: Less detailed (farther)

LOD selection:

```cpp
int selectLOD(const HLod& hlod, float screenSize) {
  for (int i = 0; i < hlod.lodArrays.size(); i++) {
    if (screenSize <= hlod.lodArrays[i].maxScreenSize) {
      return i;
    }
  }
  return hlod.lodArrays.size() - 1;  // Lowest detail
}
```

### Sub-Objects

`HLOD_SUB_OBJECT` (0x00000704)

```cpp
struct HLodSubObject {
  uint32_t boneIndex;         // Bone to attach to
  char name[32];              // Mesh name reference
};
```

**Reading sub-objects**:

```cpp
HLodSubObject readSubObject(ChunkReader& reader) {
  HLodSubObject obj;
  obj.boneIndex = reader.read<uint32_t>();
  obj.name = reader.readString(32);
  return obj;
}
```

## Model Assembly

### Bone Attachment

Each sub-object is attached to a bone:

```cpp
glm::mat4 getSubObjectTransform(
    const HLodSubObject& subObj,
    const std::vector<glm::mat4>& boneMatrices
) {
  return boneMatrices[subObj.boneIndex];
}
```

### Mesh Lookup

Sub-object names reference meshes in the same file:

```cpp
Mesh* findMesh(const W3DFile& file, const std::string& name) {
  for (auto& mesh : file.meshes) {
    if (mesh.header.meshName == name) {
      return &mesh;
    }
  }
  return nullptr;
}
```

### Full Assembly

```cpp
void assembleHLodModel(
    const HLod& hlod,
    const W3DFile& file,
    int lodLevel,
    const std::vector<glm::mat4>& boneMatrices,
    std::vector<RenderMesh>& output
) {
  const auto& lodArray = hlod.lodArrays[lodLevel];

  for (const auto& subObj : lodArray.subObjects) {
    Mesh* mesh = findMesh(file, subObj.name);
    if (!mesh) continue;

    RenderMesh rm;
    rm.mesh = mesh;
    rm.transform = boneMatrices[subObj.boneIndex];
    output.push_back(rm);
  }
}
```

## Aggregates

`HLOD_AGGREGATE_ARRAY` (0x00000705)

Aggregates are always-visible objects that don't participate in LOD switching:

```cpp
struct HLodAggregateArray {
  std::vector<HLodSubObject> aggregates;
};
```

Usage:

- Shadow volumes
- Collision meshes
- Helper objects

## Proxies

`HLOD_PROXY_ARRAY` (0x00000706)

Proxies are placeholder objects for external references:

```cpp
struct HLodProxyArray {
  std::vector<HLodSubObject> proxies;
};
```

Usage:

- Attachment points for weapons
- Vehicle occupant positions
- Effect spawn points

## LOD Switching

### Screen Size Calculation

Screen size is typically calculated as:

```cpp
float calculateScreenSize(
    const BoundingBox& bounds,
    const Camera& camera
) {
  float diameter = glm::length(bounds.max - bounds.min);
  float distance = glm::length(camera.position - bounds.center);

  // Approximate screen coverage
  return diameter / distance;
}
```

### Hysteresis

To prevent LOD "popping", use hysteresis:

```cpp
class LODSelector {
  int currentLOD = 0;
  float hysteresis = 0.1f;  // 10% buffer

  int selectLOD(const HLod& hlod, float screenSize) {
    // Check if we should switch down (more detail)
    if (currentLOD > 0) {
      float threshold = hlod.lodArrays[currentLOD - 1].maxScreenSize;
      if (screenSize > threshold * (1.0f + hysteresis)) {
        currentLOD--;
      }
    }

    // Check if we should switch up (less detail)
    if (currentLOD < hlod.lodArrays.size() - 1) {
      float threshold = hlod.lodArrays[currentLOD].maxScreenSize;
      if (screenSize < threshold * (1.0f - hysteresis)) {
        currentLOD++;
      }
    }

    return currentLOD;
  }
};
```

## Typical LOD Configuration

| LOD | Max Screen Size | Description |
|-----|-----------------|-------------|
| 0 | 1.0 | Highest detail (close-up) |
| 1 | 0.5 | Medium detail |
| 2 | 0.25 | Low detail |
| 3 | 0.1 | Lowest detail (far away) |

## HLod Without Hierarchy

Some simple HLod models don't use a hierarchy:

```cpp
bool hasHierarchy(const HLod& hlod) {
  return !hlod.hierarchyName.empty() &&
         hlod.hierarchyName != "ROOTTRANSFORM";
}
```

For these models, all sub-objects use identity transform (bone index 0).

## Debugging HLod

```cpp
void printHLod(const HLod& hlod) {
  std::cout << "HLod: " << hlod.name << "\n";
  std::cout << "Hierarchy: " << hlod.hierarchyName << "\n";
  std::cout << "LOD Levels: " << hlod.lodArrays.size() << "\n";

  for (size_t i = 0; i < hlod.lodArrays.size(); i++) {
    const auto& lod = hlod.lodArrays[i];
    std::cout << "  LOD " << i << " (max size: "
              << lod.maxScreenSize << "):\n";

    for (const auto& obj : lod.subObjects) {
      std::cout << "    - " << obj.name
                << " (bone " << obj.boneIndex << ")\n";
    }
  }
}
```
