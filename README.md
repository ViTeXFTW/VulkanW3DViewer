# VulkanW3DViewer

Modern renderer for Electronic Arts W3D format using Vulkan, GLFW and GLM.

[![Documentation](https://img.shields.io/badge/docs-online-blue)](https://vitexftw.github.io/VulkanW3DViewer/)

## Documentation

Full documentation is available at **[vitexftw.github.io/VulkanW3DViewer](https://vitexftw.github.io/VulkanW3DViewer/)**

- [Getting Started](https://vitexftw.github.io/VulkanW3DViewer/getting-started/) - Installation and build instructions
- [User Guide](https://vitexftw.github.io/VulkanW3DViewer/user-guide/) - How to use the viewer
- [W3D Format](https://vitexftw.github.io/VulkanW3DViewer/w3d-format/) - Technical format documentation
- [Architecture](https://vitexftw.github.io/VulkanW3DViewer/architecture/) - Codebase overview
- [Development](https://vitexftw.github.io/VulkanW3DViewer/development/) - Contributing guidelines

## Disclaimer

**EA has not endorsed and does not support this product.**  
**All rights go to their respective owners.**

## Overview

VulkanW3DViewer is a high-performance 3D model viewer designed to load and render W3D format files used in Command & Conquer: Generals. The project leverages modern Vulkan 1.3+ features including dynamic rendering, providing a reference implementation for parsing and displaying W3D meshes, skeletal hierarchies, animations, and hierarchical LOD (HLod) data.

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
- **CLI11**: Command line argument parser
- **GoogleTest**: Testing framework

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

The project uses CMake presets for configuration, depending on your operating system use the build scripts in the `scripts/` directory.

#### Windows

```ps1
> scripts/rebuild.ps1 <preset>
```

#### Linux / MacOS

```bash
> scripts/rebuild.sh <preset>
```

Some might need to update permissions and rerun the script:
```bash
chmod +x scripts/rebuild.sh
```


### Build Configuration

- Compiler warnings are treated as errors (`-Werror`)
- Uses C++20 standard
- Clang toolchain

## Development Status

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

Future development will include terrain rendering to as a baseline render a map from Command & Conquer Generals Zero Hour.

## Usage

### Command Line Options

```bash
VulkanW3DViewer [model] [OPTIONS]

Arguments:
  model                    W3D model file to load on startup

Options:
  -h,--help               Display help message and exit
  -t,--textures PATH      Set custom texture search path
  -d,--debug              Enable verbose debug output
```

### Examples

```bash
# Launch with GUI file browser
./VulkanW3DViewer

# Load a model directly
./VulkanW3DViewer model.w3d

# Load a model with custom texture path
./VulkanW3DViewer model.w3d -t /path/to/textures

# Enable debug output
./VulkanW3DViewer model.w3d --debug
```

### Navigation

1. Use the file browser UI to load `.w3d` files (or pass a model on the command line)
2. Navigate with mouse:
   - **Left drag**: Rotate camera
   - **Scroll**: Zoom in/out
3. View debug information in the console window

## License

See [LICENSE](./LICENSE.md) file for details.

## Acknowledgments

- Original W3D format implementation from Command & Conquer: Generals Zero Hour
- Vulkan-Hpp for modern C++ Vulkan bindings
- ImGui for debug UI
- GoogleTest for testing framework
