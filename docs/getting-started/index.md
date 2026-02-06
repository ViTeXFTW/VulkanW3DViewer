# Getting Started

Welcome to VulkanW3DViewer! This section will help you get up and running quickly.

## What You'll Need

Before you begin, make sure you have:

- A GPU with **Vulkan 1.3+** support
- **Vulkan SDK** 1.3 or later installed
- **CMake** 3.20 or later
- A **C++20 compiler** (Clang recommended)

## Quick Links

<div class="grid cards" markdown>

-   :material-download:{ .lg .middle } **Installation**

    ---

    Install the Vulkan SDK and clone the repository

    [:octicons-arrow-right-24: Installation Guide](installation.md)

-   :material-hammer:{ .lg .middle } **Building**

    ---

    Build the project from source

    [:octicons-arrow-right-24: Build Instructions](building.md)

-   :material-rocket-launch:{ .lg .middle } **Quick Start**

    ---

    Load your first W3D model

    [:octicons-arrow-right-24: Quick Start Guide](quick-start.md)

</div>

## Platform Support

| Platform | Status | Compiler |
|----------|--------|----------|
| Windows | :white_check_mark: Supported | Clang (via MSYS2 MinGW64) |
| Linux | :white_check_mark: Supported | Clang/GCC |
| macOS | :white_check_mark: Supported | Clang |

## Dependencies

All dependencies are included as git submodules - no separate installation required:

| Library | Purpose |
|---------|---------|
| GLFW | Window creation and input handling |
| Vulkan-Hpp | Modern C++ bindings for Vulkan |
| GLM | OpenGL Mathematics library |
| ImGui | Immediate-mode debug GUI |
| CLI11 | Command-line argument parsing |
| GoogleTest | Testing framework |

## Next Steps

1. [Install the prerequisites](installation.md)
2. [Build the project](building.md)
3. [Load your first model](quick-start.md)
