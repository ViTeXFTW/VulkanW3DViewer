# Development

Guidelines and resources for contributing to VulkanW3DViewer.

## Overview

This section covers everything you need to know to contribute to the project.

## Sections

<div class="grid cards" markdown>

-   :material-format-paint:{ .lg .middle } **Code Style**

    ---

    Formatting, naming conventions, and best practices

    [:octicons-arrow-right-24: Code Style](code-style.md)

-   :material-test-tube:{ .lg .middle } **Testing**

    ---

    Writing and running tests

    [:octicons-arrow-right-24: Testing Guide](testing.md)

-   :material-source-pull:{ .lg .middle } **Contributing**

    ---

    How to submit changes and get involved

    [:octicons-arrow-right-24: Contributing](contributing.md)

</div>

## Quick Start for Contributors

### 1. Set Up Development Environment

```bash
# Clone with submodules
git clone --recursive https://github.com/ViTeXFTW/VulkanW3DViewer.git
cd VulkanW3DViewer

# Build debug version
./scripts/rebuild.sh debug

# Run tests
ctest --preset test
```

### 2. Make Changes

1. Create a feature branch
2. Write tests first (TDD)
3. Implement your changes
4. Ensure tests pass
5. Format code with clang-format

### 3. Submit Pull Request

1. Push your branch
2. Open a PR against the `dev` branch
3. Wait for CI checks
4. Address review feedback

## Development Philosophy

### RAII

All resources use RAII for automatic cleanup:

```cpp
// Good
class Buffer {
  vk::raii::Buffer buffer;  // Automatic cleanup
public:
  Buffer(VulkanContext& ctx, size_t size);
  // No explicit cleanup needed
};

// Bad
class Buffer {
  VkBuffer buffer;  // Manual cleanup required
public:
  ~Buffer() { vkDestroyBuffer(...); }  // Error-prone
};
```

### Test-Driven Development

New features should follow TDD:

1. Write failing test
2. Implement feature
3. Refactor if needed
4. Verify test passes

### Minimal Changes

Keep changes focused:

- One feature per PR
- No unrelated refactoring
- Avoid scope creep

## Project Structure

```
src/
├── core/       # Vulkan foundation
├── w3d/        # W3D parsing
├── render/     # Rendering
└── ui/         # User interface

tests/
├── w3d/        # Parser tests
├── render/     # Rendering tests
└── stubs/      # Mock implementations
```

## Key Resources

| Resource | Description |
|----------|-------------|
| [CLAUDE.md](https://github.com/ViTeXFTW/VulkanW3DViewer/blob/main/CLAUDE.md) | AI assistant guidelines |
| [.github/AGENTS.md](https://github.com/ViTeXFTW/VulkanW3DViewer/blob/main/.github/AGENTS.md) | Agent environment tips |
| `legacy/` | Original W3D reference code |

## Communication

- **Issues**: Bug reports and feature requests
- **Pull Requests**: Code contributions
- **Discussions**: General questions

## Build Configurations

| Preset | Use Case |
|--------|----------|
| `debug` | Development with symbols |
| `release` | Performance testing |
| `test` | Running test suite |

## Useful Commands

```bash
# Format all code
find src tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Build and run tests
cmake --preset test && cmake --build --preset test && ctest --preset test

# Generate compile_commands.json for IDE
cmake --preset debug  # Creates in build/debug/
```
