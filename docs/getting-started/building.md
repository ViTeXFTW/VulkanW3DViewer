# Building

This guide covers building VulkanW3DViewer from source.

## Quick Build

The fastest way to build is using the provided build scripts:

=== "Windows (PowerShell)"

    ```powershell
    .\scripts\rebuild.ps1 release
    ```

=== "Linux/macOS"

    ```bash
    ./scripts/rebuild.sh release

    # If permission denied:
    chmod +x scripts/rebuild.sh
    ./scripts/rebuild.sh release
    ```

## Build Presets

The project uses CMake presets for different build configurations:

| Preset | Description | Output Directory |
|--------|-------------|------------------|
| `debug` | Debug build with symbols, no optimization | `build/debug/` |
| `release` | Optimized release build | `build/release/` |
| `test` | Debug build with testing enabled | `build/test/` |

## Manual Build

If you prefer manual control over the build process:

### Configure

```bash
# Debug build
cmake --preset debug

# Release build
cmake --preset release

# Test build
cmake --preset test
```

### Build

```bash
# Build debug
cmake --build --preset debug

# Build release
cmake --build --preset release

# Build tests
cmake --build --preset test
```

### Run

```bash
# Windows
./build/release/VulkanW3DViewer.exe

# Linux/macOS
./build/release/VulkanW3DViewer
```

## Running Tests

Build and run the test suite:

```bash
# Configure and build tests
cmake --preset test
cmake --build --preset test

# Run tests
ctest --preset test
```

!!! tip "Parallel Test Execution"
    Speed up testing with parallel execution:
    ```bash
    ctest --preset test -j $(nproc)
    ```

## Build Options

### Compiler Flags

The project uses strict compiler settings:

- `-Werror` - Treat warnings as errors
- `-Wall -Wextra` - Enable comprehensive warnings
- `-std=c++20` - C++20 standard

### Shader Compilation

Shaders are automatically compiled to SPIR-V during the build process using `glslc` from the Vulkan SDK. The compiled shaders are embedded into the executable.

## Troubleshooting

### Common Issues

??? failure "Vulkan SDK not found"
    Ensure the `VULKAN_SDK` environment variable is set:
    ```bash
    # Check if set
    echo $VULKAN_SDK

    # Set manually (example path)
    export VULKAN_SDK=/path/to/VulkanSDK/1.3.xxx/x86_64
    ```

??? failure "Submodules not initialized"
    Initialize submodules if dependencies are missing:
    ```bash
    git submodule update --init --recursive
    ```

??? failure "Permission denied on build scripts"
    Make scripts executable:
    ```bash
    chmod +x scripts/rebuild.sh
    ```

??? failure "CMake version too old"
    The project requires CMake 3.20+. Check your version:
    ```bash
    cmake --version
    ```

### Clean Build

If you encounter issues, try a clean build:

```bash
# Remove build directory
rm -rf build/

# Rebuild from scratch
cmake --preset release
cmake --build --preset release
```

## IDE Integration

### Visual Studio Code

1. Install the CMake Tools extension
2. Open the project folder
3. Select the build preset from the CMake Tools panel
4. Build using ++ctrl+shift+b++

### CLion

1. Open the project folder
2. CLion will auto-detect CMake configuration
3. Select the desired preset from the build configuration dropdown

## Next Steps

After building successfully, learn how to [use the viewer](quick-start.md)!
