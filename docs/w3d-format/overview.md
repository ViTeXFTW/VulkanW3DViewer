# Format Overview

Detailed technical overview of the W3D file format.

## Binary Structure

W3D files are binary, little-endian, with a chunk-based structure.

### Chunk Header

Every chunk starts with an 8-byte header:

```cpp
struct ChunkHeader {
  uint32_t type;   // Chunk type identifier
  uint32_t size;   // Data size (with container bit)
};
```

### Container Bit

The high bit of `size` indicates whether the chunk contains sub-chunks:

```cpp
constexpr uint32_t CONTAINER_BIT = 0x80000000;

// Parse chunk header
uint32_t type = read_u32();
uint32_t rawSize = read_u32();

bool isContainer = (rawSize & CONTAINER_BIT) != 0;
uint32_t dataSize = rawSize & ~CONTAINER_BIT;
```

**Container chunks** (bit set):

- Contain only other chunks
- No direct data
- Examples: MESH, HIERARCHY, ANIMATION

**Data chunks** (bit clear):

- Contain raw data
- No sub-chunks
- Examples: VERTICES, TRIANGLES, PIVOTS

### Reading Pattern

```cpp
void parseChunks(span<byte> data) {
  ChunkReader reader(data);

  while (reader.hasMore()) {
    ChunkHeader header = reader.readHeader();

    if (isContainer(header)) {
      // Recurse into sub-chunks
      auto subData = reader.getChunkData(header);
      parseChunks(subData);
    } else {
      // Process data chunk
      processChunk(header.type, reader.getChunkData(header));
    }
  }
}
```

## Data Types

### Primitive Types

| Type | Size | Description |
|------|------|-------------|
| `uint8` | 1 | Unsigned byte |
| `uint16` | 2 | Unsigned short |
| `uint32` | 4 | Unsigned int |
| `int32` | 4 | Signed int |
| `float` | 4 | IEEE 754 float |

### Vector Types

```cpp
struct Vector2 {
  float u, v;
};

struct Vector3 {
  float x, y, z;
};

struct Quaternion {
  float x, y, z, w;  // Note: W3D order, not GLM order
};
```

### Color Types

```cpp
struct RGB {
  uint8_t r, g, b;
};

struct RGBA {
  uint8_t r, g, b, a;
};
```

### String Type

Fixed-length, null-terminated:

```cpp
constexpr uint32_t W3D_NAME_LEN = 16;

// Reading a name
char name[W3D_NAME_LEN];
reader.read(name, W3D_NAME_LEN);
// Ensure null termination
name[W3D_NAME_LEN - 1] = '\0';
```

## Top-Level Chunks

A W3D file contains one or more top-level chunks:

| Chunk | Type ID | Description |
|-------|---------|-------------|
| MESH | 0x00000000 | 3D mesh geometry |
| HIERARCHY | 0x00000100 | Skeleton/bone structure |
| ANIMATION | 0x00000200 | Uncompressed animation |
| COMPRESSED_ANIMATION | 0x00000280 | Compressed animation |
| HLOD | 0x00000700 | Hierarchical LOD |
| BOX | 0x00000740 | Collision box |
| EMITTER | 0x00000800 | Particle emitter |

## Chunk Hierarchy

### Mesh Structure

```
MESH (container)
├── MESH_HEADER3
├── VERTICES
├── VERTEX_NORMALS
├── TRIANGLES
├── TEXCOORDS (or STAGE_TEXCOORDS)
├── VERTEX_INFLUENCES (if skinned)
├── VERTEX_SHADE_INDICES
├── MATERIAL_INFO
├── VERTEX_MATERIALS (container)
│   ├── VERTEX_MATERIAL
│   │   ├── VERTEX_MATERIAL_NAME
│   │   └── VERTEX_MATERIAL_INFO
│   └── ...
├── SHADERS
├── TEXTURES (container)
│   ├── TEXTURE
│   │   ├── TEXTURE_NAME
│   │   └── TEXTURE_INFO
│   └── ...
├── MATERIAL_PASS (container)
│   ├── VERTEX_MATERIAL_IDS
│   ├── SHADER_IDS
│   └── TEXTURE_STAGE (container)
│       ├── TEXTURE_IDS
│       └── STAGE_TEXCOORDS
└── AABTREE (collision)
```

### Hierarchy Structure

```
HIERARCHY (container)
├── HIERARCHY_HEADER
├── PIVOTS
└── PIVOT_FIXUPS (optional)
```

### Animation Structure

```
ANIMATION (container)
├── ANIMATION_HEADER
├── ANIMATION_CHANNEL (multiple)
└── BIT_CHANNEL (optional, multiple)
```

### HLod Structure

```
HLOD (container)
├── HLOD_HEADER
├── HLOD_LOD_ARRAY (container, multiple)
│   ├── HLOD_SUB_OBJECT_ARRAY_HEADER
│   └── HLOD_SUB_OBJECT (multiple)
├── HLOD_AGGREGATE_ARRAY (optional)
└── HLOD_PROXY_ARRAY (optional)
```

## File Composition

W3D files can contain different combinations:

### Single Mesh

```
MESH
```

Simple static object.

### Skinned Model

```
MESH (with vertex influences)
HIERARCHY
```

Mesh bound to skeleton.

### Animated Model

```
MESH
HIERARCHY
ANIMATION (one or more)
```

Complete animated model.

### Multi-LOD Model

```
MESH (LOD 0)
MESH (LOD 1)
...
HIERARCHY
HLOD
ANIMATION
```

Model with level of detail.

## Coordinate System

W3D uses a right-handed coordinate system:

- **X**: Right
- **Y**: Up
- **Z**: Forward (out of screen)

### UV Coordinates

W3D UV convention differs from Vulkan:

| System | U | V Origin |
|--------|---|----------|
| W3D | Right | Bottom |
| Vulkan | Right | Top |

**Conversion required**:

```cpp
v_vulkan = 1.0f - v_w3d;
```

## Alignment

Chunks are not necessarily aligned. Always read sequentially and track position.

## Version Numbers

Version format: `(major << 16) | minor`

```cpp
uint32_t version = 0x00040002;  // Version 4.2
uint16_t major = version >> 16;  // 4
uint16_t minor = version & 0xFFFF;  // 2
```

## Error Handling

When parsing:

1. **Unknown chunks**: Skip using size field
2. **Missing optional chunks**: Use defaults
3. **Version mismatch**: Warn but attempt to parse
4. **Truncated file**: Stop parsing, use what was read
