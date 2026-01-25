# VulkanW3DViewer

A modern Vulkan-based viewer and renderer for W3D (Westwood 3D) model files from Command & Conquer: Generals.

## Overview

VulkanW3DViewer is a high-performance 3D model viewer designed to load and render W3D format files used in Command & Conquer: Generals. The project leverages modern Vulkan 1.3+ features including dynamic rendering, providing a reference implementation for parsing and displaying W3D meshes, skeletal hierarchies, animations, and hierarchical LOD (HLod) data.

### Key Features

- **W3D Format Support**: Chunk-based parsing of Westwood 3D files
- **Modern Vulkan Rendering**: Uses Vulkan 1.3+ with dynamic rendering (no legacy VkRenderPass)
- **Interactive Camera**: Orbital camera controls with mouse input
- **ImGui Integration**: Debug console and file browser for runtime model loading
- **Skeletal Animation Support**: Parsing infrastructure for hierarchies and animation keyframes (rendering in progress)

### W3D Format

W3D (Westwood 3D) is a chunk-based binary format containing:
- **Meshes**: Geometry with vertices, normals, vertex colors, and materials
- **Hierarchies**: Skeletal bone structures for animation
- **Animations**: Keyframe-based skeletal animations
- **HLod**: Hierarchical Level of Detail data for model assembly

## Requirements

### Hardware
- GPU with Vulkan 1.3+ support

### Software
- **Vulkan SDK** (1.3 or later) - Must be installed system-wide at `C:/VulkanSDK/`
- **CMake** (3.20 or later)
- **C++20 compiler** (Clang recommended via MSYS2 MinGW64 on Windows)

### Dependencies (Included as Git Submodules)
- **GLFW**: Windowing and input handling
- **Vulkan-Hpp**: C++ bindings for Vulkan
- **GLM**: OpenGL Mathematics library
- **ImGui**: Immediate mode GUI (integrated in `src/ui/`)

## Building

### Initial Setup

1. Clone the repository with submodules:
```bash
git clone --recursive <repository-url>
cd VulkanW3DViewer
```

If you already cloned without `--recursive`, initialize submodules:
```bash
git submodule update --init --recursive
```

2. Ensure Vulkan SDK is installed and the `VULKAN_SDK` environment variable is set.

### Build Instructions

The project uses CMake presets for configuration:

#### Debug Build
```bash
# Configure
cmake --preset debug

# Build
cmake --build --preset debug

# Run
./build/debug/VulkanW3DViewer.exe
```

#### Release Build
```bash
# Configure
cmake --preset release

# Build
cmake --build --preset release

# Run
./build/release/VulkanW3DViewer.exe
```

### Build Configuration

- Compiler warnings are treated as errors (`-Werror`)
- Uses C++20 standard
- Clang toolchain via MSYS2 MinGW64 (Windows)

## Project Structure

```
src/
  main.cpp                    # Application entry and render loop
  core/
    vulkan_context.hpp/cpp    # Device, swapchain, queues, depth buffer
    buffer.hpp/cpp            # GPU buffer management with staging
    pipeline.hpp/cpp          # Graphics pipeline and descriptors
  w3d/
    loader.hpp/cpp            # W3D file loading interface
    chunk_reader.hpp          # Binary chunk parsing utilities
    chunk_types.hpp           # W3D chunk type enumerations
    types.hpp                 # W3D data structures
    mesh_parser.hpp/cpp       # Mesh chunk parsing
    hierarchy_parser.hpp/cpp  # Skeleton/bone parsing
    animation_parser.hpp/cpp  # Animation keyframe parsing
    hlod_parser.hpp/cpp       # Hierarchical LOD parsing
  render/
    camera.hpp/cpp            # Orbital camera with mouse controls
    mesh_converter.hpp/cpp    # W3D mesh to GPU vertex conversion
    renderable_mesh.hpp/cpp   # GPU mesh buffer management
    bounding_box.hpp          # AABB utilities
  ui/
    imgui_backend.hpp/cpp     # ImGui Vulkan integration
    console_window.hpp/cpp    # Debug console UI
    file_browser.hpp/cpp      # File browser for W3D loading
shaders/
  basic.vert/frag             # Basic lit shader with vertex colors
```

## Development Status

| Phase | Status | Description |
|-------|--------|-------------|
| Phase 1 | ✅ Complete | Vulkan foundation - device, swapchain, pipeline, basic rendering |
| Phase 2 | ✅ Complete | W3D file parsing - chunk reader, mesh/hierarchy/animation structures |
| Phase 3 | ✅ Complete | Static mesh rendering - GPU upload, camera controls |
| Phase 4 | 🚧 In Progress | Hierarchy/pose - bone matrices, rest pose display |
| Phase 5 | 📋 Planned | HLod assembly - model assembly, LOD switching |
| Phase 6 | 📋 Planned | Materials - W3D shader states, textures, multi-pass rendering |

## Usage

1. Launch the application
2. Use the file browser UI to load `.w3d` files
3. Navigate with mouse:
   - **Left drag**: Rotate camera
   - **Scroll**: Zoom in/out
4. View debug information in the console window

## Code Style

- 2-space indentation
- Namespace: `w3d::`
- Modern C++20 patterns (structured bindings, designated initializers)

## License

See LICENSE file for details.

## Acknowledgments

- Original W3D format implementation from Command & Conquer: Generals
- Vulkan-Hpp for modern C++ Vulkan bindings
- ImGui for debug UI
