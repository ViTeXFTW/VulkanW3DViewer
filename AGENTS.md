# AGENTS.md - VulkanW3DViewer

This file guides AI coding agents working on this Vulkan-based W3D renderer.

## Build Commands

```bash
# Quick Build (using helper scripts - recommended)
# PowerShell
.\scripts\rebuild.ps1 debug                    # Auto-detect compiler
.\scripts\rebuild.ps1 release -Compiler msvc   # MSVC (Windows)
.\scripts\rebuild.ps1 debug -D -R              # Clean debug build and run

# Bash
./scripts/rebuild.sh debug                     # Auto-detect compiler
./scripts/rebuild.sh release -c gcc            # GCC
./scripts/rebuild.sh debug -c clang -d         # Clean build with Clang

# Configure (CMake presets)
cmake --preset debug          # Auto-detect compiler
cmake --preset msvc-debug     # MSVC (Windows)
cmake --preset clang-release  # Clang
cmake --preset gcc-debug      # GCC
cmake --preset test           # Debug build with tests enabled (BUILD_TESTING=ON)

# Build
cmake --build --preset debug
cmake --build --preset msvc-release
cmake --build --preset clang-debug

# Run main application
./build/debug/VulkanW3DViewer.exe
./build/msvc-release/VulkanW3DViewer.exe
./build/clang-release/VulkanW3DViewer

# Run all tests
ctest --preset test

# Run specific test suite
./build/test/w3d_tests
./build/test/mesh_converter_tests
./build/test/skeleton_tests
./build/test/texture_tests
./build/test/bounding_box_tests
./build/test/raycast_tests
./build/test/hlod_hover_tests

# Run single test (GoogleTest filter)
./build/test/w3d_tests --gtest_filter=ChunkReaderTest.ReadUint8
```

## Code Style

### Formatting
- **Indentation**: 2 spaces (no tabs)
- **Column limit**: 100 characters
- **Brace style**: Attach (`} else {`)
- **Header guards**: `#pragma once` (not include guards)
- **Tool**: Run `clang-format` before committing (configured in `.clang-format`)

### Naming Conventions
- **Classes**: PascalCase (`ChunkReader`, `VulkanContext`, `Material`)
- **Functions**: camelCase (`readBytes`, `loadFromMemory`, `create`)
- **Member variables**: snake_case with trailing underscore (`data_`, `pos_`, `buffer_`)
- **Local variables**: snake_case (`result`, `index`, `count`)
- **Constants**: UPPER_CASE for compile-time (`MAX_FRAMES`, `W3D_DEBUG`)
- **Enums**: PascalCase for enum class, UPPER_CASE values (`BlendMode::Opaque`)
- **Structs**: PascalCase (`ChunkHeader`, `Vector3`, `MaterialInfo`)
- **Namespaces**: lowercase (`w3d`, `MaterialFlags`)

### Types
- Use `std::optional<T>` for fallible functions (return `std::nullopt` on failure)
- Use `std::span<const uint8_t>` for read-only byte buffers
- Use `uint32_t`, `uint8_t`, etc. from `<cstdint>` for fixed-width types
- Prefer `std::vector<T>` over raw arrays
- Use `std::filesystem::path` for file paths
- GLM types for math: `glm::vec3`, `glm::mat4`, etc.

### Imports/Includes
Regrouped includes in this priority order:
1. Main module headers (same directory, priority 1)
2. `<vulkan/...>` (priority 2 - MUST come before GLFW)
3. `<GLFW/...>` (priority 3)
4. `<glm/...>` (priority 4)
5. Standard library: `<string>`, `<vector>`, etc. (priority 5)
6. Other project headers (priority 6)

Example:
```cpp
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
#include "chunk_reader.hpp"
```

### Error Handling
- Use `std::optional<T>` for recoverable errors (file loading, parsing)
- Provide optional `std::string *outError` parameter for error messages
- Throw custom exceptions (e.g., `ParseError`) for logic errors
- Always validate bounds before array/vector access
- Check Vulkan result codes with `.result` from RAII wrappers

### Memory Management
- **RAII pattern** is mandatory for all resources (Vulkan objects, buffers, etc.)
- Classes manage their own resources in destructor
- Delete copy constructor/assignment for resource-owning types
- Implement move constructor/assignment for transfers
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) where appropriate
- No `new`/`delete` without ownership semantics

### C++20 Patterns
- Structured bindings: `auto [x, y, z] = ...`
- Designated initializers: `Vector3 v{.x = 1.0f, .y = 2.0f, .z = 3.0f}`
- `auto` for iterator types when type is obvious
- `const` correctness - mark methods `const` if they don't modify state
- `noexcept` on move constructors/assignment
- `[[nodiscard]]` on functions with meaningful return values

### Vulkan-Specific
- Use Vulkan-Hpp RAII wrappers (`vk::` namespace)
- Vulkan objects must be destroyed before context cleanup
- Use command buffers from pool for single-time operations
- Prefer dynamic rendering over VkRenderPass
- Validation layers enabled in debug builds only (`W3D_DEBUG` macro)

### Testing
- **TDD approach**: Write tests before implementing features
- Test files mirror source structure (`tests/w3d/` for `src/w3d/`)
- Use Google Test framework (`TEST_F`, `EXPECT_EQ`, etc.)
- Test executables are standalone with minimal dependencies
- Tests use fixtures in `tests/resources/`

### Comments
- **No comments** in code unless specifically requested
- Exception: Critical file format notes in W3D parser headers

## Release Process

Creating a new release follows a streamlined workflow with automated changelog generation:

### Standard Release Workflow

1. **Prepare for release**:
   - Ensure all features/fixes are merged to `dev` branch
   - All tests pass (`ctest --preset test`)
   - Application runs without issues

2. **Create release PR**:
   - Create PR from `dev` → `main`
   - **Title format (required)**: `release: vX.Y.Z` (e.g., `release: v0.2.0-alpha`)
   - Fill out the "Release Information" section in PR template
   - Add 2-3 bullet points highlighting key changes

3. **Merge to main**:
   - Get PR approval
   - Merge PR to `main`
   - GitHub Actions automatically creates a **draft release** with:
     - Auto-generated changelog (categorized by commit type)
     - Release tag (`vX.Y.Z`)
     - Installation instructions

4. **Build binaries locally**:
   
   **Linux**:
   ```bash
   ./scripts/rebuild.sh release -c clang
   mkdir VulkanW3DViewer-linux-x64-vX.Y.Z
   cp build/release/VulkanW3DViewer VulkanW3DViewer-linux-x64-vX.Y.Z/
   cp README.md LICENSE.md VulkanW3DViewer-linux-x64-vX.Y.Z/
   strip VulkanW3DViewer-linux-x64-vX.Y.Z/VulkanW3DViewer
   tar -czf VulkanW3DViewer-linux-x64-vX.Y.Z.tar.gz VulkanW3DViewer-linux-x64-vX.Y.Z
   ```
   
   **Windows** (PowerShell):
   ```powershell
   .\scripts\rebuild.ps1 release -Compiler msvc
   mkdir VulkanW3DViewer-windows-x64-vX.Y.Z
   cp build/release/VulkanW3DViewer.exe VulkanW3DViewer-windows-x64-vX.Y.Z/
   cp README.md,LICENSE.md VulkanW3DViewer-windows-x64-vX.Y.Z/
   Compress-Archive VulkanW3DViewer-windows-x64-vX.Y.Z VulkanW3DViewer-windows-x64-vX.Y.Z.zip
   ```

5. **Upload and publish**:
   - Navigate to the draft release on GitHub
   - Upload both `.tar.gz` (Linux) and `.zip` (Windows) files
   - Review auto-generated changelog, edit if needed
   - Click "Publish release"

### Version Naming Convention

Use semantic versioning with optional pre-release suffixes:
- **Stable releases**: `v1.0.0`, `v1.2.3`
- **Alpha releases**: `v0.2.0-alpha`, `v1.0.0-alpha.1`
- **Beta releases**: `v0.3.0-beta`, `v1.0.0-beta.2`
- **Release candidates**: `v1.0.0-rc1`, `v2.0.0-rc.2`

Pre-release versions are automatically marked as "pre-release" on GitHub.

### Changelog Generation

The changelog is auto-generated from commit messages using conventional commits:
- 🚀 **Features** - `feat:` commits
- 🐛 **Bug Fixes** - `fix:` commits
- ⚡ **Performance** - `perf:` commits
- 🔨 **Refactoring** - `refactor:` commits
- 📚 **Documentation** - `docs:` commits
- 🔧 **Build System** - `build:` commits
- 👷 **CI/CD** - `ci:` commits
- ✅ **Tests** - `test:` commits
- 🧹 **Chores** - `chore:` commits

Ensure PR titles follow this format for proper categorization.

### Manual/Emergency Release

For emergency releases or if automated workflow fails:
1. Navigate to Actions → Manual Release Build
2. Click "Run workflow"
3. Enter version (e.g., `v0.2.1-hotfix`)
4. Workflow builds binaries and creates release with both platforms

### Testing Changelog Locally

Install git-cliff and test changelog generation:
```bash
# Install git-cliff
cargo install git-cliff
# Or download from: https://github.com/orhun/git-cliff/releases

# Generate changelog for unreleased commits
git-cliff --config cliff.toml --unreleased

# Generate changelog between tags
git-cliff --config cliff.toml v0.1.0..HEAD
```
