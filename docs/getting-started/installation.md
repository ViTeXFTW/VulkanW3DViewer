# Installation

This guide covers installing the prerequisites needed to build VulkanW3DViewer.

## Prerequisites

### Vulkan SDK

The Vulkan SDK is required for building and running the application.

=== "Windows"

    1. Download the Vulkan SDK from [LunarG](https://vulkan.lunarg.com/sdk/home)
    2. Run the installer
    3. The SDK should install to `C:/VulkanSDK/`
    4. Verify the `VULKAN_SDK` environment variable is set

    ```powershell
    # Verify installation
    echo $env:VULKAN_SDK
    ```

=== "Linux"

    **Ubuntu/Debian:**
    ```bash
    # Add LunarG repository
    wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
    sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list https://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list

    # Install SDK
    sudo apt update
    sudo apt install vulkan-sdk
    ```

    **Arch Linux:**
    ```bash
    sudo pacman -S vulkan-devel
    ```

    **Fedora:**
    ```bash
    sudo dnf install vulkan-tools vulkan-loader-devel vulkan-validation-layers-devel
    ```

=== "macOS"

    1. Download the Vulkan SDK from [LunarG](https://vulkan.lunarg.com/sdk/home)
    2. Mount the DMG and run the installer
    3. Add to your shell profile:

    ```bash
    export VULKAN_SDK="$HOME/VulkanSDK/<version>/macOS"
    export PATH="$VULKAN_SDK/bin:$PATH"
    export DYLD_LIBRARY_PATH="$VULKAN_SDK/lib:$DYLD_LIBRARY_PATH"
    export VK_ICD_FILENAMES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"
    export VK_LAYER_PATH="$VULKAN_SDK/share/vulkan/explicit_layer.d"
    ```

### CMake

CMake 3.20 or later is required.

=== "Windows"

    Download from [cmake.org](https://cmake.org/download/) or install via:
    ```powershell
    winget install Kitware.CMake
    ```

=== "Linux"

    ```bash
    # Ubuntu/Debian
    sudo apt install cmake

    # Arch
    sudo pacman -S cmake

    # Fedora
    sudo dnf install cmake
    ```

=== "macOS"

    ```bash
    brew install cmake
    ```

### Compiler

A C++20 compatible compiler is required. Clang is recommended.

=== "Windows (MSYS2)"

    1. Install [MSYS2](https://www.msys2.org/)
    2. Open MSYS2 MinGW64 shell
    3. Install the toolchain:

    ```bash
    pacman -S mingw-w64-x86_64-clang mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
    ```

=== "Linux"

    ```bash
    # Ubuntu/Debian
    sudo apt install clang

    # Arch
    sudo pacman -S clang

    # Fedora
    sudo dnf install clang
    ```

=== "macOS"

    Clang comes with Xcode Command Line Tools:
    ```bash
    xcode-select --install
    ```

## Clone the Repository

Clone the repository with submodules:

```bash
git clone --recursive https://github.com/ViTeXFTW/VulkanW3DViewer.git
cd VulkanW3DViewer
```

!!! warning "Submodules Required"
    The `--recursive` flag is essential. If you forget it, initialize submodules manually:
    ```bash
    git submodule update --init --recursive
    ```

## Verify Installation

Confirm everything is set up:

```bash
# Check Vulkan
vulkaninfo --summary

# Check CMake
cmake --version

# Check compiler
clang++ --version
```

## Next Steps

Now you're ready to [build the project](building.md)!
