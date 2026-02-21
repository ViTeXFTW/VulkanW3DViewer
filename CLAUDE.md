# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> **Full documentation:** [vitexftw.github.io/VulkanW3DViewer](https://vitexftw.github.io/VulkanW3DViewer/)

## Build Commands

### Getting Help

For detailed usage information, run:
```bash
# PowerShell
.\scripts\rebuild.ps1 -Help

# Bash
./scripts/rebuild.sh --help
```

### Quick Build (using helper scripts)

**Recommended:** Use the rebuild scripts for hassle-free builds with automatic environment setup.

```bash
# Windows (PowerShell) - automatically sets up MSVC environment
.\scripts\rebuild.ps1 debug                    # Auto-detect compiler
.\scripts\rebuild.ps1 release -Compiler msvc   # MSVC (recommended on Windows)
.\scripts\rebuild.ps1 debug -D                 # Clean build
.\scripts\rebuild.ps1 release -Compiler msvc -R # Build and run
.\scripts\rebuild.ps1 debug -Compiler clang    # Use Clang explicitly

# Linux/Mac (Bash)
./scripts/rebuild.sh debug                     # Auto-detect compiler
./scripts/rebuild.sh release -c gcc            # Use GCC
./scripts/rebuild.sh debug -c clang -d         # Clean build with Clang
./scripts/rebuild.sh test                      # Build and run tests
```

**Features:**
- 🚀 Simple syntax: `<config>` + optional `-Compiler <name>`
- 🔧 Automatic MSVC environment setup (no VS Developer Command Prompt needed)
- 🧹 `-D` flag for clean rebuilds
- ▶️  `-R` flag to run after building (PowerShell only)
- ❓ Built-in help: Run `.\scripts\rebuild.ps1 -Help` or `./scripts/rebuild.sh --help`

### Manual CMake Commands

```bash
# Configure (first time or after CMakeLists.txt changes)
cmake --preset debug          # Auto-detect compiler
cmake --preset msvc-debug     # MSVC (Windows, Visual Studio generator)
cmake --preset clang-release  # Clang (Ninja generator)
cmake --preset gcc-debug      # GCC (Ninja generator)
cmake --preset test           # Test build

# Build
cmake --build --preset debug
cmake --build --preset msvc-release
cmake --build --preset clang-debug

# Run (after building)
./build/debug/VulkanW3DViewer.exe          # Windows debug
./build/msvc-release/VulkanW3DViewer.exe   # Windows MSVC release
ctest --preset test                        # Run tests
```

### Available Presets

| Preset | Compiler | Generator | Description |
|--------|----------|-----------|-------------|
| `debug` | Auto-detect | Ninja | Debug build (auto-detect compiler) |
| `release` | Auto-detect | Ninja | Release build (auto-detect compiler) |
| `test` | Auto-detect | Ninja | Debug build with tests |
| `clang-debug` | Clang | Ninja | Debug build with Clang |
| `clang-release` | Clang | Ninja | Release build with Clang |
| `gcc-debug` | GCC | Ninja | Debug build with GCC |
| `gcc-release` | GCC | Ninja | Release build with GCC |
| `msvc-debug` | MSVC | VS 2026 | Debug build with MSVC |
| `msvc-release` | MSVC | VS 2026 | Release build with MSVC |

## Project Overview

This is a **W3D format renderer and map scene viewer** -- a modern Vulkan-based tool for loading and rendering W3D 3D model files and `.map` scene files from Command & Conquer Generals: Zero Hour. The goal is to produce a community rendering pipeline that can faithfully display terrain, water, placed objects, and lighting from the original game's map format, with the architecture designed to support future WorldBuilder-style editing.

The project uses C++20, GLFW for windowing, Vulkan-Hpp for C++ Vulkan bindings, and GLM for math.

**W3D Format:** Westwood 3D format (chunk-based) containing meshes, hierarchies, skeletal animations, and HLod (hierarchical LOD). Reference implementation is in `lib/GeneralsGameCode/`.

**Map Format:** DataChunk binary format (`.map` files) containing heightmap terrain, texture blending, placed objects, water areas, polygon triggers, and global lighting. Distinct from W3D chunks -- uses named chunks with a `CkMp` TOC header. See AGENTS.md for full format specification.

### Requirements

- **Vulkan 1.3+** GPU (uses dynamic rendering, no VkRenderPass)
- Vulkan SDK installed system-wide (`C:/VulkanSDK/`)
- GLFW, Vulkan-Hpp, ImGui, GLM, CLI11, and GoogleTest are git submodules in `lib/`

## Architecture

- Use RAII where ever applicable to keep track of resource lifetime.

### Code Structure

```
src/
  main.cpp                    # Application entry, CLI argument parsing (CLI11)
  core/                       # Application orchestration (viewer-specific)
    application.hpp/cpp       # Main application class
    renderer.hpp/cpp          # Rendering orchestration
    render_state.hpp          # Centralized render state
    shader_loader.hpp         # Shader loading utilities
    settings.hpp/cpp          # Application settings
    app_paths.hpp/cpp         # Application path utilities
  lib/                        # Reusable library components (future w3d_lib static library)
    formats/
      w3d/                    # W3D binary format parsing
        w3d.hpp               # W3D module main header
        types.hpp             # W3D data structures (Mesh, Hierarchy, Animation, etc.)
        chunk_types.hpp       # W3D chunk type enumerations
        chunk_reader.hpp      # Binary chunk parsing utilities
        loader.hpp/cpp        # W3D file loading interface
        model_loader.hpp/cpp  # High-level model interface
        mesh_parser.hpp/cpp   # Mesh chunk parsing
        hierarchy_parser.hpp/cpp # Skeleton/bone parsing
        animation_parser.hpp/cpp # Animation keyframe parsing
        hlod_parser.hpp/cpp   # Hierarchical LOD parsing
        hlod_model.hpp/cpp    # HLod model assembly with LOD switching
      map/                    # Map file parsing (DataChunk format) [PLANNED]
        data_chunk_reader.hpp/cpp  # DataChunk binary reader (CkMp TOC + named chunks)
        map_loader.hpp/cpp    # Top-level .map file loader
        heightmap_parser.hpp/cpp   # HeightMapData chunk (uint8 grid)
        blend_tile_parser.hpp/cpp  # BlendTileData chunk (textures, blending, cliffs)
        objects_parser.hpp/cpp     # ObjectsList/Object sub-chunks
        triggers_parser.hpp/cpp    # PolygonTriggers (water areas, rivers)
        lighting_parser.hpp/cpp    # GlobalLighting (4 time-of-day sets)
        types.hpp             # MapFile, HeightMap, BlendTileData, MapObject, etc.
      big/                    # BIG archive support
        big_archive_manager.hpp/cpp  # BIG file extraction
        asset_registry.hpp/cpp       # Asset name indexing
      ini/                    # SAGE INI dialect parsing [PLANNED]
        ini_parser.hpp/cpp    # Block-based INI parser
        terrain_types.hpp/cpp # TerrainType definitions (name -> TGA)
        water_settings.hpp/cpp # Water rendering configuration
    gfx/                      # Graphics foundation
      vulkan_context.hpp/cpp  # Device, swapchain, queues, depth buffer
      buffer.hpp/cpp          # GPU buffer management with staging
      pipeline.hpp/cpp        # Graphics pipeline, descriptors
      texture.hpp/cpp         # Texture loading and management
      camera.hpp/cpp          # Orbital camera with mouse controls
      bounding_box.hpp        # AABB utilities
      renderable.hpp          # Base renderable interface
    scene/                    # Scene management
      scene.hpp/cpp           # Scene container
  render/                     # Rendering utilities
    animation_player.hpp/cpp  # Animation playback control
    bone_buffer.hpp/cpp       # GPU buffer for bone transformations
    hover_detector.hpp/cpp    # Mesh picking via raycast
    material.hpp              # Material definitions and GPU format
    mesh_converter.hpp/cpp    # W3D mesh to GPU vertex conversion
    raycast.hpp/cpp           # Ray intersection utilities
    renderable_mesh.hpp/cpp   # GPU mesh representation
    skeleton.hpp/cpp          # Skeleton pose computation
    skeleton_renderer.hpp/cpp # Skeleton debug visualization
    terrain/                  # Terrain rendering [PLANNED]
      terrain_mesh.hpp/cpp    # Heightmap -> triangle mesh (32x32 chunks)
      terrain_atlas.hpp/cpp   # Texture atlas builder from tile TGAs
      terrain_blend.hpp/cpp   # Alpha blend pattern generation
      terrain_renderable.hpp/cpp # IRenderable for terrain
    water/                    # Water rendering [PLANNED]
      water_mesh.hpp/cpp      # Polygon trigger -> water mesh
      water_renderable.hpp/cpp # IRenderable for water
  ui/                         # User interface
    imgui_backend.hpp/cpp     # ImGui Vulkan integration
    ui_manager.hpp/cpp        # UI component lifecycle management
    ui_context.hpp            # Shared UI context
    ui_window.hpp             # Window base class
    ui_panel.hpp              # Panel base class
    console_window.hpp/cpp    # Debug console UI
    file_browser.hpp/cpp      # File browser for loading W3D files
    model_browser.hpp/cpp     # BIG archive model browser
    viewport_window.hpp/cpp   # 3D viewport
    settings_window.hpp/cpp   # Settings dialog
    hover_tooltip.hpp/cpp     # Tooltip display
    panels/                   # UI panels
      animation_panel.hpp/cpp # Animation controls
      camera_panel.hpp/cpp    # Camera settings
      display_panel.hpp/cpp   # Display options
      lod_panel.hpp/cpp       # LOD selection
      model_info_panel.hpp/cpp# Model information
      mesh_visibility_panel.hpp/cpp # Mesh visibility
shaders/
  basic.vert/frag             # Shader with texture and material support
  skinned.vert                # Skeletal animation vertex shader
  skeleton.vert/frag          # Skeleton visualization
  terrain.vert/frag           # Terrain: base tile + blend + lighting [PLANNED]
  water.vert/frag             # Water: scrolling UV, transparency [PLANNED]
```

## Implementation Phases

### W3D Model Viewer (Complete)

| Phase | Status | Description |
|-------|--------|-------------|
| 1 | Done | Vulkan foundation - device, swapchain, pipeline, cube rendering |
| 2 | Done | W3D file parsing - chunk reader, mesh/hierarchy/animation structs |
| 3 | Done | Static mesh rendering - GPU upload, viewer controls (no textures yet) |
| 4 | Done | Hierarchy/pose - bone matrices, rest pose display |
| 5 | Done | HLod assembly - model assembly, LOD switching |
| 6 | Done | Materials - texture manager, material push constants, shader support |
| 7 | Done | Animations - load animation, and apply to bones |
| 8 | Done | Render animations onto meshes |

### Terrain & Map Scene Rendering (In Progress)

Goal: Load `.map` files and render complete C&C Generals: Zero Hour scenes (terrain, water, objects, lighting). Architecture designed for future WorldBuilder-style editing (mutable data structures from day one). Output is a `w3d_lib` static library + the viewer application consuming it.

| Phase | Status | Description |
|-------|--------|-------------|
| 0 | Pending | Architecture refactoring -- extract `w3d_lib` static library, integrate VMA, add dynamic buffers, mipmap generation, texture arrays, pipeline refactor, RTS camera |
| 1 | Pending | Map file parsing -- DataChunk reader, HeightMapData, BlendTileData, ObjectsList, PolygonTriggers, GlobalLighting, WorldInfo, SidesList |
| 2 | Pending | INI parsing -- SAGE INI dialect parser, TerrainType definitions, Water settings |
| 3 | Pending | Terrain rendering -- heightmap mesh (32x32 chunks), texture atlas, blend system, cliff UVs, terrain shaders, frustum culling |
| 4 | Pending | Water rendering -- polygon trigger meshes, scrolling UV shader, shoreline blending |
| 5 | Pending | Object placement & scene graph -- scene nodes with transforms, object template resolution, instanced rendering, roads/bridges |
| 6 | Pending | Lighting & polish -- time-of-day lighting, shadow color, cloud shadows, minimap |
| 7 | Pending | Map viewer UI -- map browser, map info panel, object list, time-of-day selector, layer toggles, mode switching |

## Code Style

- 2-space indentation for C/C++ files
- Compiler warnings treated as errors (`-Werror` on GCC/Clang, `/W4` on MSVC)
- Supports multiple compilers: MSVC, Clang, GCC, Intel
- Namespace: `w3d::`
- Modern C++20 patterns (structured bindings, designated initializers)

## Testing

- New features should follow a Test Driven Development (TDD) style where the expected behavior of key functions are created and added to the relevant test suite before implementing the feature.
- Tests should be structured similar to the `src/` directory to easily find the relevant tests for the associated file.

## Key Reference Files

### W3D Model Format
- **W3D format spec:** `lib/GeneralsGameCode/` (original SAGE engine source)
- **Vulkan-Hpp samples:** `lib/Vulkan-Hpp/RAII_Samples/`

### Map/Terrain Format (in `lib/GeneralsGameCode/`)
- **DataChunk reader/writer:** `Generals/Code/GameEngine/.../DataChunk.h/cpp` -- binary framing format
- **Heightmap parser:** `Core/GameEngineDevice/.../WorldHeightMap.h/cpp` -- `ParseHeightMapData()`, `ParseBlendTileData()`, `ParseLightingData()` (2535 lines)
- **Tile data:** `Core/GameEngineDevice/.../TileData.h/cpp` -- 64x64 pixel tile storage
- **Terrain renderer:** `Core/GameEngineDevice/.../HeightMap.h/cpp` -- full 3D heightmap rendering
- **Terrain textures:** `Core/GameEngineDevice/.../TerrainTex.h/cpp` -- runtime atlas generation
- **Water rendering:** `Core/GameEngineDevice/.../W3DWater.h` + `Water/W3DWater.cpp`
- **Map objects:** `Core/GameEngine/.../MapObject.h` -- `MAP_XY_FACTOR`, `MAP_HEIGHT_SCALE`
- **Polygon triggers:** `Generals/Code/.../PolygonTrigger.h/cpp` -- water areas, rivers
- **Version constants:** `GeneralsMD/Code/.../MapReaderWriterInfo.h` -- all chunk versions
- **Map writer:** `GeneralsMD/Code/Tools/WorldBuilder/src/WHeightMapEdit.cpp` -- `saveToFile()`
- **Terrain types:** `GeneralsMD/Code/.../TerrainTypes.h/cpp` -- INI terrain definitions
- **Water INI:** `GeneralsMD/Code/.../INIWater.cpp` -- water settings parsing

### Architecture Notes
- Map files use **DataChunk** format (`CkMp` magic), completely separate from W3D chunk format
- DataChunk uses named chunks (string -> ID via TOC), W3D uses numbered chunks (uint32 type IDs)
- Both formats are binary, little-endian, chunk-based, but share no code
- The DataChunk TOC ID space is shared between chunk names AND Dict key names
- Dict values support 5 types: BOOL (1 byte), INT (4), REAL (4), ASCIISTRING (uint16 len + chars), UNICODESTRING (uint16 charLen + charLen*2 bytes)
- See AGENTS.md for full map format specification and terrain rendering pipeline details
