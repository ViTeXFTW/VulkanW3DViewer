# Testing

Guide to writing and running tests for VulkanW3DViewer.

## Overview

The project uses [Google Test](https://github.com/google/googletest) for unit testing. Tests are organized to mirror the `src/` directory structure.

## Running Tests

### Build and Run

```bash
# Configure with tests enabled
cmake --preset test

# Build tests
cmake --build --preset test

# Run all tests
ctest --preset test
```

### Parallel Execution

Speed up test runs with parallel execution:

```bash
ctest --preset test -j $(nproc)
```

### Verbose Output

See detailed test output:

```bash
ctest --preset test --output-on-failure
```

### Run Specific Tests

Filter by name:

```bash
# Run tests matching pattern
ctest --preset test -R "ChunkReader"

# Run a single test
ctest --preset test -R "ChunkReaderTest.ReadUint32"
```

## Test Structure

### Directory Layout

```
tests/
├── CMakeLists.txt              # Test configuration
├── w3d/                        # W3D parser tests
│   ├── test_chunk_reader.cpp
│   ├── test_loader.cpp
│   ├── test_mesh_parser.cpp
│   ├── test_hierarchy_parser.cpp
│   ├── test_animation_parser.cpp
│   └── test_hlod_parser.cpp
├── render/                     # Rendering tests
│   ├── test_animation_player.cpp
│   ├── test_bounding_box.cpp
│   ├── test_mesh_converter.cpp
│   ├── test_skeleton_pose.cpp
│   ├── test_texture_loading.cpp
│   └── raycast_test.cpp
└── stubs/                      # Mock implementations
    └── core/
        └── pipeline.hpp
```

### Naming Convention

- Test files: `test_<module>.cpp`
- Test fixtures: `<Module>Test`
- Test cases: `TEST_F(<Fixture>, <TestName>)`

## Writing Tests

### Basic Test

```cpp
#include <gtest/gtest.h>
#include "w3d/chunk_reader.hpp"

TEST(ChunkReaderTest, ReadUint32) {
  std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
  w3d::ChunkReader reader(data);

  uint32_t value = reader.read<uint32_t>();

  EXPECT_EQ(value, 0x04030201);  // Little-endian
}
```

### Test Fixtures

Use fixtures for shared setup:

```cpp
class MeshParserTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test data
    testMesh = createTestMesh();
  }

  void TearDown() override {
    // Cleanup if needed
  }

  w3d::Mesh testMesh;

  w3d::Mesh createTestMesh() {
    w3d::Mesh mesh;
    mesh.header.numVertices = 3;
    mesh.vertices = {{0,0,0}, {1,0,0}, {0,1,0}};
    return mesh;
  }
};

TEST_F(MeshParserTest, ParsesVertices) {
  EXPECT_EQ(testMesh.vertices.size(), 3);
}

TEST_F(MeshParserTest, ComputesBoundingBox) {
  auto bounds = computeBounds(testMesh);
  EXPECT_EQ(bounds.min, glm::vec3(0, 0, 0));
  EXPECT_EQ(bounds.max, glm::vec3(1, 1, 0));
}
```

### Parameterized Tests

Test multiple inputs:

```cpp
class QuaternionConversionTest :
    public ::testing::TestWithParam<std::tuple<w3d::Quaternion, glm::quat>> {
};

TEST_P(QuaternionConversionTest, ConvertsCorrectly) {
  auto [w3dQuat, expected] = GetParam();
  glm::quat result = convertQuaternion(w3dQuat);
  EXPECT_NEAR(result.w, expected.w, 0.0001f);
  EXPECT_NEAR(result.x, expected.x, 0.0001f);
  EXPECT_NEAR(result.y, expected.y, 0.0001f);
  EXPECT_NEAR(result.z, expected.z, 0.0001f);
}

INSTANTIATE_TEST_SUITE_P(
  QuaternionTests,
  QuaternionConversionTest,
  ::testing::Values(
    std::make_tuple(w3d::Quaternion{0, 0, 0, 1}, glm::quat(1, 0, 0, 0)),
    std::make_tuple(w3d::Quaternion{1, 0, 0, 0}, glm::quat(0, 1, 0, 0))
  )
);
```

### Assertions

Use appropriate assertions:

```cpp
// Value comparisons
EXPECT_EQ(a, b);       // a == b
EXPECT_NE(a, b);       // a != b
EXPECT_LT(a, b);       // a < b
EXPECT_LE(a, b);       // a <= b
EXPECT_GT(a, b);       // a > b
EXPECT_GE(a, b);       // a >= b

// Floating point (with tolerance)
EXPECT_FLOAT_EQ(a, b);        // Nearly equal (4 ULP)
EXPECT_NEAR(a, b, epsilon);   // Within epsilon

// Boolean
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// Strings
EXPECT_STREQ(a, b);    // C strings equal
EXPECT_STRNE(a, b);    // C strings not equal

// Exceptions
EXPECT_THROW(func(), std::runtime_error);
EXPECT_NO_THROW(func());

// Death tests (for crashes)
EXPECT_DEATH(func(), "error message");
```

Use `ASSERT_*` for fatal errors (stops test immediately):

```cpp
ASSERT_NE(pointer, nullptr);  // Must not be null
pointer->doSomething();       // Safe to use after assertion
```

## Mock Objects

### Stubs Directory

For testing without Vulkan, use stub implementations:

```cpp
// tests/stubs/core/pipeline.hpp
namespace w3d {
class Pipeline {
public:
  Pipeline() = default;

  // Stub methods that do nothing
  void bind(vk::CommandBuffer) {}
  void pushConstants(const void*, size_t) {}
};
}
```

### Using Stubs

```cpp
#include "stubs/core/pipeline.hpp"

TEST(RenderTest, DrawsWithPipeline) {
  w3d::Pipeline stubPipeline;  // Uses stub
  // Test drawing logic without actual Vulkan
}
```

## Test Data

### Creating Synthetic Data

```cpp
std::vector<uint8_t> createMeshChunk() {
  std::vector<uint8_t> data;

  // Chunk header
  appendU32(data, static_cast<uint32_t>(ChunkType::MESH));
  appendU32(data, 100 | 0x80000000);  // Container bit

  // Sub-chunks...

  return data;
}
```

### Binary Helpers

```cpp
void appendU32(std::vector<uint8_t>& data, uint32_t value) {
  data.push_back(value & 0xFF);
  data.push_back((value >> 8) & 0xFF);
  data.push_back((value >> 16) & 0xFF);
  data.push_back((value >> 24) & 0xFF);
}

void appendFloat(std::vector<uint8_t>& data, float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(float));
  appendU32(data, bits);
}
```

## Test-Driven Development

### TDD Workflow

1. **Write failing test**
   ```cpp
   TEST(FeatureTest, NewBehavior) {
     EXPECT_EQ(newFunction(input), expectedOutput);
   }
   ```

2. **Run test (should fail)**
   ```bash
   ctest --preset test -R "NewBehavior"
   ```

3. **Implement feature**
   ```cpp
   int newFunction(int input) {
     return input * 2;  // Implement
   }
   ```

4. **Run test (should pass)**
   ```bash
   ctest --preset test -R "NewBehavior"
   ```

5. **Refactor if needed**

### Example: Adding Animation Blending

```cpp
// 1. Write test first
TEST(AnimationBlendingTest, BlendsPositions) {
  Pivot poseA{.translation = {0, 0, 0}};
  Pivot poseB{.translation = {2, 0, 0}};

  Pivot result = blendPivots(poseA, poseB, 0.5f);

  EXPECT_NEAR(result.translation.x, 1.0f, 0.001f);
}

// 2. Implement
Pivot blendPivots(const Pivot& a, const Pivot& b, float t) {
  return Pivot{
    .translation = glm::mix(a.translation, b.translation, t),
    .rotation = glm::slerp(a.rotation, b.rotation, t)
  };
}
```

## Coverage

### Generating Coverage

```bash
# Build with coverage flags
cmake --preset test -DCMAKE_CXX_FLAGS="--coverage"
cmake --build --preset test

# Run tests
ctest --preset test

# Generate report
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage-report
```

### Viewing Coverage

Open `coverage-report/index.html` in a browser.

## Continuous Integration

Tests run automatically on pull requests via GitHub Actions:

- All tests must pass
- Coverage should not decrease significantly
- New features should include tests
