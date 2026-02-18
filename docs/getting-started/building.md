# Building

This guide covers building VulkanW3DViewer from source.

## Quick Build

The fastest way to build is using the provided build scripts:

=== "Windows (PowerShell)"

    ```powershell
    .\scripts\rebuild.ps1 debug                    # Auto-detect compiler
    .\scripts\rebuild.ps1 release -Compiler msvc   # MSVC (recommended on Windows)
    .\scripts\rebuild.ps1 debug -D                 # Clean build
    .\scripts\rebuild.ps1 release -Compiler msvc -R # Build and run
    .\scripts\rebuild.ps1 debug -Compiler clang    # Use Clang explicitly
    ```

=== "Linux/macOS"

    ```bash
    ./scripts/rebuild.sh debug                     # Auto-detect compiler
    ./scripts/rebuild.sh release -c gcc            # Use GCC
    ./scripts/rebuild.sh debug -c clang -d         # Clean build with Clang
    ./scripts/rebuild.sh test                      # Build and run tests

    # If permission denied:
    chmod +x scripts/rebuild.sh
    ./scripts/rebuild.sh release
    ```

## Build Presets

The project uses CMake presets for different build configurations:

| Preset | Compiler | Description | Output Directory |
|--------|----------|-------------|------------------|
| `debug` | Auto-detect | Debug build with symbols, no optimization | `build/debug/` |
| `release` | Auto-detect | Optimized release build | `build/release/` |
| `test` | Auto-detect | Debug build with testing enabled | `build/test/` |

## Compiler Selection

### Supported Compilers

The project supports multiple compilers, each with their own strengths:

| Compiler | Platform | Use Case |
|----------|----------|----------|
| **MSVC** | Windows | Recommended for Windows development. Best IDE integration with Visual Studio. |
| **Clang** | Windows, Linux, macOS | Excellent diagnostics, fast compilation. Good cross-platform development. |
| **GCC** | Linux, macOS | Standard on Linux systems. Excellent optimization. |

### Choosing a Compiler

- **Windows development**: Use MSVC for best Visual Studio integration, or Clang for better error messages
- **Linux development**: Use GCC or Clang (both well-supported)
- **macOS development**: Use Clang (Apple Clang is the default)
- **Cross-platform development**: Use Clang for consistent behavior across platforms

The build scripts will auto-detect the best available compiler if you don't specify one explicitly.

## Manual Build

If you prefer manual control over the build process:

### Configure

```bash
# Auto-detect compiler
cmake --preset debug          # Debug build
cmake --preset release        # Release build
cmake --preset test           # Test build

# Specific compilers
cmake --preset msvc-debug     # MSVC debug (Windows)
cmake --preset msvc-release   # MSVC release (Windows)
cmake --preset clang-debug    # Clang debug
cmake --preset clang-release  # Clang release
cmake --preset gcc-debug      # GCC debug
cmake --preset gcc-release    # GCC release
```

### Build

```bash
# Build auto-detected compiler
cmake --build --preset debug
cmake --build --preset release
cmake --build --preset test

# Build specific compilers
cmake --build --preset msvc-release
cmake --build --preset clang-debug
cmake --build --preset gcc-release
```

### Run

```bash
# Windows (auto-detected compiler)
./build/release/VulkanW3DViewer.exe

# Windows (MSVC)
./build/msvc-release/VulkanW3DViewer.exe

# Linux/macOS (auto-detected compiler)
./build/release/VulkanW3DViewer

# Linux/macOS (Clang)
./build/clang-release/VulkanW3DViewer

# Linux/macOS (GCC)
./build/gcc-release/VulkanW3DViewer
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

The project uses strict compiler settings across all supported compilers:

**GCC/Clang:**
- `-Werror` - Treat warnings as errors
- `-Wall -Wextra` - Enable comprehensive warnings
- `-Wpedantic` - Pedantic warnings for ISO C++ conformance
- `-std=c++20` - C++20 standard

**MSVC:**
- `/W4` - Warning level 4
- `/permissive-` - Standards conformance mode
- `/utf-8` - UTF-8 source and execution charset
- `/std:c++20` - C++20 standard (implied)

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
# Remove specific build directory
rm -rf build/release/          # Auto-detected compiler
rm -rf build/msvc-release/     # MSVC
rm -rf build/clang-release/    # Clang
rm -rf build/gcc-release/      # GCC

# Or remove all build directories
rm -rf build/

# Rebuild from scratch
cmake --preset release
cmake --build --preset release
```

**Using build scripts:**

=== "PowerShell"

    ```powershell
    .\scripts\rebuild.ps1 debug -D    # Clean debug build
    .\scripts\rebuild.ps1 release -D -Compiler msvc  # Clean MSVC release build
    ```

=== "Bash"

    ```bash
    ./scripts/rebuild.sh debug -d              # Clean debug build
    ./scripts/rebuild.sh release -c gcc -d    # Clean GCC release build
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
