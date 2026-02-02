# Meshes

W3D mesh format documentation.

## Overview

Meshes contain 3D geometry with materials and optional skinning data.

## Mesh Header

`MESH_HEADER3` (0x0000001F) - 116 bytes

```cpp
struct MeshHeader3 {
  uint32_t version;           // Format version
  uint32_t attributes;        // Mesh flags
  char meshName[16];          // Mesh name
  char containerName[16];     // Container name
  uint32_t numTris;           // Triangle count
  uint32_t numVertices;       // Vertex count
  uint32_t numMaterials;      // Material count
  uint32_t numDamageStages;   // Damage stage count
  int32_t sortLevel;          // Sort priority
  uint32_t prelitVersion;     // Prelit version
  uint32_t futureCounts;      // Reserved
  uint32_t vertexChannels;    // Vertex data flags
  uint32_t faceChannels;      // Face data flags
  Vector3 min;                // Bounding box min
  Vector3 max;                // Bounding box max
  Vector3 sphCenter;          // Bounding sphere center
  float sphRadius;            // Bounding sphere radius
};
```

## Mesh Attributes

Bit flags in `attributes` field:

### Collision Flags

```cpp
namespace MeshFlags {
  constexpr uint32_t COLLISION_TYPE_MASK     = 0x00000FF0;
  constexpr uint32_t COLLISION_TYPE_PHYSICAL = 0x00000010;
  constexpr uint32_t COLLISION_TYPE_PROJECTILE = 0x00000020;
  constexpr uint32_t COLLISION_TYPE_VIS      = 0x00000040;
  constexpr uint32_t COLLISION_TYPE_CAMERA   = 0x00000080;
  constexpr uint32_t COLLISION_TYPE_VEHICLE  = 0x00000100;
}
```

### Render Flags

```cpp
namespace MeshFlags {
  constexpr uint32_t HIDDEN      = 0x00001000;
  constexpr uint32_t TWO_SIDED   = 0x00002000;
  constexpr uint32_t CAST_SHADOW = 0x00008000;
}
```

### Geometry Type

```cpp
namespace MeshFlags {
  constexpr uint32_t GEOMETRY_TYPE_MASK           = 0x00FF0000;
  constexpr uint32_t GEOMETRY_TYPE_NORMAL         = 0x00000000;
  constexpr uint32_t GEOMETRY_TYPE_CAMERA_ALIGNED = 0x00010000;
  constexpr uint32_t GEOMETRY_TYPE_SKIN           = 0x00020000;  // Skinned mesh
  constexpr uint32_t GEOMETRY_TYPE_CAMERA_ORIENTED = 0x00060000;
}
```

### Prelit Flags

```cpp
namespace MeshFlags {
  constexpr uint32_t PRELIT_MASK    = 0x0F000000;
  constexpr uint32_t PRELIT_UNLIT   = 0x01000000;
  constexpr uint32_t PRELIT_VERTEX  = 0x02000000;
  constexpr uint32_t PRELIT_LIGHTMAP_MULTI_PASS = 0x04000000;
  constexpr uint32_t PRELIT_LIGHTMAP_MULTI_TEXTURE = 0x08000000;
}
```

## Geometry Data

### Vertices

`VERTICES` (0x00000002) - Array of Vector3

```cpp
struct Vector3 {
  float x, y, z;
};

// Reading
std::vector<Vector3> vertices(header.numVertices);
reader.read(vertices.data(), vertices.size() * sizeof(Vector3));
```

### Normals

`VERTEX_NORMALS` (0x00000003) - Array of Vector3

Same format as vertices. One normal per vertex.

### Triangles

`TRIANGLES` (0x00000020) - Array of Triangle

```cpp
struct Triangle {
  uint32_t vertexIndices[3];  // Indices into vertex array
  uint32_t attributes;        // Surface attributes
  Vector3 normal;             // Face normal
  float distance;             // Plane distance
};
```

### UV Coordinates

`TEXCOORDS` (0x0000000D) or `STAGE_TEXCOORDS` (0x0000004A)

```cpp
struct Vector2 {
  float u, v;
};

// Reading
std::vector<Vector2> texCoords(count);
reader.read(texCoords.data(), texCoords.size() * sizeof(Vector2));

// Convert for Vulkan
for (auto& tc : texCoords) {
  tc.v = 1.0f - tc.v;  // V-flip required
}
```

### Per-Face UV Indices

`PER_FACE_TEXCOORD_IDS` (0x0000004B)

When present, UV coordinates are indexed per-face vertex, not per-vertex:

```cpp
// Array of indices, 3 per triangle
std::vector<uint32_t> perFaceUVIds(header.numTris * 3);
```

This requires "unrolling" the mesh to create unique vertices for rendering.

### Vertex Colors

`VERTEX_COLORS` (0x00000115) - Array of RGBA

```cpp
struct RGBA {
  uint8_t r, g, b, a;
};
```

## Skinning Data

### Vertex Influences

`VERTEX_INFLUENCES` (0x0000000E)

```cpp
struct VertexInfluence {
  uint16_t boneIndex;   // Primary bone
  uint16_t boneIndex2;  // Secondary bone (if used)
  // Weights stored separately or implied
};
```

For single-bone skinning, weight is 1.0 for the primary bone.

For two-bone skinning, weights sum to 1.0.

## Material System

### Material Info

`MATERIAL_INFO` (0x00000028)

```cpp
struct MaterialInfo {
  uint32_t passCount;           // Number of material passes
  uint32_t vertexMaterialCount; // Number of vertex materials
  uint32_t shaderCount;         // Number of shaders
  uint32_t textureCount;        // Number of textures
};
```

### Vertex Material

`VERTEX_MATERIAL_INFO` (0x0000002D)

```cpp
struct VertexMaterialInfo {
  uint32_t attributes;
  RGB ambient;
  RGB diffuse;
  RGB specular;
  RGB emissive;
  float shininess;
  float opacity;
  float translucency;
};
```

### Shader Definition

`SHADERS` (0x00000029) - Array of ShaderDef

```cpp
struct ShaderDef {
  uint8_t depthCompare;      // Depth test function
  uint8_t depthMask;         // Depth write enable
  uint8_t colorMask;         // Color write mask
  uint8_t destBlend;         // Destination blend
  uint8_t fogFunc;           // Fog function
  uint8_t priGradient;       // Primary gradient
  uint8_t secGradient;       // Secondary gradient
  uint8_t srcBlend;          // Source blend
  uint8_t texturing;         // Texturing enable
  uint8_t detailColorFunc;   // Detail color function
  uint8_t detailAlphaFunc;   // Detail alpha function
  uint8_t shaderPreset;      // Preset index
  uint8_t alphaTest;         // Alpha test enable
  uint8_t postDetailColorFunc;
  uint8_t postDetailAlphaFunc;
  uint8_t padding;
};
```

### Texture Reference

`TEXTURE_NAME` (0x00000032) - Null-terminated string

```cpp
char textureName[256];  // Variable length
```

## Material Pass

`MATERIAL_PASS` (0x00000038) - Container

Contains mappings from vertices to materials/shaders/textures:

- `VERTEX_MATERIAL_IDS` - Per-vertex material index
- `SHADER_IDS` - Per-vertex shader index
- `TEXTURE_STAGE` - Texture stage data
  - `TEXTURE_IDS` - Per-vertex texture index
  - `STAGE_TEXCOORDS` - UV coordinates for this stage

## Collision Tree

`AABTREE` (0x00000090) - Container

Axis-Aligned Bounding Box tree for collision detection:

```cpp
struct AABTreeHeader {
  uint32_t nodeCount;
  uint32_t polyCount;
};

struct AABTreeNode {
  Vector3 min;
  Vector3 max;
  uint32_t frontOrPoly0;    // Front child or first polygon
  uint32_t backOrPolyCount; // Back child or polygon count
};
```

## Mesh Unrolling

For modern rendering, W3D meshes often need "unrolling":

```cpp
std::vector<Vertex> unrollMesh(const Mesh& mesh) {
  std::vector<Vertex> result;

  for (size_t ti = 0; ti < mesh.triangles.size(); ti++) {
    const auto& tri = mesh.triangles[ti];

    for (int i = 0; i < 3; i++) {
      uint32_t vi = tri.vertexIndices[i];

      Vertex v;
      v.position = mesh.vertices[vi];
      v.normal = mesh.normals[vi];

      // UV may be per-face indexed
      if (!mesh.perFaceTexCoordIds.empty()) {
        v.texCoord = mesh.texCoords[mesh.perFaceTexCoordIds[ti * 3 + i]];
      } else {
        v.texCoord = mesh.texCoords[vi];
      }

      result.push_back(v);
    }
  }

  return result;
}
```
