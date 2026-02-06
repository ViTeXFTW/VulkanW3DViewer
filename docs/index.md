# VulkanW3DViewer

**Modern Vulkan-based renderer for W3D format files from Command & Conquer Generals**

---

<div class="grid cards" markdown>

-   :material-rocket-launch:{ .lg .middle } **Getting Started**

    ---

    Install dependencies, build the project, and run your first model in minutes.

    [:octicons-arrow-right-24: Quick Start](getting-started/quick-start.md)

-   :material-book-open-variant:{ .lg .middle } **User Guide**

    ---

    Learn how to load models, navigate the 3D viewport, and use the interface.

    [:octicons-arrow-right-24: User Guide](user-guide/index.md)

-   :material-cube-outline:{ .lg .middle } **W3D Format**

    ---

    Technical documentation for the Westwood 3D file format.

    [:octicons-arrow-right-24: W3D Format](w3d-format/index.md)

-   :material-code-braces:{ .lg .middle } **Architecture**

    ---

    Dive into the codebase structure and understand how it all fits together.

    [:octicons-arrow-right-24: Architecture](architecture/index.md)

</div>

---

## Overview

VulkanW3DViewer is a high-performance 3D model viewer designed to load and render **W3D format** files used in **Command & Conquer: Generals**. The project leverages modern **Vulkan 1.3+** features including dynamic rendering, providing a reference implementation for parsing and displaying:

- **Meshes** with full material and texture support
- **Skeletal hierarchies** and bone structures
- **Animations** with real-time playback
- **Hierarchical LOD (HLod)** with dynamic level switching

!!! warning "Disclaimer"
    **EA has not endorsed and does not support this product.**
    All rights go to their respective owners.

## Features

- **Modern Vulkan Rendering** - Uses Vulkan 1.3+ with dynamic rendering (no VkRenderPass)
- **Complete W3D Support** - Meshes, hierarchies, animations, and HLod
- **Real-time Animation** - Smooth skeletal animation playback
- **Texture Support** - Automatic texture loading with custom path support
- **Debug Visualization** - Skeleton rendering and model inspection
- **Cross-platform** - Windows, Linux, macOS support

## Requirements

| Component | Requirement |
|-----------|-------------|
| GPU | Vulkan 1.3+ support |
| Vulkan SDK | 1.3 or later |
| CMake | 3.20 or later |
| Compiler | C++20 (Clang recommended) |

## Quick Example

```bash
# Clone with submodules
git clone --recursive https://github.com/ViTeXFTW/VulkanW3DViewer.git
cd VulkanW3DViewer

# Build (Linux/macOS)
./scripts/rebuild.sh release

# Run with a model
./build/release/VulkanW3DViewer model.w3d
```

## Development Status

All core phases are complete:

| Phase | Description |
|-------|-------------|
| :white_check_mark: | Vulkan foundation - device, swapchain, pipeline |
| :white_check_mark: | W3D file parsing - chunk reader, data structures |
| :white_check_mark: | Static mesh rendering with viewer controls |
| :white_check_mark: | Hierarchy/pose with bone matrices |
| :white_check_mark: | HLod assembly with LOD switching |
| :white_check_mark: | Materials with texture support |
| :white_check_mark: | Animation loading and playback |
| :white_check_mark: | Skeletal animation rendering |

## License

This project is licensed under the [MIT License](https://github.com/ViTeXFTW/VulkanW3DViewer/blob/main/LICENSE.md).
