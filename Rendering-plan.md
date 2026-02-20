# C&C terrain rendering pipeline investigation

**Session ID:** ses_384b9d9e9ffeiHLte0RRuWnKkn
**Created:** 2/20/2026, 2:38:40 PM
**Updated:** 2/20/2026, 2:52:32 PM

---

## User

I want to create a plan around expanding the application to also be able to render terrain similar to the original game in the aim of creating a new rendering pipeline for the Command & Conquer Community scene. Help me investigate the path to supporting rendering like the original, it should be able to load the same map files which contain all the information regarding how to render a scene. Ask clarifying questions if any

---

## Assistant (Plan · claude-opus-4-6 · 298.5s)



I'll help you plan terrain rendering support for Command & Conquer maps. Let me first explore the current codebase to understand the existing architecture, then research the W3D/map file formats.

**Tool: task**

**Input:**
```json
{
  "description": "Explore codebase architecture",
  "prompt": "Explore the VulkanW3DViewer codebase at C:\\CodeProjects\\VulkanW3DViewer thoroughly. I need to understand:\n\n1. The overall project structure - directory layout, key source files\n2. The current rendering pipeline - how Vulkan is set up, what shaders exist, how meshes are rendered\n3. The W3D file parsing - what chunk types are already supported, what data structures exist\n4. Any existing terrain-related code or references\n5. The asset loading pipeline - how models/textures are loaded and prepared for rendering\n6. The camera/scene management system\n7. Any existing map file support\n\nReturn a comprehensive summary of:\n- Directory structure (top-level and key subdirectories)\n- Key classes and their responsibilities\n- The rendering pipeline flow (from file load to screen)\n- W3D chunk types currently parsed\n- Shader files and what they do\n- Any terrain or map-related code already present\n- The build system structure (CMake targets, dependencies)",
  "subagent_type": "explore"
}
```

**Output:**
```
task_id: ses_384b9b10fffeQV93Kq0LT3mxQa (for resuming to continue this task if needed)

<task_result>
I now have a comprehensive understanding of the entire codebase. Here is the full summary:

---

# VulkanW3DViewer Codebase Analysis

## 1. Directory Structure

```
C:\CodeProjects\VulkanW3DViewer\
|
|-- CMakeLists.txt              # Root build file (v0.3.0, C++20)
|-- CMakePresets.json            # Build presets (debug/release/test x MSVC/Clang/GCC)
|-- AGENTS.md                   # AI agent instructions
|-- CLAUDE.md                   # Alias for AGENTS.md
|-- README.md
|-- LICENSE.md
|-- .clang-format               # Code formatting config
|-- .clangd                     # Clangd LSP config
|-- .editorconfig
|-- mkdocs.yml                  # Documentation site config
|-- compile_commands.json       # Build system generated (symlink)
|
|-- src/                        # Main application source
|   |-- main.cpp                # Entry point (CLI11 argument parsing)
|   |-- core/                   # Application framework
|   |   |-- application.hpp/cpp # Main app class: window, Vulkan init, main loop
|   |   |-- renderer.hpp/cpp    # Frame rendering orchestration
|   |   |-- render_state.hpp    # Centralized rendering toggles/flags
|   |   |-- settings.hpp/cpp    # Persistent JSON settings (window, paths, display)
|   |   |-- shader_loader.hpp   # Loads embedded SPIR-V shader bytecode
|   |   |-- app_paths.hpp/cpp   # Platform-specific app data directories
|   |   |-- debug.hpp           # Debug macros
|   |
|   |-- lib/                    # Reusable library layer (Vulkan-aware)
|   |   |-- gfx/               # Graphics abstractions
|   |   |   |-- vulkan_context.hpp/cpp  # Vulkan instance/device/swapchain/renderpass
|   |   |   |-- pipeline.hpp/cpp        # Graphics pipeline + descriptor management
|   |   |   |-- buffer.hpp/cpp          # Buffer, StagedBuffer, VertexBuffer, IndexBuffer, UniformBuffer
|   |   |   |-- texture.hpp/cpp         # TextureManager (TGA+DDS loading, GPU upload)
|   |   |   |-- camera.hpp/cpp          # Orbit camera (mouse drag + scroll zoom)
|   |   |   |-- bounding_box.hpp        # AABB utility struct
|   |   |   |-- renderable.hpp          # IRenderable interface (abstract draw + bounds)
|   |   |
|   |   |-- formats/            # File format parsers
|   |   |   |-- w3d/            # W3D binary format parsing
|   |   |   |   |-- w3d.hpp               # Umbrella header
|   |   |   |   |-- chunk_types.hpp       # All W3D chunk type IDs + helper enums
|   |   |   |   |-- chunk_reader.hpp      # Binary reader with span-based sub-readers
|   |   |   |   |-- types.hpp             # All W3D data structures (Mesh, Hierarchy, Animation, HLod, Box, etc.)
|   |   |   |   |-- loader.hpp/cpp        # Top-level file loader (dispatches to parsers)
|   |   |   |   |-- mesh_parser.hpp/cpp   # Mesh chunk parsing
|   |   |   |   |-- hierarchy_parser.hpp/cpp  # Skeleton hierarchy parsing
|   |   |   |   |-- animation_parser.hpp/cpp  # Animation + compressed animation parsing
|   |   |   |   |-- hlod_parser.hpp/cpp   # HLod + Box chunk parsing
|   |   |   |   |-- model_loader.hpp/cpp  # High-level: parse W3D -> GPU upload -> configure camera
|   |   |   |   |-- hlod_model.hpp/cpp    # GPU-ready HLod model (LOD, skinning, draw commands)
|   |   |   |
|   |   |   |-- big/            # BIG archive support (C&C Generals)
|   |   |       |-- big_archive_manager.hpp/cpp  # Opens BIG files, extracts to cache
|   |   |       |-- asset_registry.hpp/cpp       # Scans archives, indexes models/textures/INI
|   |   |       |-- ini_extractor.hpp/cpp        # INI file extraction (placeholder for future metadata parsing)
|   |   |
|   |   |-- scene/              # Scene management
|   |       |-- scene.hpp/cpp   # Simple renderable list (add/remove/clear)
|   |
|   |-- render/                 # Rendering-specific code
|   |   |-- mesh_converter.hpp/cpp     # W3D Mesh -> GPU Vertex/Index conversion (with texture splitting)
|   |   |-- renderable_mesh.hpp/cpp    # Simple GPU mesh container (non-HLod fallback)
|   |   |-- skeleton.hpp/cpp           # SkeletonPose: computes rest/animated bone world transforms
|   |   |-- skeleton_renderer.hpp/cpp  # Skeleton debug visualization (lines+spheres, double-buffered)
|   |   |-- bone_buffer.hpp/cpp        # SSBO for bone matrices (GPU skinning, double-buffered)
|   |   |-- animation_player.hpp/cpp   # Animation playback (standard+compressed, Loop/Once/PingPong)
|   |   |-- material.hpp               # Material data structures + blend modes
|   |   |-- raycast.hpp/cpp            # Ray-triangle, ray-sphere, ray-line intersection
|   |   |-- hover_detector.hpp/cpp     # Mouse hover detection (meshes, bones, joints)
|   |
|   |-- ui/                     # ImGui-based user interface
|       |-- imgui_backend.hpp/cpp      # ImGui Vulkan/GLFW integration
|       |-- ui_manager.hpp/cpp         # Window registry + draw orchestration
|       |-- ui_window.hpp              # Base UIWindow class
|       |-- ui_panel.hpp               # Base UIPanel class
|       |-- ui_context.hpp             # Shared state context passed to all UI components
|       |-- viewport_window.hpp/cpp    # 3D viewport (menu bar, dockspace)
|       |-- console_window.hpp/cpp     # Log/message console
|       |-- file_browser.hpp/cpp       # File selection dialog
|       |-- model_browser.hpp/cpp      # BIG archive model browser
|       |-- settings_window.hpp/cpp    # Settings editor UI
|       |-- hover_tooltip.hpp/cpp      # Hover info tooltip overlay
|       |-- panels/
|           |-- model_info_panel.hpp/cpp        # Model metadata display
|           |-- mesh_visibility_panel.hpp/cpp   # Per-mesh show/hide toggles
|           |-- lod_panel.hpp/cpp               # LOD level selector (Auto/Manual)
|           |-- display_panel.hpp/cpp           # Global display toggles (mesh/skeleton)
|           |-- camera_panel.hpp/cpp            # Camera parameters editor
|           |-- animation_panel.hpp/cpp         # Animation playback controls + timeline
|
|-- shaders/                    # GLSL shader sources (compiled to SPIR-V, embedded in binary)
|   |-- basic.vert              # Standard vertex shader (MVP transform)
|   |-- basic.frag              # Fragment shader (lighting, texturing, alpha test, hover tint)
|   |-- skinned.vert            # GPU skinned vertex shader (bone matrix SSBO)
|   |-- skeleton.vert           # Skeleton debug visualization vertex shader
|   |-- skeleton.frag           # Skeleton debug visualization fragment shader (hover tint)
|
|-- tests/                      # Google Test test suites
|   |-- CMakeLists.txt          # Test targets definition
|   |-- w3d/                    # W3D parser tests
|   |   |-- test_chunk_reader.cpp
|   |   |-- test_mesh_parser.cpp
|   |   |-- test_hierarchy_parser.cpp
|   |   |-- test_animation_parser.cpp
|   |   |-- test_hlod_parser.cpp
|   |   |-- test_loader.cpp
|   |-- render/                 # Rendering logic tests
|   |   |-- test_mesh_converter.cpp
|   |   |-- test_skeleton_pose.cpp
|   |   |-- test_animation_player.cpp
|   |   |-- test_bounding_box.cpp
|   |   |-- raycast_test.cpp
|   |   |-- test_hlod_hover.cpp
|   |   |-- test_texture_loading.cpp
|   |   |-- test_mesh_visibility.cpp
|   |-- ui/
|   |   |-- test_file_browser.cpp
|   |-- core/
|   |   |-- test_settings.cpp
|   |   |-- test_app_paths.cpp
|   |-- stubs/                  # Vulkan-free stubs for unit testing
|       |-- lib/gfx/ (pipeline.hpp, buffer.hpp)
|       |-- core/ (render_state.hpp, pipeline.hpp, buffer.hpp)
|
|-- lib/                        # Third-party dependencies (git submodules)
|   |-- Vulkan-Hpp/             # Vulkan C++ headers (includes Vulkan-Headers)
|   |-- glfw/                   # Window/input management
|   |-- glm/                    # Mathematics library
|   |-- imgui/                  # Immediate-mode GUI (with Vulkan+GLFW backends)
|   |-- googletest/             # Testing framework
|   |-- CLI11/                  # Command-line argument parsing
|   |-- json/                   # nlohmann/json (for settings serialization)
|   |-- BigXtractor/            # BIG archive extraction library
|   |-- GeneralsGameCode/       # Reference: original C&C Generals source code
|
|-- cmake/                      # CMake utilities
|   |-- EmbedShaders.cmake      # Embeds compiled SPIR-V into C++ header
|   |-- mingw-w64-toolchain.cmake
|   |-- generate-vulkan-mingw-lib.sh
|
|-- scripts/                    # Build helper scripts
|   |-- rebuild.ps1             # PowerShell build script
|   |-- rebuild.sh              # Bash build script
|
|-- docs/                       # MkDocs documentation
    |-- index.md
    |-- w3d-format/             # W3D file format documentation
    |-- architecture/           # Architecture docs
    |-- development/
    |-- getting-started/
    |-- user-guide/
    |-- api/
```

## 2. Key Classes and Their Responsibilities

### Core Layer (`src/core/`)
| Class | Responsibility |
|-------|---------------|
| `Application` | Top-level orchestrator: creates window, initializes Vulkan, runs main loop, manages all subsystems |
| `Renderer` | Owns pipelines, command buffers, sync objects; records and submits draw commands per frame |
| `RenderState` | Simple struct holding display toggles (`showMesh`, `showSkeleton`, `useHLodModel`, `useSkinnedRendering`) |
| `Settings` | Persistent JSON settings: paths, window size, display defaults. Saved to app data directory |
| `AppPaths` | Platform-specific paths (settings file, app data directory) |

### Graphics Layer (`src/lib/gfx/`)
| Class | Responsibility |
|-------|---------------|
| `VulkanContext` | Full Vulkan lifecycle: instance, device, swapchain, depth buffer, render pass, framebuffers, command pool |
| `Pipeline` | Creates graphics pipelines (standard + skinned variants), descriptor set layouts, push constants |
| `DescriptorManager` | Manages descriptor pools and per-frame/per-texture descriptor sets for the standard pipeline |
| `SkinnedDescriptorManager` | Same as above but with additional bone SSBO binding (binding 2) |
| `Buffer` / `StagedBuffer` / `VertexBuffer<T>` / `IndexBuffer` / `UniformBuffer<T>` | RAII Vulkan buffer abstractions with staging upload support |
| `TextureManager` | Loads TGA/DDS textures from disk or BIG archives, uploads to GPU, maintains name-to-index map |
| `Camera` | Orbit camera: mouse-drag rotation, scroll zoom, target point, produces view matrix |
| `BoundingBox` | Min/max AABB with expand/center/radius utilities |
| `IRenderable` | Abstract interface: `draw()`, `bounds()`, `typeName()`, `isValid()` |

### W3D Parsing Layer (`src/lib/formats/w3d/`)
| Class | Responsibility |
|-------|---------------|
| `ChunkReader` | Binary reader over `std::span<const uint8_t>`: reads primitives, vectors, quaternions, chunk headers; creates sub-readers |
| `Loader` | Top-level: reads file to memory, iterates top-level chunks, dispatches to parsers, returns `W3DFile` |
| `MeshParser` | Parses MESH chunks: header, vertices, normals, UVs, triangles, materials, textures, material passes, AABTree |
| `HierarchyParser` | Parses HIERARCHY chunks: header, pivots (bones with parent/translation/rotation) |
| `AnimationParser` | Parses ANIMATION and COMPRESSED_ANIMATION chunks: channels, bit channels, timecoded/adaptive-delta data |
| `HLodParser` | Parses HLOD and BOX chunks: LOD arrays, sub-objects, aggregates, proxies |
| `ModelLoader` | High-level pipeline: load W3D -> parse -> upload textures -> create GPU meshes -> configure skeleton -> set camera |
| `HLodModel` | GPU-side HLod model: manages LOD levels, draws with textures/hover, supports both static and skinned rendering |

### BIG Archive Layer (`src/lib/formats/big/`)
| Class | Responsibility |
|-------|---------------|
| `BigArchiveManager` | Opens BIG archive files (W3DZH.big, TexturesZH.big, etc.), extracts assets to disk cache |
| `AssetRegistry` | Scans BIG archives, builds searchable index of model names, texture names, INI files |
| `IniExtractor` | Placeholder for INI file extraction/parsing (future feature) |

### Render Layer (`src/render/`)
| Class | Responsibility |
|-------|---------------|
| `MeshConverter` | Converts W3D `Mesh` structs to GPU `Vertex`/`SkinnedVertex` arrays, handles per-triangle texture splitting |
| `RenderableMesh` | Simple GPU mesh container for non-HLod models; holds vertex/index buffers + CPU copies for raycasting |
| `SkeletonPose` | Computes bone world transforms from hierarchy; supports rest pose and animated pose; produces skinning matrices |
| `SkeletonRenderer` | Debug visualization of skeleton as colored lines (bones) and icosphere joints; double-buffered |
| `BoneMatrixBuffer` | SSBO holding bone matrices for GPU skinning; max 256 bones; double-buffered |
| `AnimationPlayer` | Manages animation playback: selection, play/pause/stop, frame scrubbing, loop/once/pingpong modes |
| `Material` | CPU material definition with `toGPU()` conversion; supports diffuse/emissive/specular/alpha/blend modes |
| `Ray` / `TriangleHit` / `LineHit` / `SphereHit` | Raycasting primitives: screen-to-world ray, Moller-Trumbore triangle test, line/sphere tests |
| `HoverDetector` | Uses raycasting to detect mouse hover over meshes (LOD-aware), bones, and joints |

### UI Layer (`src/ui/`)
| Class | Responsibility |
|-------|---------------|
| `ImGuiBackend` | Initializes and manages ImGui with Vulkan/GLFW backends |
| `UIManager` | Registry of `UIWindow` instances; calls `draw()` on all visible windows |
| `UIContext` | Shared context struct passed to all UI: pointers to loaded data, render state, callbacks |
| `ViewportWindow` | Main viewport with menu bar (File/View/Help menus), dockspace, sidebar panels |
| `ConsoleWindow` | Scrolling log window with info/warning/error message types |
| `FileBrowser` | File selection dialog for .w3d files |
| `ModelBrowser` | Searchable list of models from BIG archives |
| `SettingsWindow` | Editor for persistent settings (paths, game directory) |
| `HoverTooltip` | Floating tooltip showing hovered mesh/bone name |
| Panel classes | `ModelInfoPanel`, `MeshVisibilityPanel`, `LodPanel`, `DisplayPanel`, `CameraPanel`, `AnimationPanel` |

## 3. Rendering Pipeline Flow

### Initialization
1. `Application::initWindow()` -- GLFW window creation (no OpenGL API)
2. `Application::initVulkan()`:
   - `VulkanContext::init()` -- Creates Vulkan instance, surface, physical/logical device, swapchain (with depth buffer, render pass, framebuffers), command pool
   - `SkeletonRenderer::create()` -- Creates skeleton debug pipeline (line+point topology)
   - `BoneMatrixBuffer::create()` -- Creates double-buffered SSBOs for bone matrices
   - `TextureManager::init()` -- Creates default pink/white checkerboard texture
   - BIG archive initialization (if game directory configured)
   - `Renderer::init()` -- Creates standard + skinned graphics pipelines, uniform buffers, descriptor managers, command buffers, sync objects
3. `Application::initUI()` -- ImGui initialization, window/panel registration

### Per-Frame Rendering
1. `glfwPollEvents()` -- Input processing
2. `Camera::update()` -- Orbit camera from mouse input
3. `HoverDetector::update()` -- Raycast mouse cursor against meshes/skeleton
4. `AnimationPlayer::update()` -- Advance animation time
5. If animation frame changed:
   - `AnimationPlayer::applyToPose()` -- Update `SkeletonPose` with interpolated transforms
   - `Renderer::waitForCurrentFrame()` -- Wait for GPU fence
   - `SkeletonRenderer::updateFromPose()` -- Rebuild skeleton line/joint geometry
   - `BoneMatrixBuffer::update()` -- Upload new skinning matrices to SSBO
6. `HLodModel::updateLOD()` -- Auto-select LOD level based on camera distance
7. `ImGuiBackend::newFrame()` + `Application::drawUI()` -- Build ImGui draw data
8. `Renderer::drawFrame()`:
   - Wait for fence (if not already waited)
   - Acquire swapchain image
   - Update UBO (model=identity, view=camera, proj=perspective with Y-flip)
   - **Record command buffer:**
     - Begin render pass (clear color 0.1 gray, clear depth)
     - **Mesh rendering** (if `showMesh`):
       - HLod skinned path: Bind skinned pipeline -> per-mesh texture descriptor + bone SSBO -> push material constants -> draw indexed
       - HLod static path: Bind standard pipeline -> per-mesh texture descriptor -> push material constants -> draw indexed
       - Simple mesh path: Bind standard pipeline -> push default material -> draw indexed
     - **Skeleton overlay** (if `showSkeleton`): Draw lines + spheres with skeleton pipeline
     - **ImGui rendering**: `ImGuiBackend::render()`
     - End render pass
   - Submit to graphics queue
   - Present to swapchain
   - Advance frame index (double-buffered: 0/1)

### Descriptor Set Layout
- **Standard pipeline** (binding 0: UBO, binding 1: texture sampler): MVP matrices + combined image sampler
- **Skinned pipeline** (binding 0: UBO, binding 1: texture sampler, binding 2: bone SSBO): Adds storage buffer of bone matrices
- **Push constants** (fragment stage): `MaterialPushConstant` with diffuse/emissive/specular colors, hover tint, flags, alpha threshold, useTexture

### Shader Details
- **`basic.vert`**: Standard MVP vertex transform. Outputs fragColor, fragTexCoord, fragNormal, fragWorldPos
- **`basic.frag`**: Directional light (ambient 0.3 + diffuse), optional texture sampling, material push constants, alpha test, hover tint, emissive
- **`skinned.vert`**: Same as basic but reads `bones[inBoneIndex]` from SSBO for rigid skinning (1 bone per vertex). Transforms position and normal by bone matrix before MVP
- **`skeleton.vert`**: Simple position+color vertex shader for line/point rendering
- **`skeleton.frag`**: Applies hover tint push constant to vertex color

## 4. W3D Chunk Types Currently Parsed

### Fully Parsed Top-Level Chunks
| Chunk | Handler | Data Produced |
|-------|---------|---------------|
| `MESH` (0x00000000) | `MeshParser::parse()` | `Mesh` struct |
| `HIERARCHY` (0x00000100) | `HierarchyParser::parse()` | `Hierarchy` struct |
| `ANIMATION` (0x00000200) | `AnimationParser::parse()` | `Animation` struct |
| `COMPRESSED_ANIMATION` (0x00000280) | `AnimationParser::parseCompressed()` | `CompressedAnimation` struct |
| `HLOD` (0x00000700) | `HLodParser::parse()` | `HLod` struct |
| `BOX` (0x00000740) | `HLodParser::parseBox()` | `Box` struct |

### Mesh Sub-Chunks Parsed
`MESH_HEADER3`, `VERTICES`, `VERTEX_NORMALS`, `TEXCOORDS`, `TRIANGLES`, `VERTEX_COLORS`, `VERTEX_SHADE_INDICES`, `VERTEX_INFLUENCES`, `MESH_USER_TEXT`, `MATERIAL_INFO`, `SHADERS`, `VERTEX_MATERIALS` (with `VERTEX_MATERIAL_NAME`, `VERTEX_MATERIAL_INFO`, `VERTEX_MAPPER_ARGS0/1`), `TEXTURES` (with `TEXTURE_NAME`, `TEXTURE_INFO`), `MATERIAL_PASS` (with `VERTEX_MATERIAL_IDS`, `SHADER_IDS`, `DCG`, `DIG`, `SCG`, `TEXTURE_STAGE` with `TEXTURE_IDS`, `STAGE_TEXCOORDS`, `PER_FACE_TEXCOORD_IDS`), `AABTREE` (with `AABTREE_HEADER`, `AABTREE_POLYINDICES`, `AABTREE_NODES`)

### Mesh Sub-Chunks Recognized but Skipped
`PRELIT_UNLIT`, `PRELIT_VERTEX`, `PRELIT_LIGHTMAP_MULTI_PASS`, `PRELIT_LIGHTMAP_MULTI_TEXTURE`

### Hierarchy Sub-Chunks Parsed
`HIERARCHY_HEADER`, `PIVOTS`, `PIVOT_FIXUPS`

### Animation Sub-Chunks Parsed
`ANIMATION_HEADER`, `ANIMATION_CHANNEL`, `BIT_CHANNEL`, `COMPRESSED_ANIMATION_HEADER`, `COMPRESSED_ANIMATION_CHANNEL`, `COMPRESSED_BIT_CHANNEL`

### HLod Sub-Chunks Parsed
`HLOD_HEADER`, `HLOD_LOD_ARRAY` (with `HLOD_SUB_OBJECT_ARRAY_HEADER`, `HLOD_SUB_OBJECT`), `HLOD_AGGREGATE_ARRAY`, `HLOD_PROXY_ARRAY`

### Chunk Types Defined but NOT Parsed (Skipped at Top-Level)
All the following are defined in `chunk_types.hpp` but skipped by `Loader::loadFromMemory()`:
- `HTREE` (legacy hierarchy)
- `HMODEL` (legacy model)
- `AGGREGATE`, `COLLECTION`
- `EMITTER` (particle emitters)
- `LIGHT`, `SPOT_LIGHT_INFO`
- `DAZZLE`
- `SOUNDROBJ`
- `MORPH_ANIMATION`
- `DEFORM`
- `PS2_SHADERS`
- `TEXTURE_REPLACER_INFO`
- `SPHERE`, `RING`, `NULL_OBJECT`

### Compressed Animation Flavors Supported
- **Timecoded** channels (X/Y/Z/Q with per-keyframe time codes)
- **AdaptiveDelta** channels (defined in enums but handling is in the compressed parser)

## 5. Terrain and Map-Related Code

**There is effectively no terrain or map file support in the codebase.** Specifically:

- The BIG archive lists include `TerrainZH.big` and `MapsZH.big` in both `big_archive_manager.cpp` and `asset_registry.cpp`, meaning these archives are **opened and indexed** for asset names. However, no code exists to parse terrain data or map files.
- The `AssetRegistry` only indexes files by extension (`.w3d`, `.dds`, `.tga`, `.ini`), so terrain-specific files (heightmaps, blend textures, etc.) would not be individually cataloged unless they happen to match those extensions.
- There are **zero references** to terrain parsing, heightmap loading, terrain rendering, or map file parsing anywhere in the `src/` or `tests/` directories.
- The `GeneralsGameCode` submodule under `lib/` contains the original game's WorldBuilder and map tools as reference, but none of that code is compiled or used by the viewer.
- A single mention in `docs/user-guide/loading-models.md` references "terrain pieces" as an example of model types, but this refers to W3D meshes used as terrain props, not actual terrain systems.

## 6. Asset Loading Pipeline (File to Screen)

### W3D Model Loading Flow
```
User opens file (FileBrowser or ModelBrowser)
  |
  v
Application::loadW3DFile() or loadModelByName()
  |
  v
ModelLoader::load()
  |-- Loader::load() -- Read binary file, parse chunks
  |     |-- MeshParser::parse() -- For each MESH chunk
  |     |-- HierarchyParser::parse() -- For each HIERARCHY chunk
  |     |-- AnimationParser::parse() / parseCompressed() -- For each ANIMATION chunk
  |     |-- HLodParser::parse() -- For each HLOD chunk
  |     |-- Returns W3DFile struct
  |
  |-- SkeletonPose::computeRestPose() -- Build bone world transforms
  |-- SkeletonRenderer::updateFromPose() -- Build debug visualization
  |-- BoneMatrixBuffer::update() -- Upload initial bone matrices
  |-- AnimationPlayer::load() -- Register available animations
  |-- ModelLoader::loadTextures() -- For each unique texture name:
  |     |-- TextureManager::loadTexture()
  |           |-- Resolve path (disk -> BIG archive extraction)
  |           |-- Load TGA or DDS file
  |           |-- Create staging buffer, upload to GPU image
  |           |-- Create image view + sampler
  |           |-- Store in name -> index map
  |
  |-- If HLod present:
  |     |-- HLodModel::loadSkinned() or load()
  |           |-- MeshConverter::convertAllSkinned() or convertAllWithPose()
  |           |     |-- For each mesh: extract vertices, normals, UVs, colors, indices
  |           |     |-- Split by per-triangle texture assignment
  |           |     |-- Add bone indices for skinned meshes
  |           |-- Create GPU VertexBuffer + IndexBuffer per sub-mesh
  |           |-- Organize into LOD levels + aggregates
  |
  |-- Else (no HLod):
  |     |-- RenderableMesh::loadWithPose()
  |           |-- MeshConverter::convertAllWithPose()
  |           |-- Create GPU buffers per mesh
  |
  |-- Camera::setTarget() -- Center on model bounds
```

### Texture Resolution Order
1. Check disk at configured texture path (CLI `--textures` or settings `texturePath`)
2. Try case-insensitive name matching (with `.tga` and `.dds` extensions)
3. If BIG archive support is active: extract from archive to cache, then load from cache

## 7. Camera/Scene Management System

### Camera
- **Type**: Orbit camera (`gfx::Camera`)
- **Controls**: Left-mouse drag rotates (yaw/pitch), scroll wheel zooms (distance)
- **Parameters**: target point, distance (0.1 to 10000), yaw, pitch (-1.5 to 1.5 rad)
- **Auto-centering**: When a model loads, `Camera::setTarget()` is called with the model's bounding box center and 2.5x the bounding radius as initial distance
- **View matrix**: Computed from spherical coordinates around target point
- **Projection**: Perspective with 45-degree FOV, 0.01 to 10000 near/far planes, Vulkan Y-flip

### Scene Management
- `Scene` class exists but is **minimally used** -- it is a simple vector of `IRenderable*` pointers with add/remove/clear
- The actual rendering is driven directly by the `Application` main loop and `Renderer::recordCommandBuffer()`, not through a scene graph
- No spatial partitioning, frustum culling, or render queue sorting is implemented
- LOD selection is handled by `HLodModel::updateLOD()` using screen-size thresholds from the W3D HLod data

## 8. Build System Structure

### CMake Configuration
- **Minimum version**: 3.28
- **C++ standard**: C++20
- **Generator**: Ninja (default via presets)
- **Main target**: `VulkanW3DViewer` executable
- **Test mode**: `BUILD_TESTING=ON` builds only test targets (no Vulkan dependency)

### Dependencies (Linked Libraries)
| Library | Purpose | Link Target |
|---------|---------|-------------|
| Vulkan SDK | Graphics API | `Vulkan::Vulkan` |
| GLFW | Windowing/input | `glfw` |
| GLM | Math | `glm::glm` |
| BigXtractor | BIG archive reading | `big::big` |
| ImGui | UI (compiled as sources) | Direct inclusion |
| CLI11 | CLI arguments | Header-only |
| nlohmann/json | JSON settings | Header-only |
| Google Test | Testing | `gtest`, `gtest_main` |
| Vulkan-Hpp | C++ Vulkan headers | Header-only (from submodule, not system SDK) |

### Shader Compilation
- Shaders are compiled from GLSL to SPIR-V using `glslc` (from Vulkan SDK)
- Compiled SPIR-V is **embedded into a C++ header** (`embedded_shaders.hpp`) via `cmake/EmbedShaders.cmake`
- The `shader_loader.hpp` loads shaders from this embedded data at runtime (no runtime file I/O)

### Test Executables (8 targets)
| Target | Tests |
|--------|-------|
| `w3d_tests` | Chunk reader, mesh parser, hierarchy parser, animation parser, HLod parser, loader |
| `texture_tests` | Texture file format loading (TGA/DDS) |
| `mesh_converter_tests` | W3D mesh to GPU vertex conversion |
| `skeleton_tests` | Skeleton pose computation, animation player |
| `bounding_box_tests` | AABB operations |
| `raycast_tests` | Ray-triangle, ray-sphere, ray-line intersection |
| `hlod_hover_tests` | HLod mesh hover detection |
| `file_browser_tests` | File browser UI logic |
| `mesh_visibility_tests` | Mesh show/hide state logic |

All test targets use stubs under `tests/stubs/` to replace Vulkan-dependent headers, allowing tests to build without Vulkan SDK.

### Build Presets
- **Configure**: `debug`, `release`, `test`, `clang-debug`, `clang-release`, `gcc-debug`, `gcc-release`, `msvc-debug`, `msvc-release`, `clang-test`, `gcc-test`, `msvc-test`
- **Build**: Matching build presets for each
- **Test**: `test`, `clang-test`, `gcc-test`, `msvc-test` (with verbose output on failure)
</task_result>
```

**Tool: task**

**Input:**
```json
{
  "description": "Research W3D terrain/map formats",
  "prompt": "Research the Command &amp; Conquer Generals / BFME W3D terrain and map file formats. I need to understand how terrain rendering works in the original SAGE engine games.\n\nSearch the codebase at C:\\CodeProjects\\VulkanW3DViewer for any references to:\n- Terrain, heightmap, height field\n- Map files (.map format)\n- TGA files related to terrain\n- Blend tiles, cliff tiles\n- Water rendering\n- Texture tiles / terrain textures\n- Any documentation or comments about map/terrain formats\n\nAlso look for any external references, documentation files, or links to format specifications in the codebase.\n\nReturn everything you find related to terrain, maps, and scene rendering formats.",
  "subagent_type": "explore"
}
```

**Output:**
```
task_id: ses_384b99bb2ffegzYjHJb7y9VyIN (for resuming to continue this task if needed)

<task_result>
I now have a comprehensive picture of the terrain system. Let me compile my findings.

---

## Complete Research Report: SAGE Engine Terrain & Map File Formats

### 1. Source Files Found in the Codebase

All terrain-related original SAGE engine source code resides under `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\`. The VulkanW3DViewer project itself currently only references terrain/map BIG archives but does **not** yet implement terrain parsing or rendering. There is an active git branch `feat/terrain-rendering` and remote branches `feat/terrain-parsing` and `feat/terrain-rendering` indicating this work is in progress.

#### Key Original SAGE Engine Source Files

**Core terrain data structures and parsing:**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\WorldHeightMap.h` -- Primary heightmap data class
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Source\W3DDevice\GameClient\WorldHeightMap.cpp` -- Full parser implementation (2535 lines)
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\TileData.h` -- Tile bitmap storage (64x64 pixels per tile)
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Source\W3DDevice\GameClient\TileData.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngine\Include\Common\MapObject.h` -- Map object definitions, MAP_XY_FACTOR, MAP_HEIGHT_SCALE

**Map file version constants:**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\GameEngine\Include\Common\MapReaderWriterInfo.h` -- All chunk version constants

**Terrain rendering classes:**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\BaseHeightMap.h` -- Base render object (332 lines)
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\HeightMap.h` -- Full 3D heightmap renderer
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Source\W3DDevice\GameClient\HeightMap.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\FlatHeightMap.h` -- LOD/flat terrain renderer
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Source\W3DDevice\GameClient\FlatHeightMap.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\W3DTerrainBackground.h` -- Flat tile-based background rendering
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Source\W3DDevice\GameClient\W3DTerrainBackground.cpp`

**Terrain texture generation:**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\TerrainTex.h` -- TerrainTextureClass, AlphaTerrainTextureClass, AlphaEdgeTextureClass, CloudMapTerrainTextureClass
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Source\W3DDevice\GameClient\TerrainTex.cpp`

**Terrain visual interface:**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngine\Include\GameClient\TerrainVisual.h` -- Abstract interface
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\W3DTerrainVisual.h` -- W3D implementation

**Water rendering:**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\W3DWater.h` -- WaterRenderObjClass (291 lines)
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Source\W3DDevice\GameClient\Water\W3DWater.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngine\Include\GameClient\Water.h` -- WaterSetting, WaterTransparencySetting

**Terrain logic (pathfinding, collision):**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\GameEngine\Include\GameLogic\TerrainLogic.h`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\GameEngine\Source\GameLogic\Map\TerrainLogic.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\GameEngineDevice\Include\W3DDevice\GameLogic\W3DTerrainLogic.h`

**Terrain types (INI definitions):**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\GameEngine\Include\Common\TerrainTypes.h` -- TerrainClass enum, TerrainType class
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\GameEngine\Source\Common\TerrainTypes.cpp`

**Roads, tracks, and other terrain features:**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngine\Include\GameClient\TerrainRoads.h`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngine\Source\GameClient\Terrain\TerrainRoads.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\W3DTerrainTracks.h`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Source\W3DDevice\GameClient\W3DTerrainTracks.cpp`

**WorldBuilder (map editor) -- the writer side:**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\Tools\WorldBuilder\src\WHeightMapEdit.cpp` -- saveToFile() writes the .map format
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\Tools\WorldBuilder\src\MapPreview.cpp` -- TGA preview generation
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\Tools\WorldBuilder\src\WaterTool.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\Tools\WorldBuilder\src\WaterOptions.cpp`

**Water INI parsing:**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\GameEngine\Source\Common\INI\INIWater.cpp`

**Example map.ini file:**
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsReplays\GeneralsZH\1.04\Maps\[RANK] Arctic Arena ZH v1\map.ini`

**VulkanW3DViewer's own references to terrain/map archives:**
- `C:\CodeProjects\VulkanW3DViewer\src\lib\formats\big\big_archive_manager.cpp` -- Loads `TerrainZH.big` and `MapsZH.big`
- `C:\CodeProjects\VulkanW3DViewer\src\lib\formats\big\asset_registry.cpp` -- Scans terrain and map archives

---

### 2. The .map File Format (DataChunk-based)

The .map file uses a **named chunk** format (distinct from the W3D binary chunk format). It is implemented via `DataChunkInput`/`DataChunkOutput` classes. Each chunk has a string name, a version integer, and binary data. The top-level chunks in a .map file are:

| Chunk Name | Version | Description |
|---|---|---|
| `"HeightMapData"` | `K_HEIGHT_MAP_VERSION_4` (4) | Raw heightmap grid |
| `"BlendTileData"` | `K_BLEND_TILE_VERSION_8` (8) | Tile textures, blend info, cliff info |
| `"WorldInfo"` | `K_WORLDDICT_VERSION_1` (1) | World dictionary (weather, etc.) |
| `"SidesList"` | varies | Player/team definitions |
| `"ObjectsList"` / `"Object"` | `K_OBJECTS_VERSION_3` (3) | Placed objects (buildings, units, waypoints, lights, scorches) |
| `"PolygonTriggers"` | `K_TRIGGERS_VERSION_4` (4) | Polygon trigger areas (including water areas, rivers) |
| `"GlobalLighting"` | `K_LIGHTING_VERSION_3` (3) | Time-of-day lighting for terrain and objects |

---

### 3. HeightMapData Chunk

**Format (version 4):**
```
Int32  width           -- Number of vertices in X
Int32  height          -- Number of vertices in Y
Int32  borderSize      -- Non-playable border (in cells)
Int32  numBoundaries   -- Number of boundary rectangles
[for each boundary]:
  Int32  x             -- Boundary width
  Int32  y             -- Boundary height
Int32  dataSize        -- Must equal width * height
Byte[dataSize]         -- Raw height values, 0-255
```

**Key constants:**
- `MAP_XY_FACTOR = 10.0f` -- Each heightmap cell is 10 world units wide/tall
- `MAP_HEIGHT_SCALE = MAP_XY_FACTOR / 16.0f = 0.625f` -- Each height unit = 0.625 world units vertically
- Height range: 0-255 (byte), so max terrain height = `255 * 0.625 = 159.375` world units
- The heightmap is a regular grid of unsigned byte values addressed as `m_data[yIndex * width + xIndex]`

---

### 4. BlendTileData Chunk -- The Texture Tiling System

This is the most complex part of the terrain format. **Format (version 8):**

```
Int32     dataSize                              -- Same as heightmap dataSize
Short[dataSize]   m_tileNdxes                   -- Base tile index for each cell
Short[dataSize]   m_blendTileNdxes              -- Blend tile index (0 = no blend)
Short[dataSize]   m_extraBlendTileNdxes         -- Extra blend for 3-way blends
Short[dataSize]   m_cliffInfoNdxes              -- Cliff UV override index
Byte[height * flipStateWidth]  m_cellCliffState -- Passability flags per cell
Int32     m_numBitmapTiles                      -- Number of source tile bitmaps
Int32     m_numBlendedTiles                     -- Number of blend tile entries
Int32     m_numCliffInfo                        -- Number of cliff info entries

-- Texture classes:
Int32     m_numTextureClasses
[for each texture class]:
  Int32   firstTile       -- Index into source tiles array
  Int32   numTiles        -- Number of tiles in this class
  Int32   width           -- Tiles per row (e.g., 2 = 2x2 = 4 tiles from 128x128 TGA)
  Int32   (legacy/unused)
  String  name            -- Terrain type name (e.g., "TEDesert1")

-- Edge texture classes:
Int32     m_numEdgeTiles
Int32     m_numEdgeTextureClasses
[for each edge texture class]:
  Int32   firstTile
  Int32   numTiles
  Int32   width
  String  name

-- Blend tile info array:
[for i = 1..m_numBlendedTiles-1]:
  Int32   blendNdx        -- Tile index to blend over base
  Byte    horiz           -- Horizontal blend edge
  Byte    vert            -- Vertical blend edge
  Byte    rightDiagonal   -- Right diagonal blend
  Byte    leftDiagonal    -- Left diagonal blend
  Byte    inverted        -- Bit 0 = inverted, Bit 1 = flipped
  Byte    longDiagonal    -- Extended diagonal
  Int32   customBlendEdgeClass  -- -1 = use alpha blend
  Int32   FLAG_VAL (0x7ADA0000) -- Validation marker

-- Cliff info array:
[for i = 1..m_numCliffInfo-1]:
  Int32   tileIndex       -- Source tile for cliff UV
  Float   u0, v0          -- Upper-left UV
  Float   u1, v1          -- Lower-left UV
  Float   u2, v2          -- Lower-right UV
  Float   u3, v3          -- Upper-right UV
  Byte    flip            -- Flip triangle diagonal
  Byte    mutant          -- Mutant mapping flag
```

**Tile index encoding:** Each `m_tileNdxes` value encodes both the source tile and the quadrant within a 64x64-pixel tile. The top 14 bits (`tileNdx >> 2`) select a source tile; the bottom 2 bits select the quadrant (each heightmap cell maps to a 32x32-pixel quadrant of a 64x64-pixel tile). `Bit 0` selects left/right half; `Bit 1` selects top/bottom half.

---

### 5. Terrain Texture System

**Tile dimensions:**
- `TILE_PIXEL_EXTENT = 64` -- Each source tile is 64x64 pixels, 4 bytes/pixel (BGRA)
- `TILE_BYTES_PER_PIXEL = 4`
- `TEXTURE_WIDTH = 2048` -- Runtime texture atlas is 2048 pixels wide
- `TILE_OFFSET = 8` -- Padding between tiles in the atlas
- `NUM_SOURCE_TILES = 1024` -- Max source tile bitmaps
- `NUM_BLEND_TILES = 16192` -- Max blend tile entries
- `NUM_CLIFF_INFO = 32384` -- Max cliff UV overrides
- `NUM_TEXTURE_CLASSES = 256` -- Max terrain texture types

**How terrain textures are loaded:**
1. Terrain type names (e.g., `"TEDesert1"`) are looked up in `TheTerrainTypes` collection
2. Each terrain type references a TGA file (e.g., `"TEDesert1.tga"`)
3. TGA files are loaded from `Art/Terrain/` directory (constant: `TERRAIN_TGA_DIR_PATH = "Art/Terrain/"`)
4. TGA files are parsed and split into 64x64-pixel tiles -- a 128x128 TGA yields 4 tiles (2x2 grid)
5. Tiles are arranged into a 2048-wide texture atlas at runtime (`TerrainTextureClass`)
6. UV coordinates are computed by `getUVData()` / `getUVForNdx()` mapping heightmap cells to atlas positions

**Multi-pass terrain rendering (3 texture passes):**
1. **Pass 1 (Stage 0):** Base terrain texture -- the primary tile for each cell from the atlas
2. **Pass 2 (Stage 1):** Alpha blend texture -- the blend tile overlaid with alpha gradient blending (horizontal, vertical, diagonal edges between different terrain types). Uses 12 pre-generated alpha tiles (`NUM_ALPHA_TILES = 12`) for 6 blend directions x inverted
3. **Pass 3 (Stage 2):** Cloud map texture -- animated scrolling cloud shadows
4. **Pass 4 (Stage 3):** Light/noise map (macrotexture)
5. **Extra blend pass:** For 3-way texture blends (`m_extraBlendTileNdxes`)
6. **Custom edge pass:** For custom blend edge textures (`AlphaEdgeTextureClass`)

---

### 6. Blend System Details

The blend system creates smooth transitions between terrain types. At each cell:
- `m_tileNdxes[ndx]` = base terrain tile
- `m_blendTileNdxes[ndx]` = index into `m_blendedTiles[]` for the primary blend
- `m_extraBlendTileNdxes[ndx]` = index into `m_blendedTiles[]` for 3-way blend

Each `TBlendTileInfo` describes how one terrain tile blends over another:
- `blendNdx` = the tile that blends ON TOP of the base
- `horiz/vert/rightDiagonal/leftDiagonal` = which edge direction
- `inverted` = whether alpha is flipped (bit 0) or triangle diagonal is flipped (bit 1)
- `longDiagonal` = extended diagonal (3/4 of the cell instead of 1/2)
- `customBlendEdgeClass` = index of custom edge texture (-1 = use generated alpha gradients)

Alpha blending is done per-vertex with 4 alpha values (one per corner of the cell quad), set to either 0 or 255 based on the blend direction. The triangle diagonal flip (`flip`) ensures proper interpolation.

---

### 7. Cliff Texture Mapping

For steep terrain cells, standard tiling produces severe stretching. The engine handles this with:
- `m_cliffInfoNdxes[ndx]` indexes into `m_cliffInfo[]` for custom UV coordinates
- Each `TCliffInfo` stores 4 UV pairs (one per vertex) that project the texture along the cliff face
- Cliff state is detected by comparing corner heights: if `maxZ - minZ > 9.8f` world units, the cell is marked as cliff
- Cliff state is stored as a bitfield in `m_cellCliffState[]`

---

### 8. Heightmap Rendering Architecture

The engine has two rendering modes:

**HeightMapRenderObjClass** (full 3D):
- Grid is subdivided into vertex buffer "tiles" of 32x32 cells (`VERTEX_BUFFER_TILE_LENGTH = 32`)
- Uses `VERTEX_FORMAT = VertexFormatXYZDUV2` (position + diffuse color + 2 UV sets)
- Normal draw area: `1 + 4*32 = 129` vertices per axis
- Stretched draw area: `1 + 2*32 = 65` vertices per axis
- Vertex buffers slide around as camera moves (origin tracking)
- Dynamic lighting updates vertex diffuse colors in-place
- Triangle flipping: diagonal across each cell quad is selected to minimize visual artifacts on slopes

**FlatHeightMapRenderObjClass** (LOD mode):
- Uses `W3DTerrainBackground` tiles with pre-rendered flat textures
- Multiple resolution levels: 1x, 2x, 4x texture multiplier
- Used for distant terrain or performance-constrained rendering

**Rendering passes per frame:**
1. Base terrain geometry with texture atlas (stage 0)
2. Alpha blend overlay (stage 1) for terrain transitions
3. Cloud shadow layer (stage 2) -- scrolling animated cloud texture
4. Macro/noise texture (stage 3)
5. Extra 3-way blend tiles (separate draw calls for cells needing 3 textures)
6. Custom blend edge tiles
7. Shoreline tiles (destination alpha blending for water edges)
8. Scorch marks (separate vertex/index buffers, up to 500 scorches)
9. Roads (W3DRoadBuffer)
10. Trees (W3DTreeBuffer -- billboard alpha objects)
11. Props (W3DPropBuffer)
12. Bibs (building foundations -- W3DBibBuffer)
13. Bridges (W3DBridgeBuffer)
14. Shroud (fog of war -- W3DShroud)

---

### 9. Water Rendering System

**Water types** (enum `WaterType`):
| Value | Name | Description |
|---|---|---|
| 0 | `WATER_TYPE_0_TRANSLUCENT` | Translucent water, no reflection |
| 1 | `WATER_TYPE_1_FB_REFLECTION` | Frame buffer reflection (legacy) |
| 2 | `WATER_TYPE_2_PVSHADER` | Pixel/vertex shader with texture reflection |
| 3 | `WATER_TYPE_3_GRIDMESH` | 3D mesh-based water (deformable) |

**Water features:**
- Flat water plane with scrolling UV texture
- Per-time-of-day settings (sky texture, water texture, diffuse colors, scroll rates)
- Transparency settings via INI: `TransparentWaterDepth`, `TransparentWaterMinOpacity`, `StandingWaterColor`
- Default standing water texture: `"TWWater01.tga"`
- Skybox textures: N/E/S/W/T faces (e.g., `"TSMorningN.tga"`)
- **Bump-mapped water** (GeForce 3+): 32-frame animated bump map, custom pixel/vertex shaders, D3D8 resources
- **3D Mesh water**: Deformable grid with per-vertex height and velocity (`WaterMeshData`), used for dam/river effects
- **River water**: Drawn with polygon triggers, using trapezoid geometry
- **Shoreline blending**: Terrain tiles crossing the water plane use destination alpha for smooth water-terrain transitions. Alpha LUT stored in `m_destAlphaTexture`

**Water grid parameters:**
- Grid cells with configurable resolution (`m_gridCellsX`, `m_gridCellsY`, `m_gridCellSize`)
- Height clamping (`m_minGridHeight`, `m_maxGridHeight`)
- Velocity-based wave simulation with attenuation falloff
- Transform: position, orientation (angle), origin in world space

**Water areas defined by:**
- PolygonTrigger objects with `m_isWaterArea` flag (version >= 2)
- Rivers with `m_isRiver` flag and `m_riverStart` (version >= 3)

---

### 10. Terrain Types (INI System)

Terrain types are defined in INI files and stored in `TerrainZH.big`. The `TerrainType` class maps names to:
- `m_name` -- Entry name (e.g., `"TEDesert1"`)
- `m_texture` -- TGA filename (e.g., `"TEDesert1.tga"`)
- `m_class` -- Classification enum (`TerrainClass`: TERRAIN_DESERT_1, TERRAIN_GRASS, etc.)
- `m_blendEdgeTexture` -- Whether it has custom blend edge tiles
- `m_restrictConstruction` -- Whether building is prohibited

There are ~37 terrain classes defined including desert variants, snow, grass, rock, urban, concrete, asphalt, etc.

---

### 11. Map File Companion Files

Each map lives in `Maps/mapname/` with:
- `mapname.map` -- The binary map data (DataChunk format described above)
- `mapname.tga` -- Map preview image (generated by MapPreview, 128x128 or similar)
- `map.ini` -- Per-map INI overrides (weather, water, object modifications)

---

### 12. Global Lighting (in .map file)

The `"GlobalLighting"` chunk stores settings for 4 time-of-day periods (Morning, Afternoon, Evening, Night):
- For each period: terrain lighting (ambient RGB, diffuse RGB, light direction XYZ)
- Object lighting (separate ambient, diffuse, direction)
- Up to `MAX_GLOBAL_LIGHTS` (3) lights per category
- Optional shadow color (uint32)

---

### 13. BIG Archives Relevant to Terrain

The VulkanW3DViewer loads:
- **`TerrainZH.big`** -- Contains terrain TGA textures in `Art/Terrain/` path
- **`MapsZH.big`** -- Contains .map files and map.ini files

---

### 14. Current VulkanW3DViewer Status

The viewer currently does **not** implement terrain/map parsing. Evidence of planned work:
- BIG archive manager already loads `TerrainZH.big` and `MapsZH.big`
- Active branch: `feat/terrain-rendering`
- Remote branches exist for `feat/terrain-parsing` and `feat/terrain-rendering`
- No terrain-related code exists under `src/` (only model/mesh/texture handling)
</task_result>
```

---

## Assistant (Plan · claude-opus-4-6 · 150.0s)

Excellent -- I now have a thorough understanding of both your current codebase and the original SAGE engine terrain system. Before I draft a detailed plan, I have some clarifying questions:

**Tool: question**

**Input:**
```json
{
  "questions": [
    {
      "question": "Which Command & Conquer game(s) do you want to target first? The map format evolved across games (Generals, Zero Hour, BFME, BFME2). Zero Hour seems like the current focus given the 'ZH' BIG archives.",
      "header": "Target Game",
      "options": [
        {
          "label": "Zero Hour only (Recommended)",
          "description": "Focus on C&C Generals: Zero Hour map format first, since the codebase already loads ZH BIG archives"
        },
        {
          "label": "Generals + Zero Hour",
          "description": "Support both base Generals and Zero Hour (very similar formats)"
        },
        {
          "label": "All SAGE engine games",
          "description": "Aim for broad SAGE compatibility from the start (Generals, ZH, BFME, BFME2)"
        }
      ]
    },
    {
      "question": "What is the primary goal for this terrain renderer? This affects what rendering passes we prioritize.",
      "header": "Primary Goal",
      "options": [
        {
          "label": "Full scene viewer (Recommended)",
          "description": "Load a .map file and render the complete scene (terrain + water + objects + lighting) as a viewer/tool for the community"
        },
        {
          "label": "Map editor foundation",
          "description": "Build toward an interactive WorldBuilder replacement where terrain can be edited"
        },
        {
          "label": "Rendering pipeline library",
          "description": "Create a reusable rendering library that other community tools can integrate"
        }
      ]
    },
    {
      "question": "How faithful should the rendering be to the original game? This determines shader complexity and rendering approach.",
      "header": "Rendering Fidelity",
      "options": [
        {
          "label": "Visually faithful (Recommended)",
          "description": "Match the original look: texture blending, water, lighting. Use modern Vulkan techniques but aim for visual parity with the game"
        },
        {
          "label": "Enhanced/modern rendering",
          "description": "Take creative license -- PBR, better shadows, improved water. Looks better than original but different"
        },
        {
          "label": "Exact pixel match",
          "description": "Replicate the exact D3D8 rendering pipeline behavior for archival/comparison purposes"
        }
      ]
    },
    {
      "question": "What level of object placement support do you want in the initial version? Maps contain buildings, trees, props, etc.",
      "header": "Object Placement",
      "options": [
        {
          "label": "Terrain + water first",
          "description": "Start with just the terrain heightmap, textures, blending, and water. Add objects later."
        },
        {
          "label": "Terrain + static objects",
          "description": "Include terrain plus placed W3D objects (buildings, trees, props) from the map's ObjectsList"
        },
        {
          "label": "Full scene with all features",
          "description": "Everything: terrain, water, objects, roads, bridges, scorches, shroud, lighting"
        }
      ]
    },
    {
      "question": "Should this remain a standalone viewer application, or are you thinking about this as a broader community rendering engine/SDK?",
      "header": "Architecture Scope",
      "options": [
        {
          "label": "Extend current viewer",
          "description": "Add terrain/map rendering as a new mode within the existing VulkanW3DViewer application"
        },
        {
          "label": "Separate library + viewer",
          "description": "Build the terrain/map rendering as a reusable library, with the viewer as one consumer. Other tools could use the library."
        },
        {
          "label": "New application",
          "description": "Create a separate application for map viewing, sharing parsing/rendering code with the existing viewer"
        }
      ]
    },
    {
      "question": "The original engine uses a DataChunk format for .map files (different from W3D binary chunks). Do you want to also support reading maps directly from BIG archives, or only loose .map files on disk?",
      "header": "Map File Source",
      "options": [
        {
          "label": "Both BIG + loose files (Recommended)",
          "description": "Support loading .map files from both BIG archives and directly from disk"
        },
        {
          "label": "Loose files only",
          "description": "Only load .map files from disk (simpler to start with)"
        },
        {
          "label": "BIG archives only",
          "description": "Only load from MapsZH.big archive"
        }
      ]
    }
  ]
}
```

**Output:**
```
User has answered your questions: "Which Command & Conquer game(s) do you want to target first? The map format evolved across games (Generals, Zero Hour, BFME, BFME2). Zero Hour seems like the current focus given the 'ZH' BIG archives."="Zero Hour only (Recommended)", "What is the primary goal for this terrain renderer? This affects what rendering passes we prioritize."="Map editor foundation", "How faithful should the rendering be to the original game? This determines shader complexity and rendering approach."="Visually faithful (Recommended)", "What level of object placement support do you want in the initial version? Maps contain buildings, trees, props, etc."="Full scene with all features", "Should this remain a standalone viewer application, or are you thinking about this as a broader community rendering engine/SDK?"="Separate library + viewer", "The original engine uses a DataChunk format for .map files (different from W3D binary chunks). Do you want to also support reading maps directly from BIG archives, or only loose .map files on disk?"="Both BIG + loose files (Recommended)". You can now continue with the user's answers in mind.
```

---

## Assistant (Plan · claude-opus-4-6 · 60.3s)

Great answers -- this is an ambitious but very exciting direction. A few more targeted questions before I finalize the plan:

**Tool: question**

**Input:**
```json
{
  "questions": [
    {
      "question": "You selected 'Map editor foundation'. How soon do you want editing capabilities? Should the initial implementation be read-only (view maps) with the architecture designed to support future editing, or do you want basic editing (height painting, texture painting) in the first deliverable?",
      "header": "Editing Timeline",
      "options": [
        {
          "label": "View-first, edit-ready arch (Recommended)",
          "description": "Build a read-only viewer first, but architect data structures to be mutable so editing can be layered on. This avoids premature complexity."
        },
        {
          "label": "Basic editing in v1",
          "description": "Include basic terrain height/texture editing tools in the initial release"
        },
        {
          "label": "Full editor in v1",
          "description": "Build a complete WorldBuilder replacement from the start"
        }
      ]
    },
    {
      "question": "For the 'separate library + viewer' architecture, should the library be header-only, a static lib, or a shared/dynamic library? This affects how other community tools would consume it.",
      "header": "Library Type",
      "options": [
        {
          "label": "Static library (Recommended)",
          "description": "CMake static lib target. Simplest for C++ consumers, no ABI issues, easy to integrate via CMake."
        },
        {
          "label": "Shared/DLL library",
          "description": "Dynamic library with C API. Allows non-C++ consumers (Python, C#, etc.) but more complex to maintain."
        },
        {
          "label": "Header-only parsing + static rendering",
          "description": "Parsing layer is header-only (easy to reuse anywhere), rendering layer is a static lib (Vulkan-dependent)"
        }
      ]
    },
    {
      "question": "The existing viewer renders models on a plain gray background. For the map viewer, what camera system do you envision?",
      "header": "Camera System",
      "options": [
        {
          "label": "RTS camera (Recommended)",
          "description": "Top-down angled camera like the original game: pan with mouse/WASD, zoom in/out, rotation. Natural for viewing maps."
        },
        {
          "label": "Free fly camera",
          "description": "WASD + mouse look free camera. More flexible but less natural for RTS maps."
        },
        {
          "label": "Both modes",
          "description": "Support toggling between RTS-style and free-fly camera modes"
        }
      ]
    },
    {
      "question": "Do you have a preference on how to handle the INI system? Terrain types, water settings, and many visual parameters are defined in INI files. The codebase has a placeholder IniExtractor but no INI parser.",
      "header": "INI Parsing",
      "options": [
        {
          "label": "Custom parser (Recommended)",
          "description": "Write a focused parser for the SAGE INI dialect (it's not standard INI format -- it has nested blocks, field types, etc.)"
        },
        {
          "label": "Hardcoded defaults + override",
          "description": "Start with hardcoded terrain type definitions based on known ZH values. Add INI parsing later."
        },
        {
          "label": "Leverage GeneralsGameCode",
          "description": "Port/adapt the INI parsing code from the GeneralsGameCode reference source"
        }
      ]
    }
  ]
}
```

**Output:**
```
User has answered your questions: "You selected 'Map editor foundation'. How soon do you want editing capabilities? Should the initial implementation be read-only (view maps) with the architecture designed to support future editing, or do you want basic editing (height painting, texture painting) in the first deliverable?"="View-first, edit-ready arch (Recommended)", "For the 'separate library + viewer' architecture, should the library be header-only, a static lib, or a shared/dynamic library? This affects how other community tools would consume it."="Static library (Recommended)", "The existing viewer renders models on a plain gray background. For the map viewer, what camera system do you envision?"="RTS camera (Recommended)", "Do you have a preference on how to handle the INI system? Terrain types, water settings, and many visual parameters are defined in INI files. The codebase has a placeholder IniExtractor but no INI parser."="Custom parser (Recommended)". You can now continue with the user's answers in mind.
```

---

## Assistant (Plan · claude-opus-4-6 · 210.6s)

Now let me look at a few more specific technical details in the codebase to make sure the plan accounts for the current architecture correctly.

**Tool: task**

**Input:**
```json
{
  "description": "Examine rendering pipeline details",
  "prompt": "I need to examine specific implementation details in the VulkanW3DViewer codebase at C:\\CodeProjects\\VulkanW3DViewer for planning a terrain rendering extension. Please investigate and report on:\n\n1. **Renderer class** (`src/core/renderer.hpp` and `renderer.cpp`): How is the rendering pipeline structured? How are pipelines switched? How are draw calls organized? Is there an abstraction for render passes or is it hardcoded?\n\n2. **Pipeline class** (`src/lib/gfx/pipeline.hpp` and `pipeline.cpp`): How are graphics pipelines created? What vertex formats are supported? How flexible is the descriptor set layout system?\n\n3. **VulkanContext** (`src/lib/gfx/vulkan_context.hpp` and `vulkan_context.cpp`): Does it use a single render pass or dynamic rendering? How is the swapchain managed? What's the depth buffer setup?\n\n4. **Buffer classes** (`src/lib/gfx/buffer.hpp` and `buffer.cpp`): How are vertex/index buffers managed? Is there support for dynamic/streaming buffers (needed for terrain that updates)?\n\n5. **TextureManager** (`src/lib/gfx/texture.hpp` and `texture.cpp`): How are textures stored and bound? Is there support for texture arrays or atlases? What's the max texture count?\n\n6. **Scene class** (`src/lib/scene/scene.hpp` and `scene.cpp`): How minimal is it? What would need to change to support a scene graph with terrain + objects?\n\n7. **The existing CMakeLists.txt**: How are targets organized? What would need to change to extract a library target?\n\nReport the key findings, especially any architectural limitations that would need to be addressed for terrain rendering (e.g., single pipeline, single render pass, no multi-draw, no dynamic vertex buffers, etc.).",
  "subagent_type": "explore"
}
```

**Output:**
```
task_id: ses_384b1e6d8ffey3SfWpOZjEH5AW (for resuming to continue this task if needed)

<task_result>
Now I have a complete picture of the architecture. Here is my detailed report:

---

## Architectural Analysis for Terrain Rendering Extension

### 1. Renderer Class (`src/core/renderer.hpp` / `renderer.cpp`)

**Pipeline Structure:**
- The renderer holds exactly **two pipelines**: `pipeline_` (basic) and `skinnedPipeline_` (GPU skinning with bone SSBO). Each has its own `DescriptorManager`.
- Pipelines are switched with explicit `cmd.bindPipeline()` calls inside `recordCommandBuffer()`. There is no pipeline abstraction layer or registry -- switching is done inline with `if/else` branches checking `RenderState` flags.

**Draw Call Organization:**
- All draw calls happen inside a single `recordCommandBuffer()` method (lines 129-313 of `renderer.cpp`). The method follows a linear sequence:
  1. Begin render pass
  2. Bind basic pipeline
  3. Draw 3D content (HLod skinned, HLod unskinned, or simple mesh -- mutually exclusive paths)
  4. Switch back to basic pipeline if needed
  5. Draw skeleton overlay
  6. Draw ImGui
  7. End render pass
- Draw calls are **individual `drawIndexed()` per mesh** -- each mesh rebinds its own vertex buffer, index buffer, and descriptor set. There is no batching, multi-draw indirect, or instancing.

**Render Pass Abstraction:**
- There is **no render pass abstraction**. The render pass is obtained directly from `VulkanContext` via `context_->renderPass()` and `context_->framebuffer(imageIndex)`. The single render pass with a single subpass is hardcoded.

**Frame Management:**
- Double-buffered with `MAX_FRAMES_IN_FLIGHT = 2`.
- Standard fence-based synchronization with `waitForCurrentFrame()` + acquire/submit/present flow.

**Key Limitation for Terrain:**
The `FrameContext` struct (line 31) is tightly coupled to the current object types: it requires `RenderableMesh &`, `HLodModel &`, `SkeletonRenderer &`, and `HoverDetector &`. Adding terrain would require modifying this struct or creating a new rendering path.

---

### 2. Pipeline Class (`src/lib/gfx/pipeline.hpp` / `pipeline.cpp`)

**Pipeline Creation:**
- Three creation methods: `create()`, `createWithTexture()`, and `createSkinned()`. All are essentially hardcoded configurations with slight variations.
- `create()` simply delegates to `createWithTexture()` with default config.
- `createSkinned()` duplicates most of `createWithTexture()` but adds a 3rd descriptor binding (SSBO for bones at binding 2) and uses `SkinnedVertex`.
- Pipelines are always created with `eTriangleList` topology -- no support for triangle strips, lines, or patches.

**Vertex Formats (hardcoded):**
- `Vertex`: position (vec3), normal (vec3), texCoord (vec2), color (vec3) -- 44 bytes per vertex.
- `SkinnedVertex`: same as Vertex plus `boneIndex` (uint32) -- 48 bytes per vertex.
- Both formats are hardcoded in the pipeline creation. There is no mechanism to specify custom vertex formats at pipeline creation time.

**Descriptor Set Layout:**
- **Basic pipeline**: 2 bindings -- uniform buffer (binding 0, vertex stage) + combined image sampler (binding 1, fragment stage).
- **Skinned pipeline**: 3 bindings -- same as basic + storage buffer (binding 2, vertex stage) for bone matrices.
- Layouts are created inline inside pipeline creation methods. There is no reusable descriptor set layout builder.

**Descriptor Managers:**
- `DescriptorManager` and `SkinnedDescriptorManager` are separate classes (not polymorphic). Both pre-allocate `frameCount * maxTextures` descriptor sets (default `maxTextures = 64`).
- Texture descriptor sets are allocated eagerly (all 128 sets = 2 frames * 64 textures) and initialized lazily on first use via `getTextureDescriptorSet()`. The UBO binding is copied from the per-frame base set.

**PipelineConfig:**
- A minimal configuration struct: `enableBlending`, `alphaBlend`, `depthWrite`, `twoSided`. No support for wireframe, line width, tessellation, or custom topology.

**Key Limitation for Terrain:**
A terrain pipeline would need a different vertex format (e.g., position + heightmap UV, possibly without normals if computed in shader). The current architecture requires either duplicating the entire pipeline creation method again or refactoring to accept vertex format as a parameter. The fixed topology (`eTriangleList`) prevents using triangle strips for terrain. There is also no tessellation support.

---

### 3. VulkanContext (`src/lib/gfx/vulkan_context.hpp` / `vulkan_context.cpp`)

**Render Pass:**
- Uses a **traditional VkRenderPass** (not dynamic rendering). Single render pass with a single subpass.
- Two attachments: color (swapchain format, `eB8G8R8A8Srgb` preferred) and depth.
- The render pass is created once in `init()` and **not recreated on swapchain resize** (only framebuffers are recreated in `recreateSwapchain()`).

**Swapchain Management:**
- Standard min+1 image count, prefers Mailbox present mode (falls back to FIFO).
- `recreateSwapchain()` cleanly destroys framebuffers, depth resources, image views, and swapchain, then recreates them. Render pass is preserved since the format does not change.

**Depth Buffer:**
- Single depth image matching the swapchain extent.
- Format preference order: `D32Sfloat` > `D32SfloatS8Uint` > `D24UnormS8Uint`.
- Standard device-local memory, recreated on swapchain resize.
- **No MSAA** -- sample count is always `e1` for both color and depth.

**Vulkan API Level:**
- Targets Vulkan API 1.2.
- Only required device extension is `VK_KHR_SWAPCHAIN_EXTENSION_NAME`. No dynamic rendering extension, no buffer device address, no ray tracing.
- Optional feature: `samplerAnisotropy`.

**Command Pool:**
- Single command pool with `eResetCommandBuffer` flag. Used for both rendering command buffers and single-time transfer commands.
- `beginSingleTimeCommands()` / `endSingleTimeCommands()` uses a blocking `waitIdle()` after submission -- synchronous GPU transfers.

**Key Limitation for Terrain:**
The single render pass is fine for adding terrain (just another draw call within the existing subpass). However, if terrain requires a separate depth pre-pass, shadow maps, or multi-pass rendering, the architecture would need significant extension. The lack of dynamic rendering is not critical but means render pass compatibility constraints must be respected for any new pipelines.

---

### 4. Buffer Classes (`src/lib/gfx/buffer.hpp` / `buffer.cpp`)

**Buffer Architecture:**
- `Buffer`: Low-level wrapper around `VkBuffer` + `VkDeviceMemory`. Supports `create()`, `destroy()`, `map()`, `unmap()`, `upload()`. Move-only (RAII). Memory is allocated per-buffer (no allocator/VMA).
- `StagedBuffer`: Wraps a device-local `Buffer` with staging transfer. Created via `create(context, data, size, usage)` which allocates a staging buffer, uploads, copies via single-time command, and destroys the staging buffer. **One-shot immutable** -- no method to update after creation.
- `VertexBuffer<T>`: Template wrapping `StagedBuffer` for vertex data. Create-once, no update path.
- `IndexBuffer`: Same as `VertexBuffer` but for `uint32_t` indices.
- `UniformBuffer<T>`: Per-frame host-visible/host-coherent buffers. Supports per-frame `update()` via `map()/memcpy/unmap()`. This is the only buffer type with an update path.

**Dynamic/Streaming Buffer Support:**
- **None.** `VertexBuffer` and `IndexBuffer` are create-once immutable (device-local via staging). There is no:
  - Ring buffer / dynamic buffer allocation
  - Persistent mapping
  - Host-visible vertex/index buffers
  - Buffer sub-allocation or offset-based updates

**Memory Management:**
- Every buffer allocation does its own `vkAllocateMemory`. No VMA (Vulkan Memory Allocator) or pooling. This will hit the per-device allocation limit quickly with many small buffers.

**Key Limitation for Terrain:**
Terrain LOD with dynamic mesh updates (clipmaps, chunked updates) is not supported by the current buffer system. You would need to add either:
1. A `DynamicVertexBuffer` class using host-visible memory with persistent mapping, or
2. A staging ring buffer for streaming updates to device-local buffers.
The per-buffer memory allocation pattern would also need to be replaced with VMA or a sub-allocator for terrain chunks.

---

### 5. TextureManager (`src/lib/gfx/texture.hpp` / `texture.cpp`)

**Storage:**
- Textures are stored in a flat `std::vector<GPUTexture>` with a `std::unordered_map<std::string, uint32_t>` name-to-index lookup.
- Each `GPUTexture` owns its own `VkImage`, `VkDeviceMemory`, `VkImageView`, and `VkSampler` (individual allocations, no pooling).

**Texture Binding:**
- Each texture gets its own `VkSampler` (created individually via `createSampler()`). All samplers use identical settings (linear filter, repeat wrapping, max 16x anisotropy).
- Textures are bound via per-texture descriptor sets managed by `DescriptorManager`. Each unique texture index gets a pre-allocated descriptor set that is lazily initialized.

**Max Texture Count:**
- Soft limit of **64 textures** (the `maxTextures` default parameter in `DescriptorManager::create()`). This is the number of unique texture descriptor sets per frame. Exceeding this silently falls back to the per-frame base descriptor set.

**Texture Arrays/Atlases:**
- **No support.** All images are single 2D images with 1 mip level, 1 array layer. No texture arrays, no cube maps, no mipmap generation.
- Image format support: TGA (8/24/32-bit uncompressed) and DDS (uncompressed + BC1/BC2/BC3 compressed). BC formats are passed through directly to the GPU.

**Key Limitation for Terrain:**
Terrain typically needs texture arrays (for splatmapping multiple terrain layers) or large atlases. The current 64-texture-per-frame limit and individual sampler/descriptor approach would be inadequate. Mipmap generation is missing (only 1 mip level created), which will cause severe aliasing on terrain textures viewed at oblique angles. The `maxMipLevels` in the sampler is set to 0.0f.

---

### 6. Scene Class (`src/lib/scene/scene.hpp` / `scene.cpp`)

**Current State:**
- Extremely minimal: a flat list of `gfx::IRenderable*` pointers (raw, non-owning). Provides `addRenderable()`, `removeRenderable()`, `clear()`, and a `renderables()` accessor.
- `IRenderable` interface (in `renderable.hpp`): `draw(cmd)`, `bounds()`, `typeName()`, `isValid()`. Very basic -- no transform, no material, no parent-child relationships.
- **The Scene class is not actually used by the Renderer.** The `Renderer::recordCommandBuffer()` draws directly from `FrameContext` which holds `RenderableMesh&` and `HLodModel&`. The Scene exists but appears to be unused infrastructure.

**Key Limitation for Terrain:**
The Scene class would need a complete overhaul for a scene graph: hierarchical transforms, render order management, spatial partitioning (quadtree for terrain), frustum culling, and material associations. The `IRenderable` interface lacks transform and material binding, so terrain nodes could not be properly positioned or textured through it.

---

### 7. CMakeLists.txt Build Organization

**Main Application:**
- Single executable target `VulkanW3DViewer` built from `file(GLOB_RECURSE SOURCES "src/*.cpp")` -- all source files are globbed into one target.
- **No library target exists.** Everything is compiled directly into the executable.
- Dependencies: Vulkan SDK, GLFW (submodule), GLM (submodule), BigXtractor (submodule), ImGui (source files), CLI11 (header-only), nlohmann/json (header-only).
- Vulkan headers come from `lib/Vulkan-Hpp` submodule (not system Vulkan SDK headers).

**Testing:**
- Tests are conditionally built via `BUILD_TESTING` option. When enabled, the main application is **not built** (the `if(NOT BUILD_TESTING)` guard on line 30).
- Tests manually list source files they need (e.g., `src/lib/formats/w3d/loader.cpp`), using stub headers to avoid Vulkan dependencies. This is fragile.

**Shader Compilation:**
- Shaders are compiled with `glslc` and embedded into a generated C++ header via `cmake/EmbedShaders.cmake`. Shaders are embedded as byte arrays at compile time.

**Key Limitation for Terrain:**
To extract a library target (e.g., `w3d_lib`), you would need to:
1. Split sources into library vs. application sources (currently all globbed together).
2. Create a static/shared library target for the rendering/parsing code.
3. Make the main executable link against this library.
4. Tests would also link against this library instead of manually listing source files.
5. The `if(NOT BUILD_TESTING)` exclusion pattern would need removal so both the library and tests can build together.

---

## Summary of Architectural Limitations for Terrain Rendering

| Concern | Current State | Impact | Effort to Fix |
|---|---|---|---|
| **Pipeline flexibility** | Hardcoded vertex formats, duplicated creation code, TriangleList only | Cannot create terrain pipeline without code duplication or refactoring | Medium -- refactor to accept vertex format + topology as parameters |
| **Render pass** | Single VkRenderPass, single subpass, no abstraction | Fine for basic terrain; blocks shadow maps or depth pre-pass | Medium-High for multi-pass |
| **Draw call organization** | Individual `drawIndexed()` per mesh, no batching | Terrain with many chunks will have high CPU overhead | Medium -- add indirect draw or batching |
| **Dynamic vertex buffers** | None -- all vertex/index buffers are immutable device-local | Cannot update terrain mesh for LOD transitions or streaming | High -- need new `DynamicBuffer` class or ring buffer |
| **Memory allocation** | Per-buffer `vkAllocateMemory`, no VMA | Will hit allocation limits with many terrain chunks | High -- integrate VMA |
| **Texture arrays** | Not supported, 64-texture limit, no mipmaps | Cannot do terrain splatmapping efficiently | Medium -- add texture array support |
| **Scene graph** | Flat, unused, no transforms | Cannot organize terrain + objects hierarchically | High -- full scene graph needed |
| **Build system** | Monolithic executable, no library target | Cannot share code between terrain extension and existing tests | Medium -- extract library target |
| **MSAA** | Disabled (sample count always e1) | Terrain edges will alias | Low-Medium |
| **Descriptor system** | Pre-allocated fixed pools, lazy init, per-texture sets | Works but inflexible for terrain's multi-texture needs | Medium |

**Most Critical Blockers (in priority order):**
1. **No dynamic vertex/index buffers** -- terrain LOD absolutely requires updatable geometry.
2. **Hardcoded vertex format** -- terrain needs a different vertex layout than the existing `Vertex`/`SkinnedVertex` structs.
3. **No texture array support and no mipmaps** -- terrain splatmapping and distance rendering quality depend on these.
4. **Per-buffer memory allocation** -- terrain chunk count will exceed Vulkan allocation limits without VMA.
</task_result>
```

**Tool: task**

**Input:**
```json
{
  "description": "Study original terrain parser code",
  "prompt": "I need to deeply understand the original SAGE engine terrain parsing code in the GeneralsGameCode reference at C:\\CodeProjects\\VulkanW3DViewer\\lib\\GeneralsGameCode. Please examine and report on:\n\n1. **WorldHeightMap.cpp** (find the file under `Core/GameEngineDevice/Source/W3DDevice/GameClient/WorldHeightMap.cpp`): \n   - How does `parseHeightMapDataChunk()` work?\n   - How does `parseBlendTileDataChunk()` work?\n   - What is the DataChunk format? How are chunks identified and read?\n   - What's the exact binary format of each chunk?\n\n2. **DataChunkInput** format: Find the DataChunk reader classes. How do they work? Is there a chunk name lookup? What's the binary framing?\n\n3. **MapObject parsing** (ObjectsList chunk): How are placed objects read from the map? What data does each object contain (position, rotation, model name, etc.)?\n\n4. **PolygonTriggers parsing**: How are water areas and rivers defined? What geometry data is stored?\n\n5. **Global Lighting**: How is the lighting data structured in the binary format?\n\n6. **MapReaderWriterInfo.h**: What version constants exist? List them all.\n\nFocus on the exact binary format details needed to write a parser. Return the byte-level format specifications for each chunk type.",
  "subagent_type": "explore"
}
```

**Output:**
```
task_id: ses_384b1c768ffezLXXKTdw2QS1za (for resuming to continue this task if needed)

<task_result>
Now I have all the information needed. Here is the comprehensive report:

---

# SAGE Engine Map File Binary Format Specification

## Files Examined

- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Source\W3DDevice\GameClient\WorldHeightMap.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Generals\Code\GameEngine\Include\Common\DataChunk.h`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Generals\Code\GameEngine\Source\Common\System\DataChunk.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Generals\Code\GameEngine\Include\Common\MapReaderWriterInfo.h`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\GeneralsMD\Code\GameEngine\Include\Common\MapReaderWriterInfo.h`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngine\Include\Common\MapObject.h`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Generals\Code\GameEngine\Include\GameLogic\PolygonTrigger.h`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Generals\Code\GameEngine\Source\GameLogic\Map\PolygonTrigger.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Generals\Code\Tools\WorldBuilder\src\WHeightMapEdit.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\WorldHeightMap.h`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngineDevice\Include\W3DDevice\GameClient\TileData.h`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Generals\Code\GameEngine\Source\GameLogic\Map\SidesList.cpp`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Core\GameEngine\Include\Common\GameType.h`
- `C:\CodeProjects\VulkanW3DViewer\lib\GeneralsGameCode\Generals\Code\GameEngine\Include\Common\GlobalData.h`

---

## 1. DataChunk Container Format (the outermost framing)

All `.map` files use the DataChunk system. The file-level binary layout is:

### File Header: Table of Contents (TOC)

```
OFFSET  SIZE    DESCRIPTION
0x00    4       Magic tag: 'C' 'k' 'M' 'p' (bytes 0x43 0x6B 0x4D 0x70)
0x04    4       Int32 count -- number of entries in the symbol table
```

Then `count` entries, each:

```
1       uint8   len -- length of name string (NOT null-terminated)
len     bytes   name string (ASCII, no null terminator)
4       uint32  id -- numeric ID mapped to this name
```

The TOC maps symbolic chunk names (e.g. `"HeightMapData"`, `"BlendTileData"`, `"Object"`) and also Dict key names (e.g. `"originalOwner"`, `"objectInitialHealth"`) to integer IDs. IDs start at 1 and are allocated sequentially. The same ID space is shared by chunk names AND Dict key names.

### Chunk Header (each chunk, immediately after TOC for the first)

```
OFFSET  SIZE    DESCRIPTION
0x00    4       uint32 chunkID -- maps to name via TOC
0x04    2       uint16 version -- DataChunkVersionType
0x06    4       int32  dataSize -- byte count of chunk PAYLOAD (excludes this 10-byte header)
0x0A    ...     payload bytes (dataSize bytes)
```

So each chunk header is exactly **10 bytes** (4 + 2 + 4). The constant `CHUNK_HEADER_BYTES = 4` in the header is misleading -- it is defined as "2 shorts in chunk file header" which apparently counts the ID as 2 shorts (4 bytes), but the actual read sequence is: 4 (ID) + 2 (version) + 4 (dataSize) = 10 bytes.

Chunks can be **nested**. A parent chunk's `dataSize` includes the full bytes of all child chunks (headers + payloads). When reading child data, `decrementDataLeft()` traverses the entire chunk stack, decrementing `dataLeft` for all ancestors.

### Primitive Read Operations

| Method | Wire format | Size |
|--------|------------|------|
| `readInt()` | little-endian int32 | 4 bytes |
| `readReal()` | little-endian IEEE 754 float32 | 4 bytes |
| `readByte()` | single byte (signed) | 1 byte |
| `readAsciiString()` | uint16 length + `length` bytes of ASCII (no null) | 2 + len |
| `readUnicodeString()` | uint16 charCount + `charCount * 2` bytes of UTF-16LE (no null) | 2 + len*2 |
| `readArrayOfBytes(ptr, n)` | raw n bytes | n |
| `readDict()` | See Dict format below | variable |

### Dict Binary Format

```
2       uint16  pairCount -- number of key-value pairs
```

Then for each pair:

```
4       int32   keyAndType
          - low 8 bits: DataType enum (0=BOOL, 1=INT, 2=REAL, 3=ASCIISTRING, 4=UNICODESTRING)
          - upper 24 bits (>>8): TOC id for the key name
...     value   depends on type:
          DICT_BOOL (0):        1 byte (0 or 1)
          DICT_INT (1):         4 bytes int32
          DICT_REAL (2):        4 bytes float32
          DICT_ASCIISTRING (3): uint16 len + len bytes
          DICT_UNICODESTRING (4): uint16 charLen + charLen*2 bytes
```

### Chunk Parse Registration and Dispatch

`DataChunkInput::parse()` loops: while not at end of file (or end of parent chunk), it opens the next chunk, looks up the name from the TOC, then iterates through registered parsers looking for a match on `(label, parentLabel)`. The parser callback receives a `DataChunkInfo` with `label`, `parentLabel`, `version`, and `dataSize`. After the callback returns, `closeDataChunk()` seeks past any unread bytes.

---

## 2. Top-Level Chunk Order (as written by WHeightMapEdit::saveToFile)

The map file contains these chunks at the top level, in this order:

1. **`"HeightMapData"`** -- version K_HEIGHT_MAP_VERSION_4 (4)
2. **`"BlendTileData"`** -- version K_BLEND_TILE_VERSION_7 (7) or _8 (8)
3. **`"WorldInfo"`** -- version K_WORLDDICT_VERSION_1 (1)
4. **`"SidesList"`** -- version K_SIDES_DATA_VERSION_3 (3), contains nested `"PlayerScriptsList"` sub-chunks
5. **`"ObjectsList"`** -- version K_OBJECTS_VERSION_3 (3), contains nested `"Object"` sub-chunks
6. **`"PolygonTriggers"`** -- version K_TRIGGERS_VERSION_3 (3) or _4 (4)
7. **`"GlobalLighting"`** -- version K_LIGHTING_VERSION_3 (3)

---

## 3. HeightMapData Chunk (version 4)

Parsed by `WorldHeightMap::ParseHeightMapData()`. Writer at `WHeightMapEdit.cpp` line 612.

```
4       int32   width           -- number of height samples in X
4       int32   height          -- number of height samples in Y
4       int32   borderSize      -- non-playable border width (v3+)
4       int32   numBoundaries   -- count of boundary rects (v4+)
  for each boundary (numBoundaries iterations):
    4   int32   boundary[i].x   -- playable area width (top-right X)
    4   int32   boundary[i].y   -- playable area height (top-right Y)
4       int32   dataSize        -- total bytes of height data (must == width * height)
dataSize bytes  heightData      -- array of uint8 height values, row-major [y * width + x]
```

Each height value is an unsigned byte [0..255]. World height = `value * MAP_HEIGHT_SCALE` where `MAP_HEIGHT_SCALE = MAP_XY_FACTOR / 16.0 = 10.0 / 16.0 = 0.625`. Each cell is `MAP_XY_FACTOR = 10.0` world units wide/tall.

For **version 1** maps: data was at 5.0 spacing and gets downsampled by 2x after reading.
For **version 2**: 10.0 spacing, no borderSize.
For **version 3**: added borderSize.
For **version 4**: added multiple boundaries.

If version < 4, a single boundary is synthesized as `(width - 2*borderSize, height - 2*borderSize)`.

---

## 4. BlendTileData Chunk (version 7)

Parsed by `WorldHeightMap::ParseBlendTileData()`. Writer at `WHeightMapEdit.cpp` line 636.

```
4       int32   dataSize        -- must match heightmap dataSize (width * height)

-- Arrays, each dataSize * sizeof(int16) = dataSize * 2 bytes:
dataSize*2  int16[]  tileNdxes            -- terrain texture class index per cell
dataSize*2  int16[]  blendTileNdxes       -- primary blend tile index (0 = no blend)
dataSize*2  int16[]  extraBlendTileNdxes  -- secondary blend layer (v6+, for 3-way blends)
dataSize*2  int16[]  cliffInfoNdxes       -- cliff UV info index (v5+, 0 = no cliff)

-- Cliff passability bitmap (v7+):
height * flipStateWidth  bytes  cellCliffState  -- bitfield, 1 bit per cell
                                                -- flipStateWidth = (width + 7) / 8

-- Scalar counts:
4       int32   numBitmapTiles       -- number of source bitmap tiles
4       int32   numBlendedTiles      -- number of blend tile info records (index 0 is implicit "none")
4       int32   numCliffInfo         -- number of cliff info records (v5+, index 0 is default)

-- Texture classes:
4       int32   numTextureClasses
  for each textureClass (numTextureClasses iterations):
    4   int32   firstTile       -- index of first tile in source tile array
    4   int32   numTiles        -- number of tiles in this class
    4   int32   width           -- tile grid width (e.g. 2 means 2x2 = 4 tiles)
    4   int32   legacy          -- (unused, always 0; formerly isGDF flag)
    2+N string  name            -- AsciiString: terrain type name (e.g. "GrassLight")

-- Edge texture classes (v4+):
4       int32   numEdgeTiles
4       int32   numEdgeTextureClasses
  for each edgeTextureClass:
    4   int32   firstTile
    4   int32   numTiles
    4   int32   width
    2+N string  name            -- AsciiString

-- Blend tile info array (for indices 1..numBlendedTiles-1):
  for each blendedTile (i = 1 to numBlendedTiles-1):
    4   int32   blendNdx        -- source texture index for blend
    1   byte    horiz           -- horizontal blend flag
    1   byte    vert            -- vertical blend flag
    1   byte    rightDiagonal   -- right diagonal blend flag
    1   byte    leftDiagonal    -- left diagonal blend flag
    1   byte    inverted        -- bit 0 = inverted, bit 1 = flipped (FLIPPED_MASK = 0x2)
    1   byte    longDiagonal    -- long diagonal flag (v3+)
    4   int32   customBlendEdgeClass -- custom edge class index, -1 = use alpha (v4+)
    4   int32   flag            -- sentinel: must be 0x7ADA0000 (FLAG_VAL)

-- Cliff info array (v5+, for indices 1..numCliffInfo-1):
  for each cliffInfo (i = 1 to numCliffInfo-1):
    4   int32   tileIndex       -- which tile texture
    4   float32 u0              -- upper-left U
    4   float32 v0              -- upper-left V
    4   float32 u1              -- lower-left U
    4   float32 v1              -- lower-left V
    4   float32 u2              -- lower-right U
    4   float32 v2              -- lower-right V
    4   float32 u3              -- upper-right U
    4   float32 v3              -- upper-right V
    1   byte    flip            -- flip flag
    1   byte    mutant          -- mutant mapping flag
```

**Key version differences:**
- v1/v2: No blend extras, no cliff info, no edges
- v3: Added `longDiagonal` byte per blend tile
- v4: Added `customBlendEdgeClass` int per blend tile + edge texture classes section
- v5: Added `cliffInfoNdxes` array + `numCliffInfo` + cliff info records
- v6: Added `extraBlendTileNdxes` array (3-way terrain blending)
- v7: Added `cellCliffState` bitfield for painted passability. NOTE: v7 had a bug where `byteWidth = (width+1)/8` instead of `(width+7)/8` -- the reader compensates.
- v8 (Zero Hour only): Same as v7 with corrected cliff state width.

---

## 5. ObjectsList / Object Chunks (version 3)

The `"ObjectsList"` chunk is a parent that contains nested `"Object"` sub-chunks. Parser registration chain:

```
registerParser("ObjectsList", "", ParseObjectsDataChunk)
  -> internally registers: registerParser("Object", "ObjectsList", ParseObjectDataChunk)
```

### Each "Object" sub-chunk (version 3):

```
4       float32 loc.x           -- X position in world coordinates
4       float32 loc.y           -- Y position in world coordinates
4       float32 loc.z           -- Z position (v3+; in v1-v2, Z is forced to 0)
4       float32 angle           -- rotation angle in radians, counterclockwise from +X
4       int32   flags           -- bit flags:
                                    0x001 = FLAG_DRAWS_IN_MIRROR
                                    0x002 = FLAG_ROAD_POINT1
                                    0x004 = FLAG_ROAD_POINT2
                                    0x008 = FLAG_ROAD_CORNER_ANGLED
                                    0x010 = FLAG_BRIDGE_POINT1
                                    0x020 = FLAG_BRIDGE_POINT2
                                    0x040 = FLAG_ROAD_CORNER_TIGHT
                                    0x080 = FLAG_ROAD_JOIN
                                    0x100 = FLAG_DONT_RENDER
2+N     string  name            -- AsciiString: thing template name (e.g. "GLA/GLAVehicleTechnical")
...     Dict    properties      -- key-value property Dict (v2+)
```

The Dict typically contains keys like:
- `originalOwner` (AsciiString) -- team ownership
- `uniqueID` (AsciiString) -- e.g. "GLAVehicleTechnical 42"
- `objectInitialHealth` (Int) -- default 100
- `objectEnabled` (Bool) -- default true
- `objectIndestructible` (Bool)
- `objectUnsellable` (Bool)
- `objectPowered` (Bool)
- `objectRecruitableAI` (Bool)
- `objectTargetable` (Bool)
- `waypointID` (Int) -- present if object is a waypoint
- `waypointName` (AsciiString) -- present if object is a waypoint
- `lightHeightAboveTerrain` (Real) -- present if object is a light
- `scorchType` (Int) -- present if object is a scorch mark

All these key names are stored as string-to-ID mappings in the file's TOC.

---

## 6. PolygonTriggers Chunk (version 3)

Parsed by `PolygonTrigger::ParsePolygonTriggersDataChunk()`. Writer at `PolygonTrigger.cpp` line 225.

```
4       int32   count           -- number of polygon triggers

for each trigger (count iterations):
  2+N   string  triggerName     -- AsciiString: name of the trigger area
  4     int32   triggerID       -- unique integer ID
  1     byte    isWaterArea     -- (v2+) 1 if this polygon defines a water surface
  1     byte    isRiver         -- (v3+) 1 if this water area is a river
  4     int32   riverStart      -- (v3+) index of the starting point for river flow
  4     int32   numPoints       -- number of polygon vertices

  for each point (numPoints iterations):
    4   int32   x               -- X coordinate (integer, in world units)
    4   int32   y               -- Y coordinate (integer, in world units)
    4   int32   z               -- Z coordinate / water surface height
```

Water areas are polygon triggers with `isWaterArea = true`. The Z coordinate of the polygon points defines the water surface height. Rivers (`isRiver = true`) use `riverStart` to identify which vertex index the river flows from.

Version 4 (Zero Hour) adds a layer name field (not present in the Generals codebase examined).

---

## 7. GlobalLighting Chunk (version 3)

Parsed by `WorldHeightMap::ParseLightingDataChunk()`. Writer at `WHeightMapEdit.cpp` line 757.

There are 4 time-of-day slots corresponding to the `TimeOfDay` enum:
- Index 0 = Morning (TIME_OF_DAY_FIRST = 1, so array index i+1)
- Index 1 = Afternoon
- Index 2 = Evening
- Index 3 = Night

`MAX_GLOBAL_LIGHTS = 3` (lights 0, 1, 2).

```
4       int32   timeOfDay       -- current time of day enum value (0=invalid, 1=morning, 2=afternoon, 3=evening, 4=night)

-- For i = 0..3 (4 time-of-day slots), EACH slot contains:

  -- Terrain light [0] (9 floats):
  4     float32 terrain[0].ambient.red
  4     float32 terrain[0].ambient.green
  4     float32 terrain[0].ambient.blue
  4     float32 terrain[0].diffuse.red
  4     float32 terrain[0].diffuse.green
  4     float32 terrain[0].diffuse.blue
  4     float32 terrain[0].lightPos.x
  4     float32 terrain[0].lightPos.y
  4     float32 terrain[0].lightPos.z

  -- Object light [0] (9 floats):
  4     float32 objects[0].ambient.red
  4     float32 objects[0].ambient.green
  4     float32 objects[0].ambient.blue
  4     float32 objects[0].diffuse.red
  4     float32 objects[0].diffuse.green
  4     float32 objects[0].diffuse.blue
  4     float32 objects[0].lightPos.x
  4     float32 objects[0].lightPos.y
  4     float32 objects[0].lightPos.z

  -- (v2+) Additional object lights [1] and [2] (2 * 9 = 18 floats):
  for j = 1..2:
    4   float32 objects[j].ambient.red
    4   float32 objects[j].ambient.green
    4   float32 objects[j].ambient.blue
    4   float32 objects[j].diffuse.red
    4   float32 objects[j].diffuse.green
    4   float32 objects[j].diffuse.blue
    4   float32 objects[j].lightPos.x
    4   float32 objects[j].lightPos.y
    4   float32 objects[j].lightPos.z

  -- (v3+) Additional terrain lights [1] and [2] (2 * 9 = 18 floats):
  for j = 1..2:
    4   float32 terrain[j].ambient.red
    4   float32 terrain[j].ambient.green
    4   float32 terrain[j].ambient.blue
    4   float32 terrain[j].diffuse.red
    4   float32 terrain[j].diffuse.green
    4   float32 terrain[j].diffuse.blue
    4   float32 terrain[j].lightPos.x
    4   float32 terrain[j].lightPos.y
    4   float32 terrain[j].lightPos.z

-- After the 4 time-of-day blocks, if data remains:
4       uint32  shadowColor     -- ARGB shadow color (optional, read if !atEndOfChunk)
```

**Total per time-of-day slot at v3**: 9 + 9 + 18 + 18 = 54 floats = 216 bytes.
**Total lighting payload at v3**: 4 (timeOfDay) + 4 * 216 + 4 (shadowColor) = 872 bytes.

The `lightPos` is a direction vector (not a position), pointing from the light toward the scene. Default initialization is `{0, 0, -1}`.

---

## 8. WorldInfo Chunk (version 1)

Very simple -- just a Dict:

```
...     Dict    worldDict       -- contains world-level properties
```

Known key: `weather` (Int) -- maps to the Weather enum (0 = WEATHER_NORMAL).

---

## 9. SidesList Chunk (version 3)

```
4       int32   numSides        -- number of player sides

for each side (numSides iterations):
  ...   Dict    sideDict        -- side properties (playerName, etc.)
  4     int32   buildListCount  -- number of build list entries

  for each buildList entry:
    2+N string  buildingName    -- AsciiString
    2+N string  templateName    -- AsciiString
    4   float32 loc.x
    4   float32 loc.y
    4   float32 loc.z
    4   float32 angle
    1   byte    initiallyBuilt
    4   int32   numRebuilds
    -- (v3+):
    2+N string  script          -- AsciiString
    4   int32   health
    1   byte    isWhiner
    1   byte    isUnsellable
    1   byte    isRepairable

-- (v2+):
4       int32   numTeams
for each team:
  ...   Dict    teamDict        -- team properties (teamName, etc.)

-- Then a nested "PlayerScriptsList" sub-chunk follows within this chunk.
```

---

## 10. Version Constants (MapReaderWriterInfo.h)

### Generals version (Generals/Code):

| Constant | Value | Notes |
|----------|-------|-------|
| `K_HEIGHT_MAP_VERSION_1` | 1 | Cell = 5.0 spacing |
| `K_HEIGHT_MAP_VERSION_2` | 2 | Cell = 10.0 spacing |
| `K_HEIGHT_MAP_VERSION_3` | 3 | Added borderSize |
| `K_HEIGHT_MAP_VERSION_4` | 4 | Multiple boundaries |
| `K_BLEND_TILE_VERSION_1` | 1 | Cell = 5.0 |
| `K_BLEND_TILE_VERSION_2` | 2 | Cell = 10.0 |
| `K_BLEND_TILE_VERSION_3` | 3 | Long diagonal blends |
| `K_BLEND_TILE_VERSION_4` | 4 | Custom edge blends |
| `K_BLEND_TILE_VERSION_5` | 5 | Cliff UV coordinates |
| `K_BLEND_TILE_VERSION_6` | 6 | Extra blend layer (3-way) |
| `K_BLEND_TILE_VERSION_7` | 7 | Painted passability flags |
| `K_OBJECTS_VERSION_1` | 1 | No Dict |
| `K_OBJECTS_VERSION_2` | 2 | Includes Dict |
| `K_OBJECTS_VERSION_3` | 3 | Includes Dict (+ Z coord) |
| `K_MAP_OBJECT_VERSION_1` | 1 | |
| `K_WAYPOINTS_VERSION_1` | 1 | |
| `K_PLAYERLIST_VERSION_1` | 1 | |
| `K_TRIGGERS_VERSION_1` | 1 | Basic polygon triggers |
| `K_TRIGGERS_VERSION_2` | 2 | Added isWaterArea |
| `K_TRIGGERS_VERSION_3` | 3 | Added isRiver & riverStart |
| `K_LIGHTING_VERSION_1` | 1 | Single terrain + object light |
| `K_LIGHTING_VERSION_2` | 2 | +2 additional object lights |
| `K_LIGHTING_VERSION_3` | 3 | +2 additional terrain lights |
| `K_WORLDDICT_VERSION_1` | 1 | |
| `K_MAPPREVIEW_VERSION_1` | 1 | |

### Zero Hour additions (GeneralsMD/Code):

| Constant | Value | Notes |
|----------|-------|-------|
| `K_BLEND_TILE_VERSION_8` | 8 | Fixed passability flag width |
| `K_TRIGGERS_VERSION_4` | 4 | Added layer name |

### SidesList versions (defined in SidesList.cpp, not the header):

| Constant | Value | Notes |
|----------|-------|-------|
| `K_SIDES_DATA_VERSION_1` | 1 | Basic sides |
| `K_SIDES_DATA_VERSION_2` | 2 | Includes team list |
| `K_SIDES_DATA_VERSION_3` | 3 | Includes script, health, whiner, unsellable, repairable per build entry |

### Other constants:

| Constant | Value | Location |
|----------|-------|----------|
| `FLAG_VAL` | `0x7ADA0000` | `WorldHeightMap.h:56` -- blend tile sentinel |
| `MAP_XY_FACTOR` | `10.0f` | `MapObject.h:57` -- world units per cell |
| `MAP_HEIGHT_SCALE` | `0.625f` | `MapObject.h:58` -- `MAP_XY_FACTOR / 16.0` |
| `MAX_GLOBAL_LIGHTS` | `3` | `GlobalData.h:53` |
| `INVERTED_MASK` | `0x1` | `TileData.h:46` |
| `FLIPPED_MASK` | `0x2` | `TileData.h:47` |
| `TILE_PIXEL_EXTENT` | `64` | `TileData.h:48` |
| File magic | `'C' 'k' 'M' 'p'` | `DataChunk.cpp:504` -- TOC header tag |
</task_result>
```

---

## Assistant (Plan · claude-opus-4-6 · 112.3s)

Now I have all the information I need. Let me compile the comprehensive plan.

---

# Plan: Terrain & Map Rendering for VulkanW3DViewer

## Vision

Extend the VulkanW3DViewer project into a **full scene renderer and future map editor** for C&C Generals: Zero Hour. The work produces two deliverables:

1. **`w3d_lib`** -- a reusable static library containing all parsing and rendering code
2. **Map Viewer mode** -- a new mode in the application that loads `.map` files and renders complete scenes (terrain, water, objects, lighting)

The architecture is designed for mutability from day one, enabling a WorldBuilder-style editor in a future phase.

---

## Current State Summary

The codebase already has:
- W3D model parsing (meshes, hierarchies, animations, HLod)
- BIG archive support (loads `TerrainZH.big`, `MapsZH.big`, `W3DZH.big`, `TexturesZH.big`)
- Vulkan rendering pipeline (basic + skinned, double-buffered, ImGui UI)
- TGA/DDS texture loading

It does **not** have:
- Map file (`.map`) parsing -- uses a different "DataChunk" binary format from W3D
- Terrain heightmap rendering
- Terrain texture blending/splatmapping
- Water rendering
- INI file parsing (terrain type definitions, water settings)
- Dynamic vertex/index buffers (all GPU buffers are immutable)
- Texture arrays or mipmaps
- Scene graph or spatial partitioning
- RTS-style camera

---

## Phase Breakdown

### Phase 0: Architecture Refactoring (Foundation)
**Goal:** Extract the reusable library and address critical infrastructure gaps before terrain work begins.

| Task | Details |
|------|---------|
| **0.1 Extract `w3d_lib` static library** | Split `CMakeLists.txt` into a `w3d_lib` static library target (all of `src/lib/` + `src/render/`) and a thin `VulkanW3DViewer` executable target (just `src/main.cpp` + `src/core/` + `src/ui/`). Tests link against `w3d_lib` instead of listing individual source files. |
| **0.2 Integrate VMA** | Add [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) as a submodule. Replace all per-buffer `vkAllocateMemory` calls with VMA allocations. This is mandatory -- terrain chunks will exceed Vulkan's per-device allocation limit. |
| **0.3 Add `DynamicBuffer` class** | New buffer type using VMA with `VMA_MEMORY_USAGE_CPU_TO_GPU` (host-visible, device-local preferred). Supports `update(data, size, offset)` for streaming terrain geometry updates. Double-buffered like `UniformBuffer`. |
| **0.4 Add mipmap generation** | Implement `generateMipmaps()` using `vkCmdBlitImage` in `TextureManager`. Calculate mip levels from image dimensions. Terrain textures viewed at oblique angles will alias badly without this. |
| **0.5 Add texture array support** | Extend `TextureManager` to support creating `VkImage` with `arrayLayers > 1` for terrain texture atlases. Add `createTextureArray(names, width, height)` method. |
| **0.6 Refactor Pipeline creation** | Make `Pipeline::create()` accept a `PipelineCreateInfo` struct containing: vertex format description, topology, descriptor set layout bindings, push constant ranges. Eliminate the duplicated `createSkinned()` method. |
| **0.7 Add RTS camera** | New `RTSCamera` class alongside the existing orbit camera: WASD panning, mouse-edge scrolling, scroll-wheel zoom, Q/E rotation. Produces a view matrix with configurable pitch angle (default ~60 degrees, matching the original game). The terrain viewer will default to this camera. |

### Phase 1: Map File Parsing
**Goal:** Parse `.map` files into mutable in-memory data structures.

| Task | Details |
|------|---------|
| **1.1 DataChunk reader** | New `src/lib/formats/map/data_chunk_reader.hpp/cpp`. Reads the `CkMp` TOC header, resolves name-to-ID mapping, iterates chunks with name+version dispatch. This is independent from the W3D `ChunkReader` since the framing format is completely different (named chunks vs. numbered chunks). |
| **1.2 HeightMapData parser** | Parse the heightmap grid: width, height, borderSize, boundaries, raw `uint8_t[]` data. Store in `HeightMap` struct with mutable `std::vector<uint8_t>` (edit-ready). Support versions 1-4. |
| **1.3 BlendTileData parser** | Parse all tile arrays (`tileNdxes`, `blendTileNdxes`, `extraBlendTileNdxes`, `cliffInfoNdxes`), texture classes, edge classes, blend tile info records, cliff info records. Store in `BlendTileData` struct. Support versions 1-8. |
| **1.4 ObjectsList parser** | Parse `"ObjectsList"` -> nested `"Object"` sub-chunks. Each object: position (vec3), rotation angle, flags, template name, property Dict. Store as `std::vector<MapObject>`. |
| **1.5 PolygonTriggers parser** | Parse water areas and rivers: polygon vertices, trigger names/IDs, `isWaterArea`/`isRiver` flags. Store as `std::vector<PolygonTrigger>`. |
| **1.6 GlobalLighting parser** | Parse 4 time-of-day lighting sets (ambient/diffuse/direction for terrain and objects, up to 3 lights each). Store as `GlobalLighting` struct. |
| **1.7 WorldInfo & SidesList parsers** | Parse the Dict-based world info and sides/teams data. Lower priority but needed for completeness. |
| **1.8 Top-level MapFile loader** | `MapLoader::load(path)` -> dispatches to all chunk parsers -> returns `MapFile` struct containing all parsed data. Support loading from disk or from BIG archive extraction. |
| **1.9 Tests** | Comprehensive unit tests for DataChunk reader, each chunk parser, and the top-level loader. Use real `.map` files from `MapsZH.big` as test fixtures. |

### Phase 2: INI Parsing (Terrain Types & Water Settings)
**Goal:** Parse the INI files needed to resolve terrain texture names and water configuration.

| Task | Details |
|------|---------|
| **2.1 INI lexer/parser** | Custom parser for SAGE INI dialect in `src/lib/formats/ini/`. The format uses `block Name ... End` blocks with typed fields. Not standard INI -- it has nested blocks, enums, colors, etc. Focus on just the subset needed: `TerrainTexture.ini`, `Water.ini`. |
| **2.2 TerrainType definitions** | Parse `TerrainTexture.ini` from `TerrainZH.big` to build `name -> TGA filename` mapping. Each terrain type has: `Texture`, `BlendEdgeTexture`, `Class`, `RestrictConstruction`. |
| **2.3 Water settings** | Parse `Water.ini` for: water textures, transparency settings, standing water color, scroll rates, sky textures. Store as `WaterSettings` struct. |
| **2.4 Tests** | Unit tests for INI parser with sample INI snippets. |

### Phase 3: Terrain Rendering
**Goal:** Render the heightmap with textured, blended terrain.

| Task | Details |
|------|---------|
| **3.1 Terrain mesh generation** | Convert heightmap to triangle mesh. Each cell = 2 triangles (quad). Diagonal flip follows the original engine logic (minimize height difference across diagonal). Per-vertex data: position (from height * 0.625), UV coords (2 sets: base tile + blend tile). Use the `DynamicBuffer` from Phase 0 since terrain mesh will need updates for future editing. Subdivide into chunks (~32x32 cells each, matching original) for frustum culling. |
| **3.2 Terrain texture atlas** | Build runtime texture atlas from tile TGAs: load terrain type TGAs from `TerrainZH.big`, split into 64x64 tiles, arrange into a 2048-wide texture array. Generate UV lookup table mapping `tileNdxes` values to atlas coordinates. Use the texture array support from Phase 0.5. |
| **3.3 Terrain blend system** | Implement the alpha blending between terrain types. The original uses 12 alpha gradient patterns (horizontal, vertical, 4 diagonals x inverted). Generate these as a small texture array. Render blends as a second pass or use a fragment shader that samples base tile + blend tile + alpha mask. |
| **3.4 Terrain shaders** | New `terrain.vert` + `terrain.frag`. Vertex shader: sample terrain position (or pass through), output 2 UV sets. Fragment shader: sample base texture from atlas, sample blend texture + alpha mask, combine. Accept lighting push constants. |
| **3.5 Terrain pipeline** | Create a new Vulkan graphics pipeline for terrain using the refactored `Pipeline::create()`. Different vertex format (position + 2 UVs + normal), same render pass. |
| **3.6 Cliff texture mapping** | For cells with `cliffInfoNdxes != 0`, use the custom UV coordinates from `TCliffInfo` instead of the standard tile UVs. Pass cliff UV overrides via vertex attributes or a small SSBO. |
| **3.7 Frustum culling** | Implement view frustum culling for terrain chunks. Each 32x32 chunk has an AABB from its min/max heights. Skip chunks outside the view frustum. |
| **3.8 Terrain `IRenderable`** | New `TerrainRenderable` class implementing `IRenderable`. Owns the terrain mesh chunks, texture atlas, and blend data. `draw()` iterates visible chunks and issues draw calls. |

### Phase 4: Water Rendering
**Goal:** Render water surfaces defined by polygon triggers.

| Task | Details |
|------|---------|
| **4.1 Water mesh generation** | For each water polygon trigger, generate a flat mesh at the specified Z height. Triangulate the polygon (ear clipping or similar). |
| **4.2 Water shader** | New `water.vert` + `water.frag`. Scrolling UV animation (2 texture layers at different scroll rates). Semi-transparent with configurable opacity based on `TransparentWaterDepth` / `TransparentWaterMinOpacity`. |
| **4.3 Water pipeline** | New pipeline with alpha blending enabled, depth write disabled (or configurable). Rendered after terrain. |
| **4.4 Shoreline blending** | Where terrain meets water, blend the terrain edge using destination alpha. This creates a smooth water-terrain transition rather than a hard line. |

### Phase 5: Object Placement & Scene Graph
**Goal:** Place W3D objects on the terrain and manage the full scene.

| Task | Details |
|------|---------|
| **5.1 Scene graph** | Replace the minimal `Scene` class with a proper scene graph. `SceneNode` base class with transform (position, rotation, scale), parent-child hierarchy, bounding volume. `TerrainNode`, `WaterNode`, `ObjectNode` as concrete types. Quadtree spatial index for efficient frustum culling of hundreds of objects. |
| **5.2 Object resolver** | Given a template name from `ObjectsList` (e.g., `"GLA/GLAVehicleTechnical"`), resolve it to a W3D model file via the INI `Thing` database or naming convention. Extract from BIG archives, parse W3D, create `HLodModel`. Cache parsed models (many objects share the same template). |
| **5.3 Object instancing** | For repeated objects (trees, props), use instanced rendering. Store per-instance transforms in an SSBO. Single draw call per unique model. |
| **5.4 Object placement** | For each `MapObject`, look up its template, instantiate the resolved W3D model at the specified position/rotation. Snap Z to terrain height if the map specifies Z=0 (some older maps). |
| **5.5 Roads & bridges** | Parse road/bridge point objects (identified by `FLAG_ROAD_POINT1/2`, `FLAG_BRIDGE_POINT1/2`). Generate road mesh geometry along spline paths. Lower priority -- can be deferred. |

### Phase 6: Lighting & Polish
**Goal:** Apply the map's lighting data and add visual polish.

| Task | Details |
|------|---------|
| **6.1 Time-of-day lighting** | Apply `GlobalLighting` data to terrain and object shaders. Support switching between the 4 time-of-day presets (Morning/Afternoon/Evening/Night). Pass light direction + ambient/diffuse colors as UBO data. |
| **6.2 Shadow color** | Apply the optional shadow color from `GlobalLighting`. |
| **6.3 Cloud shadows** | Animated scrolling cloud shadow texture overlaid on terrain (the original's pass 3). Uses a separate UV set that scrolls over time. |
| **6.4 Minimap/preview** | Generate and display a top-down minimap from the terrain data. Could reuse the `mapname.tga` preview or render one dynamically. |

### Phase 7: Map Viewer UI
**Goal:** Build the UI for the map viewing mode.

| Task | Details |
|------|---------|
| **7.1 Map browser** | New `MapBrowser` UI window. Scans `MapsZH.big` for available maps, displays with preview thumbnails. Also supports opening `.map` files from disk. |
| **7.2 Map info panel** | Display map metadata: dimensions, terrain types used, object count, player count/sides. |
| **7.3 Object list panel** | Searchable/filterable list of all placed objects. Click to select and fly camera to object. |
| **7.4 Time-of-day selector** | Dropdown or slider to switch between Morning/Afternoon/Evening/Night lighting. |
| **7.5 Layer toggles** | Checkboxes to show/hide: terrain, water, objects (by type), roads, triggers. |
| **7.6 Mode switching** | Clean switching between "Model Viewer" mode (existing) and "Map Viewer" mode (new). Shared Vulkan context, different UI layouts and render paths. |

---

## Key Data Structures (New)

```
// Map file top-level
struct MapFile {
  HeightMap heightMap;
  BlendTileData blendTiles;
  std::vector<MapObject> objects;
  std::vector<PolygonTrigger> triggers;
  GlobalLighting lighting;
  WorldInfo worldInfo;
  SidesList sides;
};

// Heightmap (mutable for future editing)
struct HeightMap {
  int32_t width, height, borderSize;
  std::vector<ICoord2D> boundaries;
  std::vector<uint8_t> data;  // row-major [y * width + x]
  float getWorldHeight(int x, int y) const;
  void setHeight(int x, int y, uint8_t value);  // edit-ready
};

// Terrain object placed in map
struct MapObject {
  glm::vec3 position;
  float angle;
  uint32_t flags;
  std::string templateName;
  Dict properties;
};

// Water/trigger polygon
struct PolygonTrigger {
  std::string name;
  int32_t id;
  bool isWaterArea, isRiver;
  int32_t riverStart;
  std::vector<glm::ivec3> points;
};
```

---

## File Organization (New Directories)

```
src/lib/formats/map/          -- Map file parsing
  data_chunk_reader.hpp/cpp   -- DataChunk binary format reader
  map_loader.hpp/cpp          -- Top-level .map file loader
  heightmap_parser.hpp/cpp
  blend_tile_parser.hpp/cpp
  objects_parser.hpp/cpp
  triggers_parser.hpp/cpp
  lighting_parser.hpp/cpp
  types.hpp                   -- MapFile, HeightMap, BlendTileData, etc.

src/lib/formats/ini/          -- SAGE INI parsing
  ini_parser.hpp/cpp
  terrain_types.hpp/cpp
  water_settings.hpp/cpp

src/lib/gfx/
  dynamic_buffer.hpp/cpp      -- New: streaming vertex/index buffers
  rts_camera.hpp/cpp          -- New: RTS-style camera
  texture_array.hpp/cpp       -- New: texture array support (or extend texture.hpp)

src/render/terrain/           -- Terrain rendering
  terrain_mesh.hpp/cpp        -- Heightmap -> triangle mesh conversion
  terrain_atlas.hpp/cpp       -- Texture atlas builder
  terrain_blend.hpp/cpp       -- Blend alpha generation
  terrain_renderable.hpp/cpp  -- IRenderable for terrain

src/render/water/             -- Water rendering
  water_mesh.hpp/cpp
  water_renderable.hpp/cpp

src/lib/scene/                -- Enhanced scene management
  scene_node.hpp/cpp          -- Base scene node with transform
  scene_graph.hpp/cpp         -- Full scene graph with spatial index
  quadtree.hpp/cpp            -- Spatial partitioning

shaders/
  terrain.vert / terrain.frag
  water.vert / water.frag

tests/map/                    -- Map parsing tests
tests/ini/                    -- INI parsing tests
tests/terrain/                -- Terrain rendering logic tests
```

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| VMA integration breaks existing rendering | Medium | High | Incremental: wrap VMA behind existing `Buffer` interface, run existing tests after each change |
| Blend tile system is more complex than documented | High | Medium | The original source code is available in `GeneralsGameCode` as reference. Start with simple cases (no blending), add layers incrementally. |
| Terrain texture atlas exceeds GPU limits | Low | Medium | 2048x2048 atlas is well within limits. Can use multiple atlases if needed. |
| Performance with full scene (terrain + hundreds of objects) | Medium | Medium | Frustum culling, instanced rendering, and LOD mitigate this. Profile early. |
| Map format variations across different community maps | Medium | Low | Test with multiple maps from `MapsZH.big`. The format is well-documented via the original source. |

---

## Suggested Implementation Order

I recommend starting in this sequence, with each phase building on the previous:

1. **Phase 0** (foundation) -- especially 0.1 library extraction, 0.2 VMA, 0.6 pipeline refactor
2. **Phase 1** (parsing) -- can partially overlap with Phase 0 since parsing is Vulkan-independent
3. **Phase 2** (INI) -- needed before terrain texturing
4. **Phase 3** (terrain rendering) -- the core deliverable
5. **Phase 4** (water) -- natural extension of terrain
6. **Phase 7** (UI) -- can start early for the map browser, but full UI depends on rendering
7. **Phase 5** (objects) -- builds on existing W3D model loading
8. **Phase 6** (lighting/polish) -- final visual quality pass

Phases 1 and 2 (parsing) produce testable, Vulkan-free code and are good candidates for parallel development alongside the Phase 0 infrastructure work.

---

Does this plan align with your vision? I'm happy to dive deeper into any specific phase, adjust priorities, or break down individual tasks further. A few things I'd especially like your input on:

1. **VMA integration** -- This is the most invasive Phase 0 change. It touches every buffer and texture allocation. Are you comfortable with that scope?
2. **Blend tile complexity** -- The 3-pass blending system (base + alpha blend + extra blend + cliff overrides) is the most complex part of terrain rendering. Would you prefer to ship a simpler version first (just base tiles, no blending) and iterate?
3. **Phase ordering** -- Would you prefer to see terrain on screen faster (start Phase 3 with hardcoded textures, skip INI parsing initially) or do it properly end-to-end?

---

