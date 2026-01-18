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
./build/debug/VulkanApp.exe    # Windows debug
./build/release/VulkanApp.exe  # Windows release
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
  w3d/                        # (Phase 2) W3D file parsing
  render/                     # (Phase 3+) Mesh rendering, camera, animation
shaders/
  basic.vert/frag             # Basic lit shader with vertex colors
```

## Implementation Phases

| Phase | Status | Description |
|-------|--------|-------------|
| 1 | Done | Vulkan foundation - device, swapchain, pipeline, cube rendering |
| 2 | Pending | W3D file parsing - chunk reader, mesh/hierarchy/animation structs |
| 3 | Pending | Static mesh rendering - GPU upload, textures, viewer controls |
| 4 | Pending | Hierarchy/pose - bone matrices, rest pose display |
| 5 | Pending | HLod assembly - model assembly, LOD switching |
| 6 | Pending | Materials - W3D shader states, multi-pass |

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
