# VulkanW3DViewer

Modern Vulkan-based renderer for W3D format files from Command & Conquer: Generals.

[![Documentation](https://img.shields.io/badge/docs-online-blue)](https://vitexftw.github.io/VulkanW3DViewer/)

> **EA has not endorsed and does not support this product.**

## Features

- Vulkan 1.3+ with dynamic rendering
- W3D meshes, skeletal hierarchies, and animations
- Hierarchical LOD (HLod) with dynamic switching
- Material and texture support
- ImGui-based debug interface

## Quick Start

```bash
# Clone with submodules
git clone --recursive https://github.com/ViTeXFTW/VulkanW3DViewer.git
cd VulkanW3DViewer

# Build (Linux/macOS)
./scripts/rebuild.sh release

# Run
./build/release/VulkanW3DViewer model.w3d
```

See the [Getting Started Guide](https://vitexftw.github.io/VulkanW3DViewer/getting-started/) for detailed instructions.

## Requirements

- GPU with Vulkan 1.3+ support
- [Vulkan SDK](https://vulkan.lunarg.com/) 1.3+
- CMake 3.20+
- C++20 compiler (Clang recommended)

## Documentation

Full documentation: **[vitexftw.github.io/VulkanW3DViewer](https://vitexftw.github.io/VulkanW3DViewer/)**

| Section | Description |
|---------|-------------|
| [Getting Started](https://vitexftw.github.io/VulkanW3DViewer/getting-started/) | Installation and building |
| [User Guide](https://vitexftw.github.io/VulkanW3DViewer/user-guide/) | Using the viewer |
| [W3D Format](https://vitexftw.github.io/VulkanW3DViewer/w3d-format/) | Technical format spec |
| [Architecture](https://vitexftw.github.io/VulkanW3DViewer/architecture/) | Codebase overview |
| [Development](https://vitexftw.github.io/VulkanW3DViewer/development/) | Contributing guidelines |

## License

[MIT License](./LICENSE.md)
