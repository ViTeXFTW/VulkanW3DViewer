#include "lib/formats/map/chunk_types.hpp"
#include "lib/formats/map/map_chunk_reader.hpp"
#include "lib/formats/map/terrain_types.hpp"

#include <gtest/gtest.h>

using namespace map;

class MapChunkReaderTest : public ::testing::Test {
protected:
  // Helper to create test data
  static std::vector<uint8_t> makeData(std::initializer_list<uint8_t> bytes) {
    return std::vector<uint8_t>(bytes);
  }

  // Helper to create a chunk header
  static std::vector<uint8_t> makeChunkHeader(const std::string &name, uint32_t version,
                                              uint32_t size) {
    std::vector<uint8_t> result;
    result.reserve(12);

    // Chunk name (4 bytes, padded with nulls)
    result.insert(result.end(), name.begin(), name.end());
    while (result.size() < 4) {
      result.push_back('\0');
    }

    // Version (little-endian)
    result.push_back(static_cast<uint8_t>(version & 0xFF));
    result.push_back(static_cast<uint8_t>((version >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>((version >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((version >> 24) & 0xFF));

    // Size (little-endian)
    result.push_back(static_cast<uint8_t>(size & 0xFF));
    result.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>((size >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((size >> 24) & 0xFF));

    return result;
  }
};

// =============================================================================
// Basic Position/Size Tests
// =============================================================================

TEST_F(MapChunkReaderTest, EmptyData) {
  std::vector<uint8_t> data;
  MapChunkReader reader(data);

  EXPECT_EQ(reader.position(), 0);
  EXPECT_EQ(reader.size(), 0);
  EXPECT_EQ(reader.remaining(), 0);
  EXPECT_TRUE(reader.atEnd());
}

TEST_F(MapChunkReaderTest, InitialPosition) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  MapChunkReader reader(data);

  EXPECT_EQ(reader.position(), 0);
  EXPECT_EQ(reader.size(), 4);
  EXPECT_EQ(reader.remaining(), 4);
  EXPECT_FALSE(reader.atEnd());
}

// =============================================================================
// Seek Tests
// =============================================================================

TEST_F(MapChunkReaderTest, SeekToValidPosition) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  MapChunkReader reader(data);

  reader.seek(2);
  EXPECT_EQ(reader.position(), 2);
  EXPECT_EQ(reader.remaining(), 2);
}

TEST_F(MapChunkReaderTest, SeekPastEndThrows) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  MapChunkReader reader(data);

  EXPECT_THROW(reader.seek(5), ParseError);
}

// =============================================================================
// Skip Tests
// =============================================================================

TEST_F(MapChunkReaderTest, SkipValidAmount) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  MapChunkReader reader(data);

  reader.skip(2);
  EXPECT_EQ(reader.position(), 2);
}

TEST_F(MapChunkReaderTest, SkipPastEndThrows) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  MapChunkReader reader(data);

  EXPECT_THROW(reader.skip(5), ParseError);
}

// =============================================================================
// Read Primitive Tests
// =============================================================================

TEST_F(MapChunkReaderTest, ReadUint16LittleEndian) {
  auto data = makeData({0x34, 0x12}); // Little-endian 0x1234
  MapChunkReader reader(data);

  EXPECT_EQ(reader.read<uint16_t>(), 0x1234);
}

TEST_F(MapChunkReaderTest, ReadInt32) {
  auto data = makeData({0x78, 0x56, 0x34, 0x12}); // Little-endian 0x12345678
  MapChunkReader reader(data);

  EXPECT_EQ(reader.read<int32_t>(), 0x12345678);
}

TEST_F(MapChunkReaderTest, ReadFloat) {
  // IEEE 754 representation of 1.0f
  auto data = makeData({0x00, 0x00, 0x80, 0x3F});
  MapChunkReader reader(data);

  EXPECT_FLOAT_EQ(reader.read<float>(), 1.0f);
}

TEST_F(MapChunkReaderTest, ReadReal) {
  // IEEE 754 representation of 1.0f
  auto data = makeData({0x00, 0x00, 0x80, 0x3F});
  MapChunkReader reader(data);

  EXPECT_FLOAT_EQ(reader.readReal(), 1.0f);
}

TEST_F(MapChunkReaderTest, ReadByte) {
  auto data = makeData({0xAB, 0xCD});
  MapChunkReader reader(data);

  EXPECT_EQ(reader.readByte(), 0xAB);
  EXPECT_EQ(reader.readByte(), 0xCD);
}

// =============================================================================
// Read Array Tests
// =============================================================================

TEST_F(MapChunkReaderTest, ReadArrayInt16) {
  auto data = makeData({
      0x01, 0x00, // 1
      0x02, 0x00, // 2
      0x03, 0x00, // 3
  });
  MapChunkReader reader(data);

  auto result = reader.readArray<int16_t>(3);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
}

TEST_F(MapChunkReaderTest, ReadByteArray) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04});
  MapChunkReader reader(data);

  auto result = reader.readByteArray(4);
  EXPECT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], 0x01);
  EXPECT_EQ(result[3], 0x04);
}

// =============================================================================
// String Tests
// =============================================================================

TEST_F(MapChunkReaderTest, ReadFixedStringFull) {
  auto data = makeData({'H', 'e', 'l', 'l', 'o'});
  MapChunkReader reader(data);

  auto str = reader.readFixedString(5);
  EXPECT_EQ(str, "Hello");
}

TEST_F(MapChunkReaderTest, ReadFixedStringWithNullPadding) {
  auto data = makeData({'H', 'i', '\0', '\0', '\0'});
  MapChunkReader reader(data);

  auto str = reader.readFixedString(5);
  EXPECT_EQ(str, "Hi");
}

TEST_F(MapChunkReaderTest, ReadNullStringNormal) {
  auto data = makeData({'T', 'e', 's', 't', '\0', 'X'});
  MapChunkReader reader(data);

  auto str = reader.readNullString(10);
  EXPECT_EQ(str, "Test");
  EXPECT_EQ(reader.position(), 5); // Stopped at null
}

TEST_F(MapChunkReaderTest, ReadNullStringHitsMaxLen) {
  auto data = makeData({'A', 'B', 'C', 'D', 'E'});
  MapChunkReader reader(data);

  auto str = reader.readNullString(3);
  EXPECT_EQ(str, "ABC");
}

// =============================================================================
// Chunk Header Tests
// =============================================================================

TEST_F(MapChunkReaderTest, ReadChunkHeaderHeightMap) {
  auto headerData = makeChunkHeader("Heig", 4, 100);
  MapChunkReader reader(headerData);

  auto header = reader.readChunkHeader();
  EXPECT_EQ(header.name, "Heig");
  EXPECT_EQ(header.version, 4);
  EXPECT_EQ(header.size, 100);
  EXPECT_TRUE(header.isContainer());
}

TEST_F(MapChunkReaderTest, ReadChunkHeaderBlendTile) {
  auto headerData = makeChunkHeader("Blen", 7, 256);
  MapChunkReader reader(headerData);

  auto header = reader.readChunkHeader();
  EXPECT_EQ(header.name, "Blen");
  EXPECT_EQ(header.version, 7);
  EXPECT_EQ(header.size, 256);
}

TEST_F(MapChunkReaderTest, ReadChunkName) {
  auto data = makeData({'H', 'e', 'i', 'g'});
  MapChunkReader reader(data);

  auto name = reader.readChunkName();
  EXPECT_EQ(name, "Heig");
}

TEST_F(MapChunkReaderTest, PeekChunkHeaderDoesNotConsume) {
  auto headerData = makeChunkHeader("Heig", 4, 100);
  MapChunkReader reader(headerData);

  auto header1 = reader.peekChunkHeader();
  EXPECT_TRUE(header1.has_value());
  EXPECT_EQ(reader.position(), 0); // Position unchanged

  auto header2 = reader.peekChunkHeader();
  EXPECT_EQ(header1->size, header2->size);
}

TEST_F(MapChunkReaderTest, PeekChunkHeaderNotEnoughData) {
  auto data = makeData({0x00, 0x00, 0x00}); // Only 3 bytes, need 12
  MapChunkReader reader(data);

  auto header = reader.peekChunkHeader();
  EXPECT_FALSE(header.has_value());
}

// =============================================================================
// SubReader Tests
// =============================================================================

TEST_F(MapChunkReaderTest, SubReaderBasic) {
  auto data = makeData({0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
  MapChunkReader reader(data);

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

TEST_F(MapChunkReaderTest, SubReaderPastEndThrows) {
  auto data = makeData({0x01, 0x02});
  MapChunkReader reader(data);

  EXPECT_THROW(reader.subReader(5), ParseError);
}

// =============================================================================
// Terrain Data Structure Tests
// =============================================================================

TEST_F(MapChunkReaderTest, HeightmapDataDefaults) {
  HeightmapData heightmap;

  EXPECT_EQ(heightmap.width, 0);
  EXPECT_EQ(heightmap.height, 0);
  EXPECT_EQ(heightmap.borderSize, 0);
  EXPECT_TRUE(heightmap.boundaries.empty());
  EXPECT_TRUE(heightmap.heights.empty());
  EXPECT_FALSE(heightmap.isValid());
}

TEST_F(MapChunkReaderTest, HeightmapDataIsValid) {
  HeightmapData heightmap;
  heightmap.width = 10;
  heightmap.height = 10;
  heightmap.heights.resize(100);

  EXPECT_TRUE(heightmap.isValid());
  EXPECT_EQ(heightmap.dataSize(), 100);
}

TEST_F(MapChunkReaderTest, TileIndexDefaults) {
  TileIndex tile;

  EXPECT_EQ(tile.baseTile, 0);
  EXPECT_EQ(tile.blendTile, 0);
  EXPECT_EQ(tile.extraBlendTile, 0);
  EXPECT_EQ(tile.cliffInfo, 0);
  EXPECT_FALSE(tile.hasBlend());
  EXPECT_FALSE(tile.hasExtraBlend());
  EXPECT_FALSE(tile.hasCliffInfo());
}

TEST_F(MapChunkReaderTest, TileIndexHasBlend) {
  TileIndex tile;
  tile.blendTile = 100;

  EXPECT_TRUE(tile.hasBlend());
}

TEST_F(MapChunkReaderTest, TileIndexHasExtraBlend) {
  TileIndex tile;
  tile.extraBlendTile = 50;

  EXPECT_TRUE(tile.hasExtraBlend());
}

TEST_F(MapChunkReaderTest, TileIndexHasCliffInfo) {
  TileIndex tile;
  tile.cliffInfo = 10;

  EXPECT_TRUE(tile.hasCliffInfo());
}

TEST_F(MapChunkReaderTest, TerrainDataDefaults) {
  TerrainData terrain;

  EXPECT_FALSE(terrain.heightmap.isValid());
  EXPECT_TRUE(terrain.tiles.empty());
  EXPECT_TRUE(terrain.textureClasses.empty());
  EXPECT_TRUE(terrain.edgeTextureClasses.empty());
  EXPECT_TRUE(terrain.blendTiles.empty());
  EXPECT_TRUE(terrain.cliffInfoList.empty());
  EXPECT_FALSE(terrain.isValid());
}

// =============================================================================
// Constants Tests
// =============================================================================

TEST_F(MapChunkReaderTest, ConstantsAreDefined) {
  EXPECT_FLOAT_EQ(MAP_XY_FACTOR, 10.0f);
  EXPECT_FLOAT_EQ(MAP_HEIGHT_SCALE, MAP_XY_FACTOR / 16.0f);

  EXPECT_EQ(NUM_SOURCE_TILES, 1024);
  EXPECT_EQ(NUM_BLEND_TILES, 16192);
  EXPECT_EQ(NUM_CLIFF_INFO, 32384);
  EXPECT_EQ(NUM_TEXTURE_CLASSES, 256);
}

TEST_F(MapChunkReaderTest, ChunkVersionsAreDefined) {
  using namespace MapChunkVersion;

  EXPECT_EQ(HEIGHT_MAP_VERSION_1, 1);
  EXPECT_EQ(HEIGHT_MAP_VERSION_3, 3);
  EXPECT_EQ(HEIGHT_MAP_VERSION_4, 4);

  EXPECT_EQ(BLEND_TILE_VERSION_1, 1);
  EXPECT_EQ(BLEND_TILE_VERSION_4, 4);
  EXPECT_EQ(BLEND_TILE_VERSION_5, 5);
  EXPECT_EQ(BLEND_TILE_VERSION_6, 6);
  EXPECT_EQ(BLEND_TILE_VERSION_7, 7);
}

// =============================================================================
// Parse Error Tests
// =============================================================================

TEST_F(MapChunkReaderTest, ParseErrorContainsUsefulInfo) {
  auto data = makeData({0x01, 0x02});
  MapChunkReader reader(data);

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

// =============================================================================
// Integration Test: Read Simple HeightMap Chunk
// =============================================================================

TEST_F(MapChunkReaderTest, ReadSimpleHeightMapChunk) {
  // Create a minimal HeightMapData chunk (version 3)
  // Width: 4, Height: 4, BorderSize: 1
  std::vector<uint8_t> data;
  auto header = makeChunkHeader("Heig", 3, 2 + 2 + 2 + 4 + 16); // header + data
  data.insert(data.end(), header.begin(), header.end());

  // Width
  data.push_back(4);
  data.push_back(0);
  // Height
  data.push_back(4);
  data.push_back(0);
  // BorderSize
  data.push_back(1);
  data.push_back(0);
  // DataSize (4 * 4 = 16)
  data.push_back(16);
  data.push_back(0);
  data.push_back(0);
  data.push_back(0);
  // Height data (16 bytes)
  for (int i = 0; i < 16; ++i) {
    data.push_back(static_cast<uint8_t>(i));
  }

  MapChunkReader reader(data);

  auto chunkHeader = reader.readChunkHeader();
  EXPECT_EQ(chunkHeader.name, "Heig");
  EXPECT_EQ(chunkHeader.version, 3);

  // Read the heightmap data
  uint16_t width = reader.read<uint16_t>();
  uint16_t height = reader.read<uint16_t>();
  uint16_t borderSize = reader.read<uint16_t>();
  uint32_t dataSize = reader.read<uint32_t>();

  EXPECT_EQ(width, 4);
  EXPECT_EQ(height, 4);
  EXPECT_EQ(borderSize, 1);
  EXPECT_EQ(dataSize, 16);

  auto heights = reader.readByteArray(dataSize);
  EXPECT_EQ(heights.size(), 16);
  for (int i = 0; i < 16; ++i) {
    EXPECT_EQ(heights[i], i);
  }

  EXPECT_TRUE(reader.atEnd());
}
