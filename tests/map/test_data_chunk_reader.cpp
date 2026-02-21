#include <cstring>

#include "../../src/lib/formats/map/data_chunk_reader.hpp"

#include <gtest/gtest.h>

using namespace map;

class DataChunkReaderTest : public ::testing::Test {
protected:
  std::vector<uint8_t> buildTOC(const std::vector<std::pair<std::string, uint32_t>> &entries) {
    std::vector<uint8_t> data;

    uint32_t magic = DATA_CHUNK_MAGIC;
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&magic),
                reinterpret_cast<const uint8_t *>(&magic) + 4);

    int32_t count = static_cast<int32_t>(entries.size());
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&count),
                reinterpret_cast<const uint8_t *>(&count) + 4);

    for (const auto &[name, id] : entries) {
      uint8_t nameLen = static_cast<uint8_t>(name.size());
      data.push_back(nameLen);
      data.insert(data.end(), name.begin(), name.end());
      data.insert(data.end(), reinterpret_cast<const uint8_t *>(&id),
                  reinterpret_cast<const uint8_t *>(&id) + 4);
    }

    return data;
  }

  void appendChunkHeader(std::vector<uint8_t> &data, uint32_t id, uint16_t version,
                         int32_t dataSize) {
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&id),
                reinterpret_cast<const uint8_t *>(&id) + 4);
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&version),
                reinterpret_cast<const uint8_t *>(&version) + 2);
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&dataSize),
                reinterpret_cast<const uint8_t *>(&dataSize) + 4);
  }

  void appendInt32(std::vector<uint8_t> &data, int32_t value) {
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&value),
                reinterpret_cast<const uint8_t *>(&value) + 4);
  }

  void appendFloat(std::vector<uint8_t> &data, float value) {
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&value),
                reinterpret_cast<const uint8_t *>(&value) + 4);
  }

  void appendAsciiString(std::vector<uint8_t> &data, const std::string &str) {
    uint16_t len = static_cast<uint16_t>(str.size());
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&len),
                reinterpret_cast<const uint8_t *>(&len) + 2);
    data.insert(data.end(), str.begin(), str.end());
  }
};

TEST_F(DataChunkReaderTest, ParsesValidTOC) {
  auto data = buildTOC({
      {"HeightMapData", 1},
      {"BlendTileData", 2},
      {"ObjectsList",   3}
  });

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);

  EXPECT_FALSE(error.has_value());
  EXPECT_EQ(*reader.lookupName(1), "HeightMapData");
  EXPECT_EQ(*reader.lookupName(2), "BlendTileData");
  EXPECT_EQ(*reader.lookupName(3), "ObjectsList");
  EXPECT_FALSE(reader.lookupName(999).has_value());
}

TEST_F(DataChunkReaderTest, RejectsInvalidMagic) {
  std::vector<uint8_t> data;
  uint32_t badMagic = 0xDEADBEEF;
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&badMagic),
              reinterpret_cast<const uint8_t *>(&badMagic) + 4);
  int32_t count = 0;
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&count),
              reinterpret_cast<const uint8_t *>(&count) + 4);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);

  ASSERT_TRUE(error.has_value());
  EXPECT_NE(error->find("Invalid magic"), std::string::npos);
}

TEST_F(DataChunkReaderTest, RejectsTooSmallFile) {
  std::vector<uint8_t> data{0x43, 0x6B, 0x4D};

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);

  ASSERT_TRUE(error.has_value());
  EXPECT_NE(error->find("too small"), std::string::npos);
}

TEST_F(DataChunkReaderTest, ReadsChunkHeader) {
  auto data = buildTOC({
      {"TestChunk", 1}
  });
  appendChunkHeader(data, 1, 3, 12);
  data.insert(data.end(), 12, 0);

  DataChunkReader reader;
  auto error = reader.loadFromMemory(data);
  ASSERT_FALSE(error.has_value());

  auto header = reader.openChunk();
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->id, 1u);
  EXPECT_EQ(header->version, 3);
  EXPECT_EQ(header->dataSize, 12);
}

TEST_F(DataChunkReaderTest, ReadsByte) {
  auto data = buildTOC({
      {"Test", 1}
  });
  appendChunkHeader(data, 1, 1, 3);
  data.push_back(0x42);
  data.push_back(0xFF);
  data.push_back(0x00);

  DataChunkReader reader;
  reader.loadFromMemory(data);
  reader.openChunk();

  auto b1 = reader.readByte();
  ASSERT_TRUE(b1.has_value());
  EXPECT_EQ(*b1, 0x42);

  auto b2 = reader.readByte();
  ASSERT_TRUE(b2.has_value());
  EXPECT_EQ(*b2, -1);

  auto b3 = reader.readByte();
  ASSERT_TRUE(b3.has_value());
  EXPECT_EQ(*b3, 0);
}

TEST_F(DataChunkReaderTest, ReadsInt32) {
  auto data = buildTOC({
      {"Test", 1}
  });
  appendChunkHeader(data, 1, 1, 8);
  appendInt32(data, 0x12345678);
  appendInt32(data, -42);

  DataChunkReader reader;
  reader.loadFromMemory(data);
  reader.openChunk();

  auto i1 = reader.readInt();
  ASSERT_TRUE(i1.has_value());
  EXPECT_EQ(*i1, 0x12345678);

  auto i2 = reader.readInt();
  ASSERT_TRUE(i2.has_value());
  EXPECT_EQ(*i2, -42);
}

TEST_F(DataChunkReaderTest, ReadsFloat) {
  auto data = buildTOC({
      {"Test", 1}
  });
  appendChunkHeader(data, 1, 1, 8);
  appendFloat(data, 3.14159f);
  appendFloat(data, -2.71828f);

  DataChunkReader reader;
  reader.loadFromMemory(data);
  reader.openChunk();

  auto f1 = reader.readReal();
  ASSERT_TRUE(f1.has_value());
  EXPECT_FLOAT_EQ(*f1, 3.14159f);

  auto f2 = reader.readReal();
  ASSERT_TRUE(f2.has_value());
  EXPECT_FLOAT_EQ(*f2, -2.71828f);
}

TEST_F(DataChunkReaderTest, ReadsAsciiString) {
  auto data = buildTOC({
      {"Test", 1}
  });
  appendChunkHeader(data, 1, 1, 17);
  appendAsciiString(data, "Hello");
  appendAsciiString(data, "World");

  DataChunkReader reader;
  reader.loadFromMemory(data);
  reader.openChunk();

  auto s1 = reader.readAsciiString();
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(*s1, "Hello");

  auto s2 = reader.readAsciiString();
  ASSERT_TRUE(s2.has_value());
  EXPECT_EQ(*s2, "World");
}

TEST_F(DataChunkReaderTest, ReadsEmptyAsciiString) {
  auto data = buildTOC({
      {"Test", 1}
  });
  appendChunkHeader(data, 1, 1, 2);
  appendAsciiString(data, "");

  DataChunkReader reader;
  reader.loadFromMemory(data);
  reader.openChunk();

  auto s = reader.readAsciiString();
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(*s, "");
}

TEST_F(DataChunkReaderTest, ReadsDict) {
  auto data = buildTOC({
      {"Test", 1},
      {"key1", 2},
      {"key2", 3},
      {"key3", 4}
  });

  appendChunkHeader(data, 1, 1, 0);
  size_t chunkSizePos = data.size() - 4;

  uint16_t pairCount = 3;
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&pairCount),
              reinterpret_cast<const uint8_t *>(&pairCount) + 2);

  int32_t keyAndType1 = (2 << 8) | static_cast<int32_t>(DataType::Int);
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&keyAndType1),
              reinterpret_cast<const uint8_t *>(&keyAndType1) + 4);
  appendInt32(data, 42);

  int32_t keyAndType2 = (3 << 8) | static_cast<int32_t>(DataType::Real);
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&keyAndType2),
              reinterpret_cast<const uint8_t *>(&keyAndType2) + 4);
  appendFloat(data, 3.14f);

  int32_t keyAndType3 = (4 << 8) | static_cast<int32_t>(DataType::AsciiString);
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&keyAndType3),
              reinterpret_cast<const uint8_t *>(&keyAndType3) + 4);
  appendAsciiString(data, "test");

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  reader.loadFromMemory(data);
  reader.openChunk();

  auto dict = reader.readDict();
  ASSERT_TRUE(dict.has_value());
  EXPECT_EQ(dict->size(), 3u);

  EXPECT_EQ(dict->at("key1").type, DataType::Int);
  EXPECT_EQ(dict->at("key1").intValue, 42);

  EXPECT_EQ(dict->at("key2").type, DataType::Real);
  EXPECT_FLOAT_EQ(dict->at("key2").realValue, 3.14f);

  EXPECT_EQ(dict->at("key3").type, DataType::AsciiString);
  EXPECT_EQ(dict->at("key3").stringValue, "test");
}

TEST_F(DataChunkReaderTest, ReadsBoolInDict) {
  auto data = buildTOC({
      {"Test",    1},
      {"enabled", 2}
  });

  appendChunkHeader(data, 1, 1, 0);
  size_t chunkSizePos = data.size() - 4;

  uint16_t pairCount = 1;
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&pairCount),
              reinterpret_cast<const uint8_t *>(&pairCount) + 2);

  int32_t keyAndType = (2 << 8) | static_cast<int32_t>(DataType::Bool);
  data.insert(data.end(), reinterpret_cast<const uint8_t *>(&keyAndType),
              reinterpret_cast<const uint8_t *>(&keyAndType) + 4);
  data.push_back(1);

  int32_t actualSize = static_cast<int32_t>(data.size() - chunkSizePos - 4);
  std::memcpy(&data[chunkSizePos], &actualSize, 4);

  DataChunkReader reader;
  reader.loadFromMemory(data);
  reader.openChunk();

  auto dict = reader.readDict();
  ASSERT_TRUE(dict.has_value());
  EXPECT_EQ(dict->at("enabled").type, DataType::Bool);
  EXPECT_TRUE(dict->at("enabled").boolValue);
}

TEST_F(DataChunkReaderTest, HandlesNestedChunks) {
  auto data = buildTOC({
      {"Parent", 1},
      {"Child",  2}
  });

  appendChunkHeader(data, 1, 1, 0);
  size_t parentSizePos = data.size() - 4;

  appendChunkHeader(data, 2, 1, 4);
  appendInt32(data, 999);

  int32_t parentSize = static_cast<int32_t>(data.size() - parentSizePos - 4);
  std::memcpy(&data[parentSizePos], &parentSize, 4);

  DataChunkReader reader;
  reader.loadFromMemory(data);

  auto parent = reader.openChunk();
  ASSERT_TRUE(parent.has_value());
  EXPECT_EQ(*reader.lookupName(parent->id), "Parent");

  auto child = reader.openChunk();
  ASSERT_TRUE(child.has_value());
  EXPECT_EQ(*reader.lookupName(child->id), "Child");

  auto value = reader.readInt();
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, 999);

  reader.closeChunk();
  reader.closeChunk();
}

TEST_F(DataChunkReaderTest, SkipsUnreadDataOnClose) {
  auto data = buildTOC({
      {"Test", 1}
  });
  appendChunkHeader(data, 1, 1, 12);
  appendInt32(data, 100);
  appendInt32(data, 200);
  appendInt32(data, 300);

  appendChunkHeader(data, 1, 1, 4);
  appendInt32(data, 400);

  DataChunkReader reader;
  reader.loadFromMemory(data);

  auto chunk1 = reader.openChunk();
  ASSERT_TRUE(chunk1.has_value());

  auto val1 = reader.readInt();
  ASSERT_TRUE(val1.has_value());
  EXPECT_EQ(*val1, 100);

  reader.closeChunk();

  auto chunk2 = reader.openChunk();
  ASSERT_TRUE(chunk2.has_value());

  auto val2 = reader.readInt();
  ASSERT_TRUE(val2.has_value());
  EXPECT_EQ(*val2, 400);
}

TEST_F(DataChunkReaderTest, DetectsEndOfFile) {
  auto data = buildTOC({
      {"Test", 1}
  });
  appendChunkHeader(data, 1, 1, 4);
  appendInt32(data, 42);

  DataChunkReader reader;
  reader.loadFromMemory(data);

  EXPECT_FALSE(reader.atEnd());

  reader.openChunk();
  reader.readInt();
  reader.closeChunk();

  EXPECT_TRUE(reader.atEnd());
}
