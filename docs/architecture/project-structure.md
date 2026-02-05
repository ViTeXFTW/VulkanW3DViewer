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

### Library Layer (`src/lib/`)

The library layer contains reusable components that can be used independently of the viewer application.

```
src/lib/
├── formats/w3d/              # W3D format parsing library
│   ├── w3d.hpp               # Module header (includes all)
│   ├── types.hpp             # Data structures
│   ├── chunk_types.hpp       # Chunk type enumerations
│   ├── chunk_reader.hpp      # Binary chunk parsing
│   ├── loader.hpp/cpp        # File loading orchestrator
│   ├── model_loader.hpp/cpp  # High-level model interface
│   ├── mesh_parser.hpp/cpp   # Mesh chunk parsing
│   ├── hierarchy_parser.hpp/cpp # Skeleton parsing
│   ├── animation_parser.hpp/cpp # Animation parsing
│   ├── hlod_parser.hpp/cpp   # HLod parsing
│   └── hlod_model.hpp/cpp    # HLod model assembly
├── gfx/                      # Graphics foundation library
│   ├── vulkan_context.hpp/cpp # Vulkan device, swapchain, queues
│   ├── buffer.hpp/cpp        # GPU buffer management
│   ├── pipeline.hpp/cpp      # Graphics pipeline, descriptors
│   ├── texture.hpp/cpp       # Texture loading
│   ├── camera.hpp/cpp        # Camera utilities
│   ├── bounding_box.hpp      # AABB math utilities
│   └── renderable.hpp        # Base renderable interface
└── scene/                    # Scene management library
    └── scene.hpp/cpp         # Scene container
```

| Module | Purpose |
|--------|---------|
| `formats/w3d` | W3D file format parsing and data structures |
| `gfx` | Vulkan abstraction and GPU resource management |
| `scene` | Scene graph and renderable object management |

### Core Layer (`src/core/`)

The core layer contains application-specific orchestration code.

```
src/core/
├── application.hpp/cpp      # Main application class
├── renderer.hpp/cpp         # Rendering orchestration
├── render_state.hpp         # Centralized render state
├── shader_loader.hpp        # Shader loading utilities
├── settings.hpp/cpp         # Application settings
└── app_paths.hpp/cpp        # Application path utilities
```

| File | Purpose |
|------|---------|
| `application` | Window, main loop, component coordination |
| `renderer` | Command buffer recording, frame submission |
| `render_state` | Shared rendering state |
| `shader_loader` | SPIR-V shader loading |
| `settings` | Application configuration |
| `app_paths` | Platform-specific path resolution |

### Rendering (`src/render/`)

The render layer contains viewer-specific rendering utilities (not part of the reusable library).

```
src/render/
├── animation_player.hpp/cpp    # Animation playback
├── bone_buffer.hpp/cpp         # Bone transformation buffer
├── hover_detector.hpp/cpp      # Mesh picking
├── material.hpp                # Material definitions
├── mesh_converter.hpp/cpp      # W3D to GPU conversion
├── raycast.hpp/cpp             # Ray intersection
├── renderable_mesh.hpp/cpp     # GPU mesh representation
├── skeleton.hpp/cpp            # Skeleton pose computation
└── skeleton_renderer.hpp/cpp   # Skeleton visualization
```

| File | Purpose |
|------|---------|
| `animation_player` | Animation timeline and playback |
| `bone_buffer` | GPU buffer for bone matrices |
| `hover_detector` | Raycast-based mesh picking |
| `material` | Material data for GPU |
| `mesh_converter` | Convert W3D mesh to GPU format |
| `raycast` | Ray-triangle intersection |
| `renderable_mesh` | GPU buffers for mesh rendering |
| `skeleton` | Bone pose computation |
| `skeleton_renderer` | Bone visualization rendering |

**Note:** Camera, texture, and bounding_box utilities have been moved to `src/lib/gfx/` as they are reusable components.

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
├── core/                  # Application core tests
│   ├── test_app_paths.cpp
│   └── test_settings.cpp
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
│   ├── test_hlod_hover.cpp
│   ├── test_mesh_converter.cpp
│   ├── test_mesh_visibility.cpp
│   ├── test_skeleton_pose.cpp
│   ├── test_texture_loading.cpp
│   └── raycast_test.cpp
└── ui/                    # UI tests
    └── test_file_browser.cpp
```

Test structure mirrors `src/` for easy navigation. Note that `tests/w3d/` tests the library components in `src/lib/formats/w3d/`.

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
