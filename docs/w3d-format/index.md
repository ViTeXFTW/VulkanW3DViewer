# W3D Format

Technical documentation for the Westwood 3D (W3D) file format.

## Overview

W3D is a proprietary 3D model format developed by Westwood Studios, used in games like:

- **Command & Conquer: Generals** (2003)
- **Command & Conquer: Generals - Zero Hour** (2003)
- **Earth & Beyond** (2002)
- **Command & Conquer: Renegade** (2002)

## Format Characteristics

- **Binary format**: Little-endian byte order
- **Chunk-based**: Hierarchical chunk structure
- **Self-contained**: Geometry, materials, animations in one file
- **Extensible**: Unknown chunks can be skipped

## File Structure

```
┌─────────────────────────────────────┐
│ Chunk Header (8 bytes)              │
│   - Type (4 bytes)                  │
│   - Size (4 bytes)                  │
├─────────────────────────────────────┤
│ Chunk Data                          │
│   - May contain sub-chunks          │
├─────────────────────────────────────┤
│ Next Chunk Header                   │
├─────────────────────────────────────┤
│ ...                                 │
└─────────────────────────────────────┘
```

## Main Components

<div class="grid cards" markdown>

-   :material-cube-outline:{ .lg .middle } **Meshes**

    ---

    Geometry with vertices, normals, UVs, and materials

    [:octicons-arrow-right-24: Meshes](meshes.md)

-   :material-bone:{ .lg .middle } **Hierarchies**

    ---

    Skeleton bone structures for animation

    [:octicons-arrow-right-24: Hierarchies](hierarchies.md)

-   :material-movie:{ .lg .middle } **Animations**

    ---

    Keyframe animation data

    [:octicons-arrow-right-24: Animations](animations.md)

-   :material-layers:{ .lg .middle } **HLod**

    ---

    Level of Detail configuration

    [:octicons-arrow-right-24: HLod](hlod.md)

</div>

## Sections

- [Format Overview](overview.md) - Detailed format structure
- [Chunk Types](chunk-types.md) - Complete chunk type reference
- [Meshes](meshes.md) - Mesh data structures
- [Hierarchies](hierarchies.md) - Skeleton structures
- [Animations](animations.md) - Animation formats
- [HLod](hlod.md) - Level of detail system

## Quick Reference

### Container Bit

The high bit of the chunk size indicates a container:

```cpp
constexpr uint32_t CONTAINER_BIT = 0x80000000;

bool isContainer = (size & CONTAINER_BIT) != 0;
uint32_t dataSize = size & ~CONTAINER_BIT;
```

### Common Chunk Types

| Chunk | ID | Description |
|-------|------|-------------|
| MESH | 0x00000000 | Mesh container |
| HIERARCHY | 0x00000100 | Hierarchy container |
| ANIMATION | 0x00000200 | Animation container |
| HLOD | 0x00000700 | HLod container |

### Name Format

Names are fixed-length, null-terminated:

```cpp
constexpr uint32_t W3D_NAME_LEN = 16;  // Max name length
```

## Format Quirks

!!! warning "Important Implementation Notes"

    1. **UV V-Flip**: V coordinates are inverted compared to Vulkan
       ```cpp
       v = 1.0f - v;  // Flip V coordinate
       ```

    2. **Quaternion Order**: W3D stores (x,y,z,w), GLM expects (w,x,y,z)
       ```cpp
       glm::quat q(w3d.w, w3d.x, w3d.y, w3d.z);
       ```

    3. **Root Bone**: Parent index `0xFFFFFFFF` indicates root
       ```cpp
       if (pivot.parentIndex == 0xFFFFFFFF) {
         // This is a root bone
       }
       ```

    4. **Per-Face UVs**: UV indices are per-face, not per-vertex
       - Requires "unrolling" meshes for modern rendering

## Reference Implementation

The original W3D code can be found in:

```
legacy/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/
```

Key files:

| File | Description |
|------|-------------|
| `w3d_file.h` | Format specification |
| `meshmdlio.cpp` | Mesh loading |
| `hlod.cpp` | HLod handling |
| `hanim.cpp` | Animation handling |

## Versioning

W3D files include version numbers for compatibility:

```cpp
// Mesh version
constexpr uint32_t W3D_CURRENT_MESH_VERSION = 0x00040002;  // 4.2

// Version helpers
uint16_t major = (version >> 16) & 0xFFFF;
uint16_t minor = version & 0xFFFF;
```
