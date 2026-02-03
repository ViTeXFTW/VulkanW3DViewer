# Code Style

Coding conventions and style guidelines for VulkanW3DViewer.

## Formatting

### Clang-Format

The project uses clang-format for automatic formatting. Configuration is in `.clang-format`.

Format all files:

```bash
find src tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

### Indentation

- **2 spaces** for indentation
- No tabs
- No trailing whitespace

### Line Length

- Soft limit: 100 characters
- Hard limit: 120 characters

### Braces

Attach opening brace to statement:

```cpp
// Good
if (condition) {
  doSomething();
}

// Bad
if (condition)
{
  doSomething();
}
```

Single-line bodies allowed for short statements:

```cpp
if (x > 0) return x;
```

## Naming Conventions

### Types

- **Classes/Structs**: PascalCase
- **Enums**: PascalCase
- **Enum values**: PascalCase or SCREAMING_CASE

```cpp
class VulkanContext { };
struct MeshHeader { };
enum class ChunkType { MESH, HIERARCHY };
```

### Variables

- **Local variables**: camelCase
- **Member variables**: camelCase (no prefix)
- **Constants**: SCREAMING_CASE or kPascalCase

```cpp
class Buffer {
  vk::Buffer buffer;          // Member: camelCase
  size_t bufferSize;

  void create() {
    size_t alignedSize = ...;  // Local: camelCase
  }
};

constexpr uint32_t MAX_BONES = 256;
constexpr float kDefaultFov = 45.0f;
```

### Functions

- **Regular functions**: camelCase
- **Type traits/concepts**: snake_case (STL convention)

```cpp
void loadFile(const std::string& path);
glm::mat4 computeWorldMatrix(int boneIndex);

template<typename T>
concept is_numeric = std::is_arithmetic_v<T>;
```

### Namespaces

- **Lowercase**: `w3d`, `render`

```cpp
namespace w3d {
  // ...
}
```

### Files

- **Source files**: snake_case.cpp
- **Header files**: snake_case.hpp
- **Test files**: test_snake_case.cpp

```
vulkan_context.hpp
mesh_parser.cpp
test_chunk_reader.cpp
```

## C++ Guidelines

### Modern C++20

Use modern C++ features:

```cpp
// Structured bindings
auto [width, height] = getWindowSize();

// Designated initializers
MeshHeader header{
  .version = 4,
  .numVertices = 100
};

// Range-based algorithms
std::ranges::sort(vertices);

// std::optional
std::optional<Mesh> loadMesh(const std::string& name);

// std::span for array views
void uploadVertices(std::span<const Vertex> vertices);
```

### RAII

All resources must use RAII:

```cpp
// Good: RAII wrapper
class Buffer {
  vk::raii::Buffer buffer;
  vk::raii::DeviceMemory memory;
public:
  Buffer(VulkanContext& ctx, size_t size);
  // Automatic cleanup via destructors
};

// Bad: Manual resource management
class Buffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
public:
  ~Buffer() {
    vkDestroyBuffer(device, buffer, nullptr);  // Easy to forget
    vkFreeMemory(device, memory, nullptr);
  }
};
```

### Const Correctness

Use `const` liberally:

```cpp
class Mesh {
public:
  const std::string& getName() const { return name; }
  void setName(const std::string& newName) { name = newName; }

private:
  std::string name;
};

void processMesh(const Mesh& mesh);  // Const reference
```

### Prefer References Over Pointers

```cpp
// Good
void render(const Scene& scene);
Texture& getTexture(const std::string& name);

// Acceptable (optional/nullable)
Texture* findTexture(const std::string& name);  // May return nullptr
```

### Include Order

1. Corresponding header (for .cpp files)
2. Project headers
3. Third-party headers
4. Standard library

```cpp
// mesh_parser.cpp
#include "mesh_parser.hpp"       // 1. Corresponding header

#include "chunk_reader.hpp"      // 2. Project headers
#include "types.hpp"

#include <glm/glm.hpp>           // 3. Third-party

#include <algorithm>             // 4. Standard library
#include <vector>
```

### Include Guards

Use `#pragma once`:

```cpp
#pragma once

namespace w3d {
  // ...
}
```

## Documentation

### File Headers

Brief description at file start:

```cpp
// mesh_parser.cpp - W3D mesh chunk parsing
#include "mesh_parser.hpp"
```

### Function Comments

Document non-obvious behavior:

```cpp
/// Loads a W3D file and parses all chunks.
/// @param path Path to the W3D file
/// @return Parsed W3D data, or empty on failure
/// @throws std::runtime_error on file read errors
W3DFile load(const std::filesystem::path& path);
```

### Inline Comments

Explain "why", not "what":

```cpp
// Flip V coordinate - W3D uses bottom-left origin, Vulkan uses top-left
v = 1.0f - v;

// Skip unknown chunks to maintain forward compatibility
reader.skip(chunkSize);
```

## Error Handling

### Exceptions

Use exceptions for errors that can't be handled locally:

```cpp
if (!file.is_open()) {
  throw std::runtime_error("Failed to open file: " + path);
}
```

### Optional

Use `std::optional` for expected "not found" cases:

```cpp
std::optional<Mesh> findMesh(const std::string& name) {
  auto it = meshes.find(name);
  if (it == meshes.end()) return std::nullopt;
  return it->second;
}
```

### Assertions

Use `assert` for programmer errors (debug only):

```cpp
void setLOD(int level) {
  assert(level >= 0 && level < lodCount);  // Programmer error
  currentLOD = level;
}
```

## Performance

### Avoid Unnecessary Copies

```cpp
// Good
for (const auto& mesh : meshes) { ... }

// Bad
for (auto mesh : meshes) { ... }  // Copies each mesh
```

### Reserve Containers

```cpp
std::vector<Vertex> vertices;
vertices.reserve(header.numVertices);  // Avoid reallocations
```

### Prefer Stack Allocation

```cpp
// Good (stack)
std::array<float, 16> matrix;

// Avoid if possible (heap)
std::vector<float> matrix(16);
```
