#include "w3d/chunk_reader.hpp"

#include <gtest/gtest.h>

using namespace w3d;

class ChunkReaderTest : public ::testing::Test {
protected:
  // Helper to create test data
  static std::vector<uint8_t> makeData(std::initializer_list<uint8_t> bytes) {
    return std::vector<uint8_t>(bytes);
  }
};

// =============================================================================
// Basic Position/Size Tests
// =============================================================================

TEST_F(ChunkReaderTest, EmptyData) {
  std::vector<uint8_t> data;
  ChunkReader reader(data);

  EXPECT_EQ(reader.position(), 0);
  EXPECT_EQ(reader.size(), 0);
  EXPECT_EQ(reader.remaining(), 0);
  EXPECT_TRUE(reader.atEnd());
}

TEST_F(ChunkReaderTest, InitialPosition) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  EXPECT_EQ(reader.position(), 0);
  EXPECT_EQ(reader.size(), 4);
  EXPECT_EQ(reader.remaining(), 4);
  EXPECT_FALSE(reader.atEnd());
}

// =============================================================================
// Seek Tests
// =============================================================================

TEST_F(ChunkReaderTest, SeekToValidPosition) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  reader.seek(2);
  EXPECT_EQ(reader.position(), 2);
  EXPECT_EQ(reader.remaining(), 2);
}

TEST_F(ChunkReaderTest, SeekToEnd) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  reader.seek(4);
  EXPECT_EQ(reader.position(), 4);
  EXPECT_TRUE(reader.atEnd());
}

TEST_F(ChunkReaderTest, SeekPastEndThrows) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  EXPECT_THROW(reader.seek(5), ParseError);
}

TEST_F(ChunkReaderTest, SeekBackwards) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  reader.seek(3);
  reader.seek(1);
  EXPECT_EQ(reader.position(), 1);
}

// =============================================================================
// Skip Tests
// =============================================================================

TEST_F(ChunkReaderTest, SkipValidAmount) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  reader.skip(2);
  EXPECT_EQ(reader.position(), 2);
}

TEST_F(ChunkReaderTest, SkipToExactEnd) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  reader.skip(4);
  EXPECT_TRUE(reader.atEnd());
}

TEST_F(ChunkReaderTest, SkipPastEndThrows) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  EXPECT_THROW(reader.skip(5), ParseError);
}

TEST_F(ChunkReaderTest, SkipZeroBytes) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  reader.skip(0);
  EXPECT_EQ(reader.position(), 0);
}

// =============================================================================
// Read Primitive Tests
// =============================================================================

TEST_F(ChunkReaderTest, ReadUint8) {
  auto data = makeData({0xAB, 0xCD});
  ChunkReader reader(data);

  EXPECT_EQ(reader.read<uint8_t>(), 0xAB);
  EXPECT_EQ(reader.read<uint8_t>(), 0xCD);
  EXPECT_TRUE(reader.atEnd());
}

TEST_F(ChunkReaderTest, ReadUint16LittleEndian) {
  auto data = makeData({0x34, 0x12}); // Little-endian 0x1234
  ChunkReader reader(data);

  EXPECT_EQ(reader.read<uint16_t>(), 0x1234);
}

TEST_F(ChunkReaderTest, ReadUint32LittleEndian) {
  auto data = makeData({0x78, 0x56, 0x34, 0x12}); // Little-endian 0x12345678
  ChunkReader reader(data);

  EXPECT_EQ(reader.read<uint32_t>(), 0x12345678);
}

TEST_F(ChunkReaderTest, ReadFloat) {
  // IEEE 754 representation of 1.0f
  auto data = makeData({0x00, 0x00, 0x80, 0x3F});
  ChunkReader reader(data);

  EXPECT_FLOAT_EQ(reader.read<float>(), 1.0f);
}

TEST_F(ChunkReaderTest, ReadPastEndThrows) {
  auto data = makeData({0x01, 0x02});
  ChunkReader reader(data);

  EXPECT_THROW(reader.read<uint32_t>(), ParseError);
}

// =============================================================================
// Read Array Tests
// =============================================================================

TEST_F(ChunkReaderTest, ReadArrayEmpty) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  auto result = reader.readArray<uint8_t>(0);
  EXPECT_TRUE(result.empty());
  EXPECT_EQ(reader.position(), 0);
}

TEST_F(ChunkReaderTest, ReadArrayUint8) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  auto result = reader.readArray<uint8_t>(4);
  EXPECT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], 0x01);
  EXPECT_EQ(result[3], 0x04);
}

TEST_F(ChunkReaderTest, ReadArrayUint32) {
  auto data = makeData({
      0x01, 0x00, 0x00, 0x00, // 1
      0x02, 0x00, 0x00, 0x00, // 2
  });
  ChunkReader reader(data);

  auto result = reader.readArray<uint32_t>(2);
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
}

TEST_F(ChunkReaderTest, ReadArrayPastEndThrows) {
  auto data = makeData({0x01, 0x02});
  ChunkReader reader(data);

  EXPECT_THROW(reader.readArray<uint32_t>(2), ParseError);
}

// =============================================================================
// String Tests
// =============================================================================

TEST_F(ChunkReaderTest, ReadFixedStringFull) {
  auto data = makeData({'H', 'e', 'l', 'l', 'o'});
  ChunkReader reader(data);

  auto str = reader.readFixedString(5);
  EXPECT_EQ(str, "Hello");
}

TEST_F(ChunkReaderTest, ReadFixedStringWithNullPadding) {
  auto data = makeData({'H', 'i', '\0', '\0', '\0'});
  ChunkReader reader(data);

  auto str = reader.readFixedString(5);
  EXPECT_EQ(str, "Hi");
}

TEST_F(ChunkReaderTest, ReadFixedStringAllNulls) {
  auto data = makeData({'\0', '\0', '\0', '\0'});
  ChunkReader reader(data);

  auto str = reader.readFixedString(4);
  EXPECT_EQ(str, "");
}

TEST_F(ChunkReaderTest, ReadNullStringNormal) {
  auto data = makeData({'T', 'e', 's', 't', '\0', 'X'});
  ChunkReader reader(data);

  auto str = reader.readNullString(10);
  EXPECT_EQ(str, "Test");
  EXPECT_EQ(reader.position(), 5); // Stopped at null
}

TEST_F(ChunkReaderTest, ReadNullStringHitsMaxLen) {
  auto data = makeData({'A', 'B', 'C', 'D', 'E'});
  ChunkReader reader(data);

  auto str = reader.readNullString(3);
  EXPECT_EQ(str, "ABC");
}

TEST_F(ChunkReaderTest, ReadRemainingString) {
  auto data = makeData({'W', '3', 'D', '\0', 'X', 'Y'});
  ChunkReader reader(data);

  auto str = reader.readRemainingString();
  EXPECT_EQ(str, "W3D");
  EXPECT_EQ(reader.position(), 4); // Stopped at null
}

// =============================================================================
// Vector/Quaternion/Color Tests
// =============================================================================

TEST_F(ChunkReaderTest, ReadVector3) {
  // Three floats: 1.0, 2.0, 3.0
  auto data = makeData({
      0x00, 0x00, 0x80, 0x3F, // 1.0f
      0x00, 0x00, 0x00, 0x40, // 2.0f
      0x00, 0x00, 0x40, 0x40, // 3.0f
  });
  ChunkReader reader(data);

  auto v = reader.readVector3();
  EXPECT_FLOAT_EQ(v.x, 1.0f);
  EXPECT_FLOAT_EQ(v.y, 2.0f);
  EXPECT_FLOAT_EQ(v.z, 3.0f);
}

TEST_F(ChunkReaderTest, ReadVector2) {
  auto data = makeData({
      0x00, 0x00, 0x80, 0x3F, // 1.0f
      0x00, 0x00, 0x00, 0x40, // 2.0f
  });
  ChunkReader reader(data);

  auto v = reader.readVector2();
  EXPECT_FLOAT_EQ(v.u, 1.0f);
  EXPECT_FLOAT_EQ(v.v, 2.0f);
}

TEST_F(ChunkReaderTest, ReadQuaternion) {
  auto data = makeData({
      0x00, 0x00, 0x00, 0x00, // 0.0f (x)
      0x00, 0x00, 0x00, 0x00, // 0.0f (y)
      0x00, 0x00, 0x00, 0x00, // 0.0f (z)
      0x00, 0x00, 0x80, 0x3F, // 1.0f (w)
  });
  ChunkReader reader(data);

  auto q = reader.readQuaternion();
  EXPECT_FLOAT_EQ(q.x, 0.0f);
  EXPECT_FLOAT_EQ(q.y, 0.0f);
  EXPECT_FLOAT_EQ(q.z, 0.0f);
  EXPECT_FLOAT_EQ(q.w, 1.0f);
}

TEST_F(ChunkReaderTest, ReadRGBWithPadding) {
  auto data = makeData({0xFF, 0x80, 0x40, 0x00}); // R=255, G=128, B=64, padding
  ChunkReader reader(data);

  auto c = reader.readRGB();
  EXPECT_EQ(c.r, 255);
  EXPECT_EQ(c.g, 128);
  EXPECT_EQ(c.b, 64);
  EXPECT_EQ(reader.position(), 4); // Consumed padding byte
}

TEST_F(ChunkReaderTest, ReadRGBA) {
  auto data = makeData({0xFF, 0x80, 0x40, 0xC0});
  ChunkReader reader(data);

  auto c = reader.readRGBA();
  EXPECT_EQ(c.r, 255);
  EXPECT_EQ(c.g, 128);
  EXPECT_EQ(c.b, 64);
  EXPECT_EQ(c.a, 192);
}

// =============================================================================
// Chunk Header Tests
// =============================================================================

TEST_F(ChunkReaderTest, ReadChunkHeader) {
  auto data = makeData({
      0x00, 0x00, 0x00, 0x00, // ChunkType (MESH = 0)
      0x64, 0x00, 0x00, 0x00, // Size = 100
  });
  ChunkReader reader(data);

  auto header = reader.readChunkHeader();
  EXPECT_EQ(static_cast<uint32_t>(header.type), 0);
  EXPECT_EQ(header.size, 100);
  EXPECT_FALSE(header.isContainer());
  EXPECT_EQ(header.dataSize(), 100);
}

TEST_F(ChunkReaderTest, ReadChunkHeaderContainer) {
  auto data = makeData({
      0x01, 0x00, 0x00, 0x00, // ChunkType
      0x00, 0x01, 0x00, 0x80, // Size with container bit set (0x80000100)
  });
  ChunkReader reader(data);

  auto header = reader.readChunkHeader();
  EXPECT_TRUE(header.isContainer());
  EXPECT_EQ(header.dataSize(), 0x100);
}

TEST_F(ChunkReaderTest, PeekChunkHeaderDoesNotConsume) {
  auto data = makeData({
      0x00,
      0x00,
      0x00,
      0x00,
      0x10,
      0x00,
      0x00,
      0x00,
  });
  ChunkReader reader(data);

  auto header1 = reader.peekChunkHeader();
  EXPECT_TRUE(header1.has_value());
  EXPECT_EQ(reader.position(), 0); // Position unchanged

  auto header2 = reader.peekChunkHeader();
  EXPECT_EQ(header1->dataSize(), header2->dataSize());
}

TEST_F(ChunkReaderTest, PeekChunkHeaderNotEnoughData) {
  auto data = makeData({0x00, 0x00, 0x00}); // Only 3 bytes, need 8
  ChunkReader reader(data);

  auto header = reader.peekChunkHeader();
  EXPECT_FALSE(header.has_value());
}

// =============================================================================
// SubReader Tests
// =============================================================================

TEST_F(ChunkReaderTest, SubReaderBasic) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
  ChunkReader reader(data);

  reader.skip(1); // Skip first byte
  auto sub = reader.subReader(3);

  EXPECT_EQ(sub.size(), 3);
  EXPECT_EQ(sub.read<uint8_t>(), 0x02);
  EXPECT_EQ(sub.read<uint8_t>(), 0x03);
  EXPECT_EQ(sub.read<uint8_t>(), 0x04);
  EXPECT_TRUE(sub.atEnd());

  // Parent reader advanced past the sub-reader's data
  EXPECT_EQ(reader.position(), 4);
  EXPECT_EQ(reader.read<uint8_t>(), 0x05);
}

TEST_F(ChunkReaderTest, SubReaderIsolated) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  auto sub = reader.subReader(2);

  // Sub-reader can't read past its boundary
  EXPECT_THROW(sub.readArray<uint8_t>(3), ParseError);
}

TEST_F(ChunkReaderTest, SubReaderPastEndThrows) {
  auto data = makeData({0x01, 0x02});
  ChunkReader reader(data);

  EXPECT_THROW(reader.subReader(5), ParseError);
}

TEST_F(ChunkReaderTest, SubReaderNested) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});
  ChunkReader reader(data);

  auto sub1 = reader.subReader(6);
  EXPECT_EQ(sub1.size(), 6);

  sub1.skip(1);
  auto sub2 = sub1.subReader(3);
  EXPECT_EQ(sub2.size(), 3);
  EXPECT_EQ(sub2.read<uint8_t>(), 0x02);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(ChunkReaderTest, ReadExactlyToEnd) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  ChunkReader reader(data);

  reader.readArray<uint8_t>(4);
  EXPECT_TRUE(reader.atEnd());
  EXPECT_EQ(reader.remaining(), 0);
}

TEST_F(ChunkReaderTest, MultipleReadsSequential) {
  auto data = makeData({
      0x01, 0x00, 0x00, 0x00, // uint32 = 1
      0x02, 0x00,             // uint16 = 2
      0x03,                   // uint8 = 3
      0x04,                   // uint8 = 4
  });
  ChunkReader reader(data);

  EXPECT_EQ(reader.read<uint32_t>(), 1);
  EXPECT_EQ(reader.read<uint16_t>(), 2);
  EXPECT_EQ(reader.read<uint8_t>(), 3);
  EXPECT_EQ(reader.read<uint8_t>(), 4);
  EXPECT_TRUE(reader.atEnd());
}

TEST_F(ChunkReaderTest, ParseErrorContainsUsefulInfo) {
  auto data = makeData({0x01, 0x02});
  ChunkReader reader(data);

  try {
    reader.skip(10);
    FAIL() << "Expected ParseError";
  } catch (const ParseError &e) {
    std::string msg = e.what();
    EXPECT_NE(msg.find("pos=0"), std::string::npos);
    EXPECT_NE(msg.find("skip=10"), std::string::npos);
    EXPECT_NE(msg.find("size=2"), std::string::npos);
  }
}
