# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure (first time or after CMakeLists.txt changes)
cmake --preset debug    # Debug build
cmake --preset release  # Release build

# Build
cmake --build --preset debug
cmake --build --preset release

# Run (after building)
./build/debug/VulkanW3DViewer.exe    # Windows debug
./build/release/VulkanW3DViewer.exe  # Windows release
```

## Project Overview

This is a **W3D format renderer** - a modern Vulkan-based tool for loading and rendering W3D 3D model files from Command & Conquer Generals. The project uses C++20, GLFW for windowing, Vulkan-Hpp for C++ Vulkan bindings, and GLM for math.

**W3D Format:** Westwood 3D format (chunk-based) containing meshes, hierarchies, skeletal animations, and HLod (hierarchical LOD). Reference implementation is in `legacy/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/`.

### Requirements
- **Vulkan 1.3+** GPU (uses dynamic rendering, no VkRenderPass)
- Vulkan SDK installed system-wide (`C:/VulkanSDK/`)
- GLFW, Vulkan-Hpp, and GLM are git submodules in `lib/`

## Architecture

```
src/
  main.cpp                    # Application entry, render loop
  core/
    vulkan_context.hpp/cpp    # Device, swapchain, queues, depth buffer
    buffer.hpp/cpp            # GPU buffer management with staging
    pipeline.hpp/cpp          # Graphics pipeline, descriptors
  w3d/
    loader.hpp/cpp            # W3D file loading interface
    chunk_reader.hpp          # Binary chunk parsing utilities
    chunk_types.hpp           # W3D chunk type enumerations
    types.hpp                 # W3D data structures (Mesh, Hierarchy, Animation, etc.)
    mesh_parser.hpp/cpp       # Mesh chunk parsing
    hierarchy_parser.hpp/cpp  # Skeleton/bone parsing
    animation_parser.hpp/cpp  # Animation keyframe parsing
    hlod_parser.hpp/cpp       # Hierarchical LOD parsing
  render/
    bounding_box.hpp          # AABB utilities
    camera.hpp/cpp            # Orbital camera with mouse controls
    mesh_converter.hpp/cpp    # W3D mesh to GPU vertex conversion
    renderable_mesh.hpp/cpp   # GPU buffer management for meshes
  ui/
    imgui_backend.hpp/cpp     # ImGui Vulkan integration
    console_window.hpp/cpp    # Debug console UI
    file_browser.hpp/cpp      # File browser for loading W3D files
shaders/
  basic.vert/frag             # Basic lit shader with vertex colors
```

## Implementation Phases

| Phase | Status | Description |
|-------|--------|-------------|
| 1 | Done | Vulkan foundation - device, swapchain, pipeline, cube rendering |
| 2 | Done | W3D file parsing - chunk reader, mesh/hierarchy/animation structs |
| 3 | Done | Static mesh rendering - GPU upload, viewer controls (no textures yet) |
| 4 | Pending | Hierarchy/pose - bone matrices, rest pose display |
| 5 | Pending | HLod assembly - model assembly, LOD switching |
| 6 | Pending | Materials - W3D shader states, textures, multi-pass |

## Code Style

- 2-space indentation for C/C++ files
- Compiler warnings treated as errors (`-Werror`)
- Uses Clang toolchain via MSYS2 MinGW64
- Namespace: `w3d::`
- Modern C++20 patterns (structured bindings, designated initializers)

## Key Reference Files

- **W3D format spec:** `legacy/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/w3d_file.h`
- **Original mesh loading:** `legacy/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/meshmdlio.cpp`
- **Vulkan-Hpp samples:** `lib/Vulkan-Hpp/RAII_Samples/`
