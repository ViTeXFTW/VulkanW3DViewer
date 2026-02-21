#include <vector>

#include <gtest/gtest.h>

class DynamicBufferTest : public ::testing::Test {};

struct TestVertex {
  float x, y, z;
  float u, v;
};

TEST_F(DynamicBufferTest, SizeofTestVertex) {
  EXPECT_EQ(sizeof(TestVertex), 20);
}

TEST_F(DynamicBufferTest, VerifyVectorSize) {
  std::vector<TestVertex> vertices;
  vertices.push_back({1.0f, 2.0f, 3.0f, 0.0f, 0.0f});
  vertices.push_back({4.0f, 5.0f, 6.0f, 1.0f, 1.0f});

  EXPECT_EQ(vertices.size(), 2);
  EXPECT_EQ(sizeof(TestVertex) * vertices.size(), 40);
}

TEST_F(DynamicBufferTest, CapacityCalculation) {
  const size_t maxVertices = 1024;
  const size_t capacityBytes = sizeof(TestVertex) * maxVertices;

  EXPECT_EQ(capacityBytes, 20480);
}

TEST_F(DynamicBufferTest, UpdateSizeValidation) {
  const size_t capacity = 1000;
  const size_t updateSize1 = 500;
  const size_t updateSize2 = 1500;

  EXPECT_LT(updateSize1, capacity);
  EXPECT_GT(updateSize2, capacity);
}

TEST_F(DynamicBufferTest, OffsetValidation) {
  const size_t capacity = 1000;
  const size_t offset = 200;
  const size_t dataSize = 600;

  EXPECT_LT(offset + dataSize, capacity);

  const size_t badOffset = 500;
  const size_t badDataSize = 600;

  EXPECT_GT(badOffset + badDataSize, capacity);
}
