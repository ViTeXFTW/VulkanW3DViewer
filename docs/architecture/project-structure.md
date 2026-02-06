# Project Structure

This page details the complete directory structure of VulkanW3DViewer.

## Root Directory

```
VulkanW3DViewer/
├── .github/              # GitHub configuration
├── cmake/                # CMake modules
├── docs/                 # Documentation (you are here)
├── lib/                  # Git submodules (dependencies)
├── scripts/              # Build scripts
├── shaders/              # GLSL shaders
├── src/                  # Source code
├── tests/                # Test suite
├── CMakeLists.txt        # Main CMake configuration
├── CMakePresets.json     # Build presets
├── mkdocs.yml            # Documentation config
├── README.md             # Project readme
├── CLAUDE.md             # AI assistant instructions
└── LICENSE.md            # MIT license
```

## Source Code (`src/`)

### Entry Point

```
src/
└── main.cpp              # Application entry, CLI parsing
```

The entry point:

- Parses command line arguments using CLI11
- Creates the Application instance
- Configures settings (texture path, debug mode)
- Runs the main loop

### Core Layer (`src/core/`)

```
src/core/
├── application.hpp/cpp      # Main application class
├── vulkan_context.hpp/cpp   # Vulkan device, swapchain, queues
├── renderer.hpp/cpp         # Rendering orchestration
├── pipeline.hpp/cpp         # Graphics pipeline, descriptors
├── buffer.hpp/cpp           # GPU buffer management
├── render_state.hpp         # Centralized render state
└── shader_loader.hpp        # Shader loading utilities
```

| File | Purpose |
|------|---------|
| `application` | Window, main loop, component coordination |
| `vulkan_context` | Vulkan initialization, swapchain, queues |
| `renderer` | Command buffer recording, frame submission |
| `pipeline` | Pipeline creation, descriptor sets |
| `buffer` | GPU buffer abstraction with staging |
| `render_state` | Shared rendering state |
| `shader_loader` | SPIR-V shader loading |

### W3D Parser (`src/w3d/`)

```
src/w3d/
├── w3d.hpp                    # Module header (includes all)
├── types.hpp                  # Data structures
├── chunk_types.hpp            # Chunk type enumerations
├── chunk_reader.hpp           # Binary chunk parsing
├── loader.hpp/cpp             # File loading orchestrator
├── model_loader.hpp/cpp       # High-level model interface
├── mesh_parser.hpp/cpp        # Mesh chunk parsing
├── hierarchy_parser.hpp/cpp   # Skeleton parsing
├── animation_parser.hpp/cpp   # Animation parsing
└── hlod_parser.hpp/cpp        # HLod parsing
```

| File | Purpose |
|------|---------|
| `w3d` | Convenience header including all W3D types |
| `types` | Mesh, Hierarchy, Animation, HLod structures |
| `chunk_types` | W3D chunk type IDs and constants |
| `chunk_reader` | Low-level binary reading utilities |
| `loader` | Orchestrates parsing of W3D files |
| `model_loader` | High-level interface for loading |
| `mesh_parser` | Parses MESH chunks |
| `hierarchy_parser` | Parses HIERARCHY chunks |
| `animation_parser` | Parses ANIMATION chunks |
| `hlod_parser` | Parses HLOD chunks |

### Rendering (`src/render/`)

```
src/render/
├── animation_player.hpp/cpp    # Animation playback
├── bone_buffer.hpp/cpp         # Bone transformation buffer
├── bounding_box.hpp            # AABB utilities
├── camera.hpp/cpp              # Orbital camera
├── hlod_model.hpp/cpp          # HLod model assembly
├── hover_detector.hpp/cpp      # Mesh picking
├── material.hpp                # Material definitions
├── mesh_converter.hpp/cpp      # W3D to GPU conversion
├── raycast.hpp/cpp             # Ray intersection
├── renderable_mesh.hpp/cpp     # GPU mesh representation
├── skeleton.hpp/cpp            # Skeleton pose computation
├── skeleton_renderer.hpp/cpp   # Skeleton visualization
└── texture.hpp/cpp             # Texture loading
```

| File | Purpose |
|------|---------|
| `animation_player` | Animation timeline and playback |
| `bone_buffer` | GPU buffer for bone matrices |
| `bounding_box` | Axis-aligned bounding box math |
| `camera` | Orbital camera with mouse control |
| `hlod_model` | Multi-LOD model with sub-objects |
| `hover_detector` | Raycast-based mesh picking |
| `material` | Material data for GPU |
| `mesh_converter` | Convert W3D mesh to GPU format |
| `raycast` | Ray-triangle intersection |
| `renderable_mesh` | GPU buffers for mesh rendering |
| `skeleton` | Bone pose computation |
| `skeleton_renderer` | Bone visualization rendering |
| `texture` | Texture loading and caching |

### UI Layer (`src/ui/`)

```
src/ui/
├── imgui_backend.hpp/cpp       # ImGui Vulkan integration
├── ui_manager.hpp/cpp          # Window/panel management
├── ui_context.hpp              # Shared UI context
├── ui_window.hpp               # Window base class
├── ui_panel.hpp                # Panel base class
├── console_window.hpp/cpp      # Debug console
├── file_browser.hpp/cpp        # File open dialog
├── viewport_window.hpp/cpp     # 3D viewport
├── hover_tooltip.hpp/cpp       # Tooltip display
└── panels/
    ├── animation_panel.hpp/cpp    # Animation controls
    ├── camera_panel.hpp/cpp       # Camera settings
    ├── display_panel.hpp/cpp      # Display options
    ├── lod_panel.hpp/cpp          # LOD selection
    └── model_info_panel.hpp/cpp   # Model information
```

| File | Purpose |
|------|---------|
| `imgui_backend` | Vulkan rendering for ImGui |
| `ui_manager` | Manages UI components lifecycle |
| `ui_context` | Shared state for UI components |
| `ui_window` | Base class for floating windows |
| `ui_panel` | Base class for docked panels |
| `console_window` | Log output display |
| `file_browser` | Navigate and select files |
| `viewport_window` | 3D model viewport |
| `panels/*` | Individual UI panels |

## Shaders (`shaders/`)

```
shaders/
├── basic.vert        # Basic vertex shader
├── basic.frag        # Basic fragment shader
├── skinned.vert      # Skeletal animation vertex
├── skeleton.vert     # Skeleton visualization vertex
└── skeleton.frag     # Skeleton visualization fragment
```

Shaders are compiled to SPIR-V at build time and embedded in the executable.

## Tests (`tests/`)

```
tests/
├── CMakeLists.txt         # Test configuration
├── w3d/                   # W3D parsing tests
│   ├── test_chunk_reader.cpp
│   ├── test_loader.cpp
│   ├── test_mesh_parser.cpp
│   ├── test_hierarchy_parser.cpp
│   ├── test_animation_parser.cpp
│   └── test_hlod_parser.cpp
├── render/                # Rendering tests
│   ├── test_animation_player.cpp
│   ├── test_bounding_box.cpp
│   ├── test_mesh_converter.cpp
│   ├── test_skeleton_pose.cpp
│   ├── test_texture_loading.cpp
│   └── raycast_test.cpp
└── stubs/                 # Mock implementations
    └── core/
        └── pipeline.hpp
```

Test structure mirrors `src/` for easy navigation.

## Dependencies (`lib/`)

```
lib/
├── CLI11/           # Command-line parser
├── Vulkan-Hpp/      # C++ Vulkan bindings
├── glfw/            # Window library
├── glm/             # Math library
├── googletest/      # Testing framework
└── imgui/           # GUI library
```

All dependencies are git submodules.

## GitHub Configuration (`.github/`)

```
.github/
├── AGENTS.md              # Agent guidelines
└── workflows/
    ├── clang-format.yaml  # Auto-format PR
    ├── pr-test.yml        # Test on PR
    ├── docs.yml           # Deploy documentation
    └── release.yml        # Release workflow
```

## Build Configuration

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Main build configuration |
| `CMakePresets.json` | Debug/release/test presets |
| `cmake/EmbedShaders.cmake` | Shader embedding module |
| `.clang-format` | Code formatting rules |
| `.editorconfig` | Editor configuration |
