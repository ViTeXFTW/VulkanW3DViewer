#include <algorithm>
#include <cstring>

#include "../../src/lib/formats/map/refpack.hpp"

#include <gtest/gtest.h>

using namespace map::refpack;

class RefPackTest : public ::testing::Test {
protected:
  void appendGeneralsWrapper(std::vector<uint8_t> &data, uint32_t uncompressedSize,
                             const std::vector<uint8_t> &refpackStream) {
    data.insert(data.end(), GENERALS_MAGIC, GENERALS_MAGIC + 4);
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&uncompressedSize),
                reinterpret_cast<const uint8_t *>(&uncompressedSize) + 4);
    data.insert(data.end(), refpackStream.begin(), refpackStream.end());
  }

  void appendRefPackHeader(std::vector<uint8_t> &data, uint16_t header) {
    data.push_back((header >> 8) & 0xFF);
    data.push_back(header & 0xFF);
  }

  void appendEOF(std::vector<uint8_t> &data) { data.push_back(0xFC); }
};

TEST_F(RefPackTest, IsCompressedReturnsFalseForCkMp) {
  std::vector<uint8_t> data = {'C', 'k', 'M', 'p', 0, 0, 0, 0};

  EXPECT_FALSE(isCompressed(data));
}

TEST_F(RefPackTest, IsCompressedReturnsTrueForGeneralsWrapper) {
  std::vector<uint8_t> data;
  data.insert(data.end(), GENERALS_MAGIC, GENERALS_MAGIC + 4);
  data.insert(data.end(), 4, 0);
  data.insert(data.end(), {0x10, 0xFB, 0x00, 0x00});

  EXPECT_TRUE(isCompressed(data));
}

TEST_F(RefPackTest, IsCompressedReturnsFalseForTooSmallData) {
  std::vector<uint8_t> data = {'E', 'A', 'R', '\0'};

  EXPECT_FALSE(isCompressed(data));
}

TEST_F(RefPackTest, IsCompressedReturnsFalseForEmptyData) {
  std::vector<uint8_t> data;

  EXPECT_FALSE(isCompressed(data));
}

TEST_F(RefPackTest, GetUncompressedSizeReadsCorrectValue) {
  std::vector<uint8_t> data;
  data.insert(data.end(), GENERALS_MAGIC, GENERALS_MAGIC + 4);
  uint32_t expectedSize = 12345;
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&expectedSize),
              reinterpret_cast<const uint8_t *>(&expectedSize) + 4);

  EXPECT_EQ(getUncompressedSize(data), 12345u);
}

TEST_F(RefPackTest, GetUncompressedSizeForUncompressedDataReturnsZero) {
  std::vector<uint8_t> data = {'C', 'k', 'M', 'p', 0, 0, 0, 0};

  EXPECT_EQ(getUncompressedSize(data), 0u);
}

TEST_F(RefPackTest, GetUncompressedSizeForTooSmallDataReturnsZero) {
  std::vector<uint8_t> data = {'E', 'A', 'R', '\0'};

  EXPECT_EQ(getUncompressedSize(data), 0u);
}

TEST_F(RefPackTest, GetUncompressedSizeForZeroSize) {
  std::vector<uint8_t> data;
  data.insert(data.end(), GENERALS_MAGIC, GENERALS_MAGIC + 4);
  uint32_t zeroSize = 0;
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&zeroSize),
              reinterpret_cast<const uint8_t *>(&zeroSize) + 4);

  EXPECT_EQ(getUncompressedSize(data), 0u);
}

TEST_F(RefPackTest, GetUncompressedSizeForMaxValidSize) {
  std::vector<uint8_t> data;
  data.insert(data.end(), GENERALS_MAGIC, GENERALS_MAGIC + 4);
  uint32_t maxSize = 64 * 1024 * 1024;
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&maxSize),
              reinterpret_cast<const uint8_t *>(&maxSize) + 4);

  EXPECT_EQ(getUncompressedSize(data), 64 * 1024 * 1024u);
}

TEST_F(RefPackTest, DecompressHandles0x10FBHeader) {
  std::vector<uint8_t> refpack;
  appendRefPackHeader(refpack, 0x10FB);
  appendEOF(refpack);

  std::vector<uint8_t> data;
  appendGeneralsWrapper(data, 0, refpack);

  std::string error;
  auto result = decompress(data, &error);

  ASSERT_TRUE(result.has_value()) << error;
  EXPECT_TRUE(result->empty());
}

TEST_F(RefPackTest, DecompressHandles0x90FBHeader) {
  std::vector<uint8_t> refpack;
  appendRefPackHeader(refpack, 0x90FB);
  appendEOF(refpack);

  std::vector<uint8_t> data;
  appendGeneralsWrapper(data, 0, refpack);

  std::string error;
  auto result = decompress(data, &error);

  ASSERT_TRUE(result.has_value()) << error;
  EXPECT_TRUE(result->empty());
}

TEST_F(RefPackTest, DecompressHandles0x11FBHeader) {
  std::vector<uint8_t> refpack;
  appendRefPackHeader(refpack, 0x11FB);
  appendEOF(refpack);

  std::vector<uint8_t> data;
  appendGeneralsWrapper(data, 0, refpack);

  std::string error;
  auto result = decompress(data, &error);

  ASSERT_TRUE(result.has_value()) << error;
  EXPECT_TRUE(result->empty());
}

TEST_F(RefPackTest, DecompressHandles0x91FBHeader) {
  std::vector<uint8_t> refpack;
  appendRefPackHeader(refpack, 0x91FB);
  appendEOF(refpack);

  std::vector<uint8_t> data;
  appendGeneralsWrapper(data, 0, refpack);

  std::string error;
  auto result = decompress(data, &error);

  ASSERT_TRUE(result.has_value()) << error;
  EXPECT_TRUE(result->empty());
}

TEST_F(RefPackTest, DecompressFailsOnTruncatedData) {
  std::vector<uint8_t> data;
  data.insert(data.end(), GENERALS_MAGIC, GENERALS_MAGIC + 4);
  uint32_t size = 100;
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&size),
              reinterpret_cast<const uint8_t *>(&size) + 4);
  data.insert(data.end(), {0x10, 0xFB, 0x00, 0x00});

  std::string error;
  auto result = decompress(data, &error);

  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(error.empty());
}

TEST_F(RefPackTest, DecompressFailsOnInvalidHeader) {
  std::vector<uint8_t> data;
  data.insert(data.end(), GENERALS_MAGIC, GENERALS_MAGIC + 4);
  uint32_t size = 10;
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&size),
              reinterpret_cast<const uint8_t *>(&size) + 4);
  data.insert(data.end(), {0xFF, 0xFF, 0x00, 0x00});

  std::string error;
  auto result = decompress(data, &error);

  EXPECT_FALSE(result.has_value()) << "Error: " << error;
  EXPECT_FALSE(error.empty()) << "Error: " << error;
}

TEST_F(RefPackTest, DecompressFailsOnNonCompressedData) {
  std::vector<uint8_t> data = {'C', 'k', 'M', 'p', 0, 0, 0, 0};

  std::string error;
  auto result = decompress(data, &error);

  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(error.empty());
}

TEST_F(RefPackTest, DecompressFailsOnTooSmallData) {
  std::vector<uint8_t> data = {'E', 'A', 'R', '\0'};

  std::string error;
  auto result = decompress(data, &error);

  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(error.empty());
}

TEST_F(RefPackTest, DecompressFailsOnMissingMagic) {
  std::vector<uint8_t> data;
  data.insert(data.end(), {'E', 'A', 'X', '\0', 0, 0, 0, 0, 0x10, 0xFB, 0x00, 0x00});

  std::string error;
  auto result = decompress(data, &error);

  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(error.empty());
  EXPECT_NE(error.find("EAR compression magic"), std::string::npos);
}

TEST_F(RefPackTest, DecompressFailsOnTooLargeUncompressedSize) {
  std::vector<uint8_t> refpack;
  appendRefPackHeader(refpack, 0x10FB);
  appendEOF(refpack);

  std::vector<uint8_t> data;
  uint32_t tooLargeSize = 64 * 1024 * 1024 + 1;
  appendGeneralsWrapper(data, tooLargeSize, refpack);

  std::string error;
  auto result = decompress(data, &error);

  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(error.empty());
  EXPECT_NE(error.find("Uncompressed size too large"), std::string::npos);
}

TEST_F(RefPackTest, DecompressReturnsEmptyVectorForZeroSize) {
  std::vector<uint8_t> refpack;
  appendRefPackHeader(refpack, 0x10FB);
  appendEOF(refpack);

  std::vector<uint8_t> data;
  appendGeneralsWrapper(data, 0, refpack);

  std::string error;
  auto result = decompress(data, &error);

  ASSERT_TRUE(result.has_value()) << error;
  EXPECT_TRUE(result->empty());
}

TEST_F(RefPackTest, DecompressAllHeaderVariantsWithZeroSize) {
  std::vector<uint16_t> headers = {0x10FB, 0x90FB, 0x11FB, 0x91FB};

  for (uint16_t header : headers) {
    std::vector<uint8_t> refpack;
    appendRefPackHeader(refpack, header);
    appendEOF(refpack);

    std::vector<uint8_t> data;
    appendGeneralsWrapper(data, 0, refpack);

    std::string error;
    auto result = decompress(data, &error);

    ASSERT_TRUE(result.has_value())
        << "Failed for header 0x" << std::hex << header << ": " << error;
    EXPECT_TRUE(result->empty()) << "Expected empty for header 0x" << std::hex << header;
  }
}
