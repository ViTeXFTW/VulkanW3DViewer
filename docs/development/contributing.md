# Contributing

Guidelines for contributing to VulkanW3DViewer.

## Getting Started

### Prerequisites

1. Read the [Code Style](code-style.md) guide
2. Understand the [Testing](testing.md) practices
3. Review the [Architecture](../architecture/index.md) documentation

### Development Setup

```bash
# Fork and clone
git clone https://github.com/YOUR_USERNAME/VulkanW3DViewer.git
cd VulkanW3DViewer

# Add upstream remote
git remote add upstream https://github.com/ViTeXFTW/VulkanW3DViewer.git

# Initialize submodules
git submodule update --init --recursive

# Build
./scripts/rebuild.sh debug
```

## Workflow

### 1. Create a Branch

Branch from `main` for your work:

```bash
git checkout main
git pull upstream main
git checkout -b feature/your-feature-name
```

Branch naming conventions:

| Prefix | Use |
|--------|-----|
| `feature/` | New features |
| `fix/` | Bug fixes |
| `docs/` | Documentation |
| `refactor/` | Code refactoring |
| `test/` | Test improvements |

### 2. Make Changes

#### Write Tests First

Follow TDD - write failing tests before implementing:

```cpp
// tests/w3d/test_new_feature.cpp
TEST(NewFeatureTest, DoesWhatIExpect) {
  auto result = newFeature(input);
  EXPECT_EQ(result, expected);
}
```

#### Implement Feature

Make minimal, focused changes:

```cpp
// src/w3d/new_feature.cpp
Result newFeature(Input input) {
  // Implementation
}
```

#### Format Code

```bash
clang-format -i src/w3d/new_feature.cpp
```

#### Run Tests

```bash
cmake --preset test && cmake --build --preset test && ctest --preset test
```

### 3. Commit Changes

Write clear commit messages:

```bash
git add src/w3d/new_feature.cpp tests/w3d/test_new_feature.cpp
git commit -m "feat(w3d): add new feature for X

- Implement Y functionality
- Add tests for Z edge cases
- Update documentation"
```

#### Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**

| Type | Description |
|------|-------------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation |
| `style` | Formatting |
| `refactor` | Code restructure |
| `test` | Adding tests |
| `chore` | Maintenance |
| `perf` | Performance |

**Examples:**

```
feat(render): add skeleton visualization

- Draw lines between bone positions
- Support color configuration
- Add toggle in display panel

fix(w3d): correct quaternion conversion order

W3D uses (x,y,z,w) but GLM expects (w,x,y,z).
This fixes animation rotation issues.

Fixes #42

docs: update W3D format documentation

Add missing chunk types and clarify UV coordinate handling.
```

### 4. Push and Create PR

```bash
git push -u origin feature/your-feature-name
```

Then create a Pull Request on GitHub.

## Pull Request Guidelines

### PR Title

Use the same format as commit messages:

```
feat(render): add skeleton visualization
```

### PR Description

Include:

1. **Summary**: What does this PR do?
2. **Motivation**: Why is this change needed?
3. **Changes**: List of key changes
4. **Testing**: How was this tested?
5. **Screenshots**: If visual changes

**Template:**

```markdown
## Summary
Brief description of changes.

## Motivation
Why this change is needed.

## Changes
- Change 1
- Change 2
- Change 3

## Testing
- [ ] Unit tests pass
- [ ] Manual testing performed
- [ ] Tested with sample W3D files

## Screenshots
(if applicable)
```

### PR Checklist

Before submitting:

- [ ] Tests pass locally
- [ ] Code is formatted with clang-format
- [ ] No compiler warnings
- [ ] Documentation updated if needed
- [ ] Commit messages follow convention
- [ ] PR description is complete

## Code Review

### What Reviewers Look For

- **Correctness**: Does it work as intended?
- **Tests**: Are there sufficient tests?
- **Style**: Does it follow conventions?
- **Performance**: Any obvious issues?
- **Security**: Any vulnerabilities?
- **Documentation**: Is it clear?

### Responding to Feedback

- Address all comments
- Push fixes as new commits (don't force-push during review)
- Mark conversations as resolved when fixed
- Ask for clarification if needed

### After Approval

Maintainers will:

1. Squash and merge (if multiple commits)
2. Or rebase and merge (if clean history)

## Types of Contributions

### Bug Reports

File issues with:

- Clear title describing the bug
- Steps to reproduce
- Expected vs actual behavior
- W3D file (if format-related)
- System information

### Feature Requests

Open a discussion first to:

- Describe the feature
- Explain the use case
- Discuss implementation approach

### Documentation

Documentation improvements are welcome:

- Fix typos and errors
- Add missing information
- Improve examples
- Translate (future)

### Code Contributions

Good first issues are labeled:

- `good first issue` - Simple tasks
- `help wanted` - Needs contributors
- `documentation` - Docs improvements

## Development Tips

### IDE Setup

**VS Code:**

```json
// .vscode/settings.json
{
  "cmake.configureOnOpen": true,
  "cmake.buildDirectory": "${workspaceFolder}/build/${buildType}",
  "editor.formatOnSave": true,
  "C_Cpp.clang_format_style": "file"
}
```

**CLion:**

- Use the CMake project configuration
- Enable clang-format on save

### Debugging

```bash
# Build debug version
cmake --preset debug
cmake --build --preset debug

# Run with debugger
gdb ./build/debug/VulkanW3DViewer
```

### Performance Profiling

```bash
# Build release with debug info
cmake --preset release -DCMAKE_CXX_FLAGS="-g"
cmake --build --preset release

# Profile with perf
perf record ./build/release/VulkanW3DViewer model.w3d
perf report
```

## Getting Help

- **Questions**: Open a GitHub Discussion
- **Bugs**: File an Issue
- **Chat**: (if applicable, add Discord/Matrix link)

## Recognition

Contributors are acknowledged in:

- Release notes
- Contributors file (future)
- GitHub contributor graph

Thank you for contributing!
