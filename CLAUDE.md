# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure (first time or after CMakeLists.txt changes)
cmake --preset debug    # Debug build
cmake --preset release  # Release build
cmake --preset test     # Test build

# Build
cmake --build --preset debug
cmake --build --preset release
cmake --build --preset test

# Run (after building)
./build/debug/VulkanW3DViewer.exe    # Windows debug
./build/release/VulkanW3DViewer.exe  # Windows release
ctest --preset test                  # Run tests
```

## Project Overview

This is a **W3D format renderer** - a modern Vulkan-based tool for loading and rendering W3D 3D model files from Command & Conquer Generals. The project uses C++20, GLFW for windowing, Vulkan-Hpp for C++ Vulkan bindings, and GLM for math.

**W3D Format:** Westwood 3D format (chunk-based) containing meshes, hierarchies, skeletal animations, and HLod (hierarchical LOD). Reference implementation is in `legacy/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/`.

### Requirements

- **Vulkan 1.3+** GPU (uses dynamic rendering, no VkRenderPass)
- Vulkan SDK installed system-wide (`C:/VulkanSDK/`)
- GLFW, Vulkan-Hpp, ImGui, GLM and googletest are git submodules in `lib/`

## Architecture

- Use RAII where ever applicable to keep track of resource lifetime.

### Code Structure

```
src/
  main.cpp                    # Application entry, render loop
  core/
    vulkan_context.hpp/cpp    # Device, swapchain, queues, depth buffer
    buffer.hpp/cpp            # GPU buffer management with staging
    pipeline.hpp/cpp          # Graphics pipeline, descriptors
  w3d/
    w3d.hpp                   # W3D module main header
    loader.hpp/cpp            # W3D file loading interface
    chunk_reader.hpp          # Binary chunk parsing utilities
    chunk_types.hpp           # W3D chunk type enumerations
    types.hpp                 # W3D data structures (Mesh, Hierarchy, Animation, etc.)
    mesh_parser.hpp/cpp       # Mesh chunk parsing
    hierarchy_parser.hpp/cpp  # Skeleton/bone parsing
    animation_parser.hpp/cpp  # Animation keyframe parsing
    hlod_parser.hpp/cpp       # Hierarchical LOD parsing
  render/
    animation_player.hpp/cpp  # Animation playback control
    bone_buffer.hpp/cpp       # GPU buffer management for bone transformations
    bounding_box.hpp          # AABB utilities
    camera.hpp/cpp            # Orbital camera with mouse controls
    hlod_model.hpp/cpp        # HLod model assembly with LOD switching
    material.hpp              # Material definitions and GPU format
    mesh_converter.hpp/cpp    # W3D mesh to GPU vertex conversion
    renderable_mesh.hpp/cpp   # GPU buffer management for meshes
    skeleton.hpp/cpp          # Skeleton pose computation
    skeleton_renderer.hpp/cpp # Skeleton debug visualization
    texture.hpp/cpp           # Texture loading and management
  ui/
    imgui_backend.hpp/cpp     # ImGui Vulkan integration
    console_window.hpp/cpp    # Debug console UI
    file_browser.hpp/cpp      # File browser for loading W3D files
shaders/
  basic.vert/frag             # Shader with texture and material support
```

## Implementation Phases

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

## Code Style

- 2-space indentation for C/C++ files
- Compiler warnings treated as errors (`-Werror`)
- Uses Clang toolchain via MSYS2 MinGW64
- Namespace: `w3d::`
- Modern C++20 patterns (structured bindings, designated initializers)

## Testing

- New features should follow a Test Driven Development (TDD) style where the expected behavior of key functions are created and added to the relevant test suite before implementing the feature.
- Tests should be structured similar to the `src/` directory to easily find the relevant tests for the associated file.

## Key Reference Files

- **W3D format spec:** `legacy/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/w3d_file.h`
- **Original mesh loading:** `legacy/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/meshmdlio.cpp`
- **Vulkan-Hpp samples:** `lib/Vulkan-Hpp/RAII_Samples/`
