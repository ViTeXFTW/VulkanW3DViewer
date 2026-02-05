# AGENTS.md - VulkanW3DViewer

This file guides AI coding agents working on this Vulkan-based W3D renderer.

## Build Commands

```bash
# Configure (CMake presets)
cmake --preset debug    # Debug build
cmake --preset release  # Release build
cmake --preset test     # Build with tests enabled (BUILD_TESTING=ON)

# Build
cmake --build --preset debug
cmake --build --preset release
cmake --build --preset test

# Run main application
./build/debug/VulkanW3DViewer.exe
./build/release/VulkanW3DViewer.exe

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
