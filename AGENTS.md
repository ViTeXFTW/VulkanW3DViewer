# AGENTS.md - VulkanW3DViewer

This file guides AI coding agents working on this Vulkan-based W3D renderer and map scene viewer for C&C Generals: Zero Hour.

## Build Commands

```bash
# Quick Build (using helper scripts - recommended)
# PowerShell
.\scripts\rebuild.ps1 debug                    # Auto-detect compiler
.\scripts\rebuild.ps1 release -Compiler msvc   # MSVC (Windows)
.\scripts\rebuild.ps1 debug -D -R              # Clean debug build and run

# Bash
./scripts/rebuild.sh debug                     # Auto-detect compiler
./scripts/rebuild.sh release -c gcc            # GCC
./scripts/rebuild.sh debug -c clang -d         # Clean build with Clang

# Configure (CMake presets)
cmake --preset debug          # Auto-detect compiler
cmake --preset msvc-debug     # MSVC (Windows)
cmake --preset clang-release  # Clang
cmake --preset gcc-debug      # GCC
cmake --preset test           # Debug build with tests enabled (BUILD_TESTING=ON)

# Build
cmake --build --preset debug
cmake --build --preset msvc-release
cmake --build --preset clang-debug

# Run main application
./build/debug/VulkanW3DViewer.exe
./build/msvc-release/VulkanW3DViewer.exe
./build/clang-release/VulkanW3DViewer

# Run all tests
ctest --preset test

# Run specific test suite
./build/test/w3d_tests
./build/test/mesh_converter_tests
./build/test/skeleton_tests
./build/test/texture_tests
./build/test/bounding_box_tests
./build/test/raycast_tests
./build/test/hlod_hover_tests

# Run single test (GoogleTest filter)
./build/test/w3d_tests --gtest_filter=ChunkReaderTest.ReadUint8
```

## Code Style

### Formatting
- **Indentation**: 2 spaces (no tabs)
- **Column limit**: 100 characters
- **Brace style**: Attach (`} else {`)
- **Header guards**: `#pragma once` (not include guards)
- **Tool**: Run `clang-format` before committing (configured in `.clang-format`)

### Naming Conventions
- **Classes**: PascalCase (`ChunkReader`, `VulkanContext`, `Material`)
- **Functions**: camelCase (`readBytes`, `loadFromMemory`, `create`)
- **Member variables**: snake_case with trailing underscore (`data_`, `pos_`, `buffer_`)
- **Local variables**: snake_case (`result`, `index`, `count`)
- **Constants**: UPPER_CASE for compile-time (`MAX_FRAMES`, `W3D_DEBUG`)
- **Enums**: PascalCase for enum class, UPPER_CASE values (`BlendMode::Opaque`)
- **Structs**: PascalCase (`ChunkHeader`, `Vector3`, `MaterialInfo`)
- **Namespaces**: lowercase (`w3d`, `MaterialFlags`)

### Types
- Use `std::optional<T>` for fallible functions (return `std::nullopt` on failure)
- Use `std::span<const uint8_t>` for read-only byte buffers
- Use `uint32_t`, `uint8_t`, etc. from `<cstdint>` for fixed-width types
- Prefer `std::vector<T>` over raw arrays
- Use `std::filesystem::path` for file paths
- GLM types for math: `glm::vec3`, `glm::mat4`, etc.

### Imports/Includes
Regrouped includes in this priority order:
1. Main module headers (same directory, priority 1)
2. `<vulkan/...>` (priority 2 - MUST come before GLFW)
3. `<GLFW/...>` (priority 3)
4. `<glm/...>` (priority 4)
5. Standard library: `<string>`, `<vector>`, etc. (priority 5)
6. Other project headers (priority 6)

Example:
```cpp
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
#include "chunk_reader.hpp"
```

### Error Handling
- Use `std::optional<T>` for recoverable errors (file loading, parsing)
- Provide optional `std::string *outError` parameter for error messages
- Throw custom exceptions (e.g., `ParseError`) for logic errors
- Always validate bounds before array/vector access
- Check Vulkan result codes with `.result` from RAII wrappers

### Memory Management
- **RAII pattern** is mandatory for all resources (Vulkan objects, buffers, etc.)
- Classes manage their own resources in destructor
- Delete copy constructor/assignment for resource-owning types
- Implement move constructor/assignment for transfers
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) where appropriate
- No `new`/`delete` without ownership semantics

### C++20 Patterns
- Structured bindings: `auto [x, y, z] = ...`
- Designated initializers: `Vector3 v{.x = 1.0f, .y = 2.0f, .z = 3.0f}`
- `auto` for iterator types when type is obvious
- `const` correctness - mark methods `const` if they don't modify state
- `noexcept` on move constructors/assignment
- `[[nodiscard]]` on functions with meaningful return values

### Vulkan-Specific
- Use Vulkan-Hpp RAII wrappers (`vk::` namespace)
- Vulkan objects must be destroyed before context cleanup
- Use command buffers from pool for single-time operations
- Prefer dynamic rendering over VkRenderPass
- Validation layers enabled in debug builds only (`W3D_DEBUG` macro)

### Testing
- **TDD approach**: Write tests before implementing features
- Test files mirror source structure (`tests/w3d/` for `src/w3d/`)
- Use Google Test framework (`TEST_F`, `EXPECT_EQ`, etc.)
- Test executables are standalone with minimal dependencies
- Tests use fixtures in `tests/resources/`

### Comments
- **No comments** in code unless specifically requested
- Exception: Critical file format notes in W3D parser headers

## Map File Format (.map)

Map files use the **DataChunk** binary format (distinct from W3D chunks). The framing is:

### DataChunk Container
- **Magic**: `CkMp` (bytes `0x43 0x6B 0x4D 0x70`)
- **TOC**: `int32 count`, then `count` entries of `{uint8 nameLen, char[nameLen] name, uint32 id}`
- **Chunk header**: `uint32 chunkID` (from TOC) + `uint16 version` + `int32 dataSize` = 10 bytes
- Chunks can be **nested** (parent `dataSize` includes child headers + payloads)
- Primitives: `readInt()` = LE int32, `readReal()` = LE float32, `readByte()` = int8, `readAsciiString()` = uint16 len + chars, `readDict()` = key-value pairs

### Top-Level Chunks (in file order)
| Chunk Name | Latest Version | Description |
|---|---|---|
| `"HeightMapData"` | 4 | Raw `uint8` heightmap grid (10.0 world units/cell, 0.625 height scale) |
| `"BlendTileData"` | 8 (ZH) | Tile textures, blend info, cliff UV overrides, passability |
| `"WorldInfo"` | 1 | World Dict (weather, etc.) |
| `"SidesList"` | 3 | Players, teams, build lists, nested `"PlayerScriptsList"` |
| `"ObjectsList"` | 3 | Nested `"Object"` sub-chunks: position, rotation, template name, property Dict |
| `"PolygonTriggers"` | 4 (ZH) | Water areas, rivers, trigger polygons |
| `"GlobalLighting"` | 3 | 4 time-of-day lighting sets (ambient/diffuse/direction, 3 lights each) |

### Key Constants
| Constant | Value | Usage |
|---|---|---|
| `MAP_XY_FACTOR` | `10.0f` | World units per heightmap cell |
| `MAP_HEIGHT_SCALE` | `0.625f` | `MAP_XY_FACTOR / 16.0` -- height byte to world units |
| `TILE_PIXEL_EXTENT` | `64` | Source tile bitmap size (64x64, BGRA) |
| `FLAG_VAL` | `0x7ADA0000` | Blend tile sentinel/validation marker |
| `INVERTED_MASK` | `0x1` | Blend tile `inverted` field bit 0 |
| `FLIPPED_MASK` | `0x2` | Blend tile `inverted` field bit 1 (triangle flip) |

### Terrain Texture Pipeline
1. `BlendTileData` texture class names (e.g., `"TEDesert1"`) resolve via INI `TerrainType` definitions
2. Each terrain type references a TGA in `Art/Terrain/` within `TerrainZH.big`
3. TGAs are split into 64x64 tiles, arranged into a 2048-wide runtime texture atlas
4. `tileNdxes[cell]`: top 14 bits = source tile index, bottom 2 bits = 32x32 quadrant
5. Blending uses 12 alpha gradient patterns (6 directions x inverted) for smooth terrain transitions
6. Cliff cells (`maxZ - minZ > 9.8`) use custom UV coordinates from `cliffInfo[]`

### Terrain Rendering Passes (original engine)
1. Base terrain texture from atlas
2. Alpha blend overlay (terrain transitions)
3. Extra blend (3-way texture blends)
4. Cloud shadow layer (scrolling animated texture)
5. Macro/noise texture
6. Shoreline alpha blending (water-terrain edge)
7. Scorch marks, roads, trees, props, bridges, shroud

### Water System
- Water surfaces defined by `PolygonTrigger` with `isWaterArea = true`
- Flat plane at polygon Z height, scrolling UV texture, semi-transparent
- Rivers use `isRiver = true` with `riverStart` vertex index for flow direction
- Settings from `Water.ini`: textures, transparency, scroll rates, sky textures
- Original had 4 types: translucent, FB reflection, PV shader, grid mesh (deformable)

## Project Architecture

### Library Extraction (Planned)
The project is being restructured into:
- **`w3d_lib`** -- static library (`src/lib/` + `src/render/`): all parsing, rendering, scene management
- **`VulkanW3DViewer`** -- thin executable (`src/main.cpp` + `src/core/` + `src/ui/`): application shell

### New Directory Structure (Terrain/Map)
```
src/lib/formats/map/          -- Map file parsing (DataChunk format)
  data_chunk_reader.hpp/cpp   -- DataChunk binary format reader
  map_loader.hpp/cpp          -- Top-level .map file loader
  heightmap_parser.hpp/cpp    -- HeightMapData chunk
  blend_tile_parser.hpp/cpp   -- BlendTileData chunk
  objects_parser.hpp/cpp      -- ObjectsList/Object chunks
  triggers_parser.hpp/cpp     -- PolygonTriggers chunk
  lighting_parser.hpp/cpp     -- GlobalLighting chunk
  types.hpp                   -- MapFile, HeightMap, BlendTileData, MapObject, etc.

src/lib/formats/ini/          -- SAGE INI dialect parsing
  ini_parser.hpp/cpp          -- Block-based INI parser
  terrain_types.hpp/cpp       -- TerrainType definitions (name -> TGA)
  water_settings.hpp/cpp      -- Water rendering configuration

src/render/terrain/           -- Terrain rendering
  terrain_mesh.hpp/cpp        -- Heightmap -> triangle mesh (32x32 chunks)
  terrain_atlas.hpp/cpp       -- Texture atlas builder from tile TGAs
  terrain_blend.hpp/cpp       -- Alpha blend pattern generation
  terrain_renderable.hpp/cpp  -- IRenderable implementation for terrain

src/render/water/             -- Water rendering
  water_mesh.hpp/cpp          -- Polygon trigger -> water mesh
  water_renderable.hpp/cpp    -- IRenderable implementation for water

shaders/
  terrain.vert/frag           -- Terrain: base tile + blend + lighting
  water.vert/frag             -- Water: scrolling UV, transparency
```

### Key Infrastructure Requirements
- **VMA**: Vulkan Memory Allocator required for terrain (many small chunk buffers exceed per-device allocation limits)
- **Dynamic buffers**: Terrain mesh needs updateable vertex/index buffers (for future editing support)
- **Texture arrays**: Terrain splatmapping needs `VkImage` with `arrayLayers > 1`
- **Mipmaps**: Required for terrain textures at oblique viewing angles
- **RTS camera**: WASD pan, scroll zoom, Q/E rotation, ~60-degree pitch

## Reference Source Code

The `lib/GeneralsGameCode/` submodule contains the original SAGE engine source as reference:

| File | Contains |
|---|---|
| `Core/.../WorldHeightMap.h/cpp` | Heightmap data class + map file parser (2535 lines) |
| `Core/.../TileData.h/cpp` | Tile bitmap storage (64x64 px per tile) |
| `Core/.../BaseHeightMap.h` | Base terrain render object |
| `Core/.../HeightMap.h/cpp` | Full 3D heightmap renderer |
| `Core/.../TerrainTex.h/cpp` | Runtime texture atlas generation |
| `Core/.../W3DWater.h` + `Water/W3DWater.cpp` | Water rendering (291-line header) |
| `Generals/Code/.../DataChunk.h/cpp` | DataChunk reader/writer implementation |
| `GeneralsMD/Code/.../MapReaderWriterInfo.h` | All chunk version constants |
| `Core/.../MapObject.h` | MapObject struct, `MAP_XY_FACTOR`, `MAP_HEIGHT_SCALE` |
| `Generals/Code/.../PolygonTrigger.h/cpp` | Water area/river polygon parsing |
| `GeneralsMD/Code/Tools/WorldBuilder/src/WHeightMapEdit.cpp` | Map file writer (saveToFile) |
