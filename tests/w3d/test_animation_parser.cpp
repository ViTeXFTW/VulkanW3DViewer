#include "lib/formats/w3d/animation_parser.hpp"
#include "lib/formats/w3d/chunk_reader.hpp"

#include <gtest/gtest.h>

using namespace w3d;

class AnimationParserTest : public ::testing::Test {
protected:
  static std::vector<uint8_t> makeChunk(ChunkType type, const std::vector<uint8_t> &data,
                                        bool isContainer = false) {
    std::vector<uint8_t> result;
    uint32_t typeVal = static_cast<uint32_t>(type);
    result.push_back(typeVal & 0xFF);
    result.push_back((typeVal >> 8) & 0xFF);
    result.push_back((typeVal >> 16) & 0xFF);
    result.push_back((typeVal >> 24) & 0xFF);

    uint32_t size = static_cast<uint32_t>(data.size());
    if (isContainer) {
      size |= 0x80000000;
    }
    result.push_back(size & 0xFF);
    result.push_back((size >> 8) & 0xFF);
    result.push_back((size >> 16) & 0xFF);
    result.push_back((size >> 24) & 0xFF);

    result.insert(result.end(), data.begin(), data.end());
    return result;
  }

  static void appendFloat(std::vector<uint8_t> &vec, float f) {
    uint8_t *bytes = reinterpret_cast<uint8_t *>(&f);
    vec.insert(vec.end(), bytes, bytes + sizeof(float));
  }

  static void appendUint32(std::vector<uint8_t> &vec, uint32_t val) {
    vec.push_back(val & 0xFF);
    vec.push_back((val >> 8) & 0xFF);
    vec.push_back((val >> 16) & 0xFF);
    vec.push_back((val >> 24) & 0xFF);
  }

  static void appendUint16(std::vector<uint8_t> &vec, uint16_t val) {
    vec.push_back(val & 0xFF);
    vec.push_back((val >> 8) & 0xFF);
  }

  static void appendFixedString(std::vector<uint8_t> &vec, const std::string &str,
                                size_t len = 16) {
    for (size_t i = 0; i < len; ++i) {
      vec.push_back(i < str.size() ? str[i] : '\0');
    }
  }

  // Create animation header chunk data
  static std::vector<uint8_t> makeAnimHeader(const std::string &name, const std::string &hierName,
                                             uint32_t numFrames, uint32_t frameRate) {
    std::vector<uint8_t> data;
    appendUint32(data, 1); // version
    appendFixedString(data, name, 16);
    appendFixedString(data, hierName, 16);
    appendUint32(data, numFrames);
    appendUint32(data, frameRate);
    return data;
  }

  // Create animation channel data
  static std::vector<uint8_t> makeAnimChannel(uint16_t firstFrame, uint16_t lastFrame,
                                              uint16_t vectorLen, uint16_t flags, uint16_t pivot,
                                              const std::vector<float> &values) {
    std::vector<uint8_t> data;
    appendUint16(data, firstFrame);
    appendUint16(data, lastFrame);
    appendUint16(data, vectorLen);
    appendUint16(data, flags);
    appendUint16(data, pivot);
    appendUint16(data, 0); // padding

    for (float v : values) {
      appendFloat(data, v);
    }
    return data;
  }

  // Create bit channel data
  static std::vector<uint8_t> makeBitChannel(uint16_t firstFrame, uint16_t lastFrame,
                                             uint16_t flags, uint16_t pivot, float defaultVal,
                                             const std::vector<uint8_t> &bits) {
    std::vector<uint8_t> data;
    appendUint16(data, firstFrame);
    appendUint16(data, lastFrame);
    appendUint16(data, flags);
    appendUint16(data, pivot);
    appendFloat(data, defaultVal);
    data.insert(data.end(), bits.begin(), bits.end());
    return data;
  }
};

// =============================================================================
// Standard Animation Tests
// =============================================================================

TEST_F(AnimationParserTest, EmptyAnimationHeaderParsing) {
  auto headerData = makeAnimHeader("TestAnim", "TestHierarchy", 30, 15);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::ANIMATION_HEADER, headerData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());

  ChunkReader reader(data);
  Animation anim = AnimationParser::parse(reader, static_cast<uint32_t>(data.size()));

  EXPECT_EQ(anim.version, 1);
  EXPECT_EQ(anim.name, "TestAnim");
  EXPECT_EQ(anim.hierarchyName, "TestHierarchy");
  EXPECT_EQ(anim.numFrames, 30);
  EXPECT_EQ(anim.frameRate, 15);
  EXPECT_TRUE(anim.channels.empty());
  EXPECT_TRUE(anim.bitChannels.empty());
}

TEST_F(AnimationParserTest, SingleXTranslationChannel) {
  auto headerData = makeAnimHeader("MoveX", "Skeleton", 10, 30);

  // X translation channel: frames 0-9, 10 values
  std::vector<float> xValues;
  for (int i = 0; i < 10; ++i) {
    xValues.push_back(static_cast<float>(i) * 0.5f);
  }
  auto channelData = makeAnimChannel(0, 9, 1, AnimChannelType::X, 0, xValues);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::ANIMATION_HEADER, headerData);
  auto channelChunk = makeChunk(ChunkType::ANIMATION_CHANNEL, channelData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), channelChunk.begin(), channelChunk.end());

  ChunkReader reader(data);
  Animation anim = AnimationParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(anim.channels.size(), 1);
  EXPECT_EQ(anim.channels[0].firstFrame, 0);
  EXPECT_EQ(anim.channels[0].lastFrame, 9);
  EXPECT_EQ(anim.channels[0].vectorLen, 1);
  EXPECT_EQ(anim.channels[0].flags, AnimChannelType::X);
  EXPECT_EQ(anim.channels[0].pivot, 0);
  ASSERT_EQ(anim.channels[0].data.size(), 10);
  EXPECT_FLOAT_EQ(anim.channels[0].data[5], 2.5f);
}

TEST_F(AnimationParserTest, QuaternionRotationChannel) {
  auto headerData = makeAnimHeader("Rotate", "Skeleton", 5, 15);

  // Quaternion channel: 5 frames, 4 components each = 20 values
  std::vector<float> quatValues;
  for (int i = 0; i < 5; ++i) {
    // Identity quaternion (x, y, z, w)
    quatValues.push_back(0.0f);
    quatValues.push_back(0.0f);
    quatValues.push_back(0.0f);
    quatValues.push_back(1.0f);
  }
  auto channelData = makeAnimChannel(0, 4, 4, AnimChannelType::Q, 1, quatValues);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::ANIMATION_HEADER, headerData);
  auto channelChunk = makeChunk(ChunkType::ANIMATION_CHANNEL, channelData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), channelChunk.begin(), channelChunk.end());

  ChunkReader reader(data);
  Animation anim = AnimationParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(anim.channels.size(), 1);
  EXPECT_EQ(anim.channels[0].vectorLen, 4);
  EXPECT_EQ(anim.channels[0].flags, AnimChannelType::Q);
  EXPECT_EQ(anim.channels[0].pivot, 1);
  ASSERT_EQ(anim.channels[0].data.size(), 20);
  // First quaternion w component (index 3)
  EXPECT_FLOAT_EQ(anim.channels[0].data[3], 1.0f);
}

TEST_F(AnimationParserTest, MultipleChannelsForDifferentPivots) {
  auto headerData = makeAnimHeader("MultiChannel", "Skeleton", 10, 30);

  // X translation for pivot 0
  std::vector<float> xValues(10, 1.0f);
  auto xChannel = makeAnimChannel(0, 9, 1, AnimChannelType::X, 0, xValues);

  // Y translation for pivot 1
  std::vector<float> yValues(10, 2.0f);
  auto yChannel = makeAnimChannel(0, 9, 1, AnimChannelType::Y, 1, yValues);

  // Quaternion for pivot 2
  std::vector<float> qValues(40, 0.0f);
  for (int i = 0; i < 10; ++i) {
    qValues[i * 4 + 3] = 1.0f; // w = 1
  }
  auto qChannel = makeAnimChannel(0, 9, 4, AnimChannelType::Q, 2, qValues);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::ANIMATION_HEADER, headerData);
  auto xChunk = makeChunk(ChunkType::ANIMATION_CHANNEL, xChannel);
  auto yChunk = makeChunk(ChunkType::ANIMATION_CHANNEL, yChannel);
  auto qChunk = makeChunk(ChunkType::ANIMATION_CHANNEL, qChannel);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), xChunk.begin(), xChunk.end());
  data.insert(data.end(), yChunk.begin(), yChunk.end());
  data.insert(data.end(), qChunk.begin(), qChunk.end());

  ChunkReader reader(data);
  Animation anim = AnimationParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(anim.channels.size(), 3);
  EXPECT_EQ(anim.channels[0].flags, AnimChannelType::X);
  EXPECT_EQ(anim.channels[0].pivot, 0);
  EXPECT_EQ(anim.channels[1].flags, AnimChannelType::Y);
  EXPECT_EQ(anim.channels[1].pivot, 1);
  EXPECT_EQ(anim.channels[2].flags, AnimChannelType::Q);
  EXPECT_EQ(anim.channels[2].pivot, 2);
}

TEST_F(AnimationParserTest, BitChannelParsing) {
  auto headerData = makeAnimHeader("Visibility", "Skeleton", 16, 30);

  // Visibility bit channel: 16 frames (2 bytes of data)
  std::vector<uint8_t> bitData = {0xAA, 0x55}; // Alternating visibility
  auto bitChannelData = makeBitChannel(0, 15, 0, 0, 1.0f, bitData);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::ANIMATION_HEADER, headerData);
  auto bitChunk = makeChunk(ChunkType::BIT_CHANNEL, bitChannelData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), bitChunk.begin(), bitChunk.end());

  ChunkReader reader(data);
  Animation anim = AnimationParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(anim.bitChannels.size(), 1);
  EXPECT_EQ(anim.bitChannels[0].firstFrame, 0);
  EXPECT_EQ(anim.bitChannels[0].lastFrame, 15);
  EXPECT_EQ(anim.bitChannels[0].pivot, 0);
  EXPECT_FLOAT_EQ(anim.bitChannels[0].defaultVal, 1.0f);
  ASSERT_EQ(anim.bitChannels[0].data.size(), 2);
  EXPECT_EQ(anim.bitChannels[0].data[0], 0xAA);
  EXPECT_EQ(anim.bitChannels[0].data[1], 0x55);
}

TEST_F(AnimationParserTest, PartialFrameRangeChannel) {
  auto headerData = makeAnimHeader("Partial", "Skeleton", 30, 30);

  // Channel only covers frames 10-19
  std::vector<float> values(10, 5.0f);
  auto channelData = makeAnimChannel(10, 19, 1, AnimChannelType::Z, 0, values);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::ANIMATION_HEADER, headerData);
  auto channelChunk = makeChunk(ChunkType::ANIMATION_CHANNEL, channelData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), channelChunk.begin(), channelChunk.end());

  ChunkReader reader(data);
  Animation anim = AnimationParser::parse(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(anim.channels.size(), 1);
  EXPECT_EQ(anim.channels[0].firstFrame, 10);
  EXPECT_EQ(anim.channels[0].lastFrame, 19);
  ASSERT_EQ(anim.channels[0].data.size(), 10);
}

// =============================================================================
// Compressed Animation Tests
// =============================================================================

TEST_F(AnimationParserTest, CompressedAnimationHeaderParsing) {
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1); // version
  appendFixedString(headerData, "CompAnim", 16);
  appendFixedString(headerData, "Skeleton", 16);
  appendUint32(headerData, 60); // numFrames
  appendUint16(headerData, 30); // frameRate (uint16 for compressed)
  appendUint16(headerData, 0);  // flavor

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::COMPRESSED_ANIMATION_HEADER, headerData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());

  ChunkReader reader(data);
  CompressedAnimation anim =
      AnimationParser::parseCompressed(reader, static_cast<uint32_t>(data.size()));

  EXPECT_EQ(anim.version, 1);
  EXPECT_EQ(anim.name, "CompAnim");
  EXPECT_EQ(anim.hierarchyName, "Skeleton");
  EXPECT_EQ(anim.numFrames, 60);
  EXPECT_EQ(anim.frameRate, 30);
  EXPECT_EQ(anim.flavor, 0);
}

TEST_F(AnimationParserTest, CompressedChannelParsing) {
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1);
  appendFixedString(headerData, "CompAnim", 16);
  appendFixedString(headerData, "Skeleton", 16);
  appendUint32(headerData, 100);
  appendUint16(headerData, 30);
  appendUint16(headerData, 0);

  // Compressed channel with 3 keyframes
  std::vector<uint8_t> channelData;
  appendUint32(channelData, 3);                            // numTimeCodes
  appendUint16(channelData, 0);                            // pivot
  appendUint16(channelData, 1);                            // vectorLen
  appendUint16(channelData, AnimChannelType::TIMECODED_X); // flags
  appendUint16(channelData, 0);                            // padding
  appendUint16(channelData, 0);                            // padding
  // Time codes (3 uint16s, padded to 4 bytes)
  appendUint16(channelData, 0);  // frame 0
  appendUint16(channelData, 50); // frame 50
  appendUint16(channelData, 99); // frame 99
  appendUint16(channelData, 0);  // padding for 4-byte alignment
  // Data values (3 floats)
  appendFloat(channelData, 0.0f);
  appendFloat(channelData, 5.0f);
  appendFloat(channelData, 10.0f);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::COMPRESSED_ANIMATION_HEADER, headerData);
  auto channelChunk = makeChunk(ChunkType::COMPRESSED_ANIMATION_CHANNEL, channelData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), channelChunk.begin(), channelChunk.end());

  ChunkReader reader(data);
  CompressedAnimation anim =
      AnimationParser::parseCompressed(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(anim.channels.size(), 1);
  EXPECT_EQ(anim.channels[0].numTimeCodes, 3);
  EXPECT_EQ(anim.channels[0].pivot, 0);
  EXPECT_EQ(anim.channels[0].vectorLen, 1);
  EXPECT_EQ(anim.channels[0].flags, AnimChannelType::TIMECODED_X);

  ASSERT_EQ(anim.channels[0].timeCodes.size(), 3);
  EXPECT_EQ(anim.channels[0].timeCodes[0], 0);
  EXPECT_EQ(anim.channels[0].timeCodes[1], 50);
  EXPECT_EQ(anim.channels[0].timeCodes[2], 99);

  ASSERT_EQ(anim.channels[0].data.size(), 3);
  EXPECT_FLOAT_EQ(anim.channels[0].data[0], 0.0f);
  EXPECT_FLOAT_EQ(anim.channels[0].data[1], 5.0f);
  EXPECT_FLOAT_EQ(anim.channels[0].data[2], 10.0f);
}

TEST_F(AnimationParserTest, CompressedQuaternionChannel) {
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1);
  appendFixedString(headerData, "RotAnim", 16);
  appendFixedString(headerData, "Skeleton", 16);
  appendUint32(headerData, 50);
  appendUint16(headerData, 30);
  appendUint16(headerData, 0);

  // Compressed quaternion channel with 2 keyframes
  std::vector<uint8_t> channelData;
  appendUint32(channelData, 2); // numTimeCodes
  appendUint16(channelData, 1); // pivot
  appendUint16(channelData, 4); // vectorLen (quaternion)
  appendUint16(channelData, AnimChannelType::TIMECODED_Q);
  appendUint16(channelData, 0);
  appendUint16(channelData, 0);
  // Time codes (2 uint16s, even count = no padding)
  appendUint16(channelData, 0);
  appendUint16(channelData, 49);
  // Data: 2 quaternions (8 floats)
  // First: identity (0, 0, 0, 1)
  appendFloat(channelData, 0.0f);
  appendFloat(channelData, 0.0f);
  appendFloat(channelData, 0.0f);
  appendFloat(channelData, 1.0f);
  // Second: 90 deg around Y
  appendFloat(channelData, 0.0f);
  appendFloat(channelData, 0.707f);
  appendFloat(channelData, 0.0f);
  appendFloat(channelData, 0.707f);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::COMPRESSED_ANIMATION_HEADER, headerData);
  auto channelChunk = makeChunk(ChunkType::COMPRESSED_ANIMATION_CHANNEL, channelData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), channelChunk.begin(), channelChunk.end());

  ChunkReader reader(data);
  CompressedAnimation anim =
      AnimationParser::parseCompressed(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(anim.channels.size(), 1);
  EXPECT_EQ(anim.channels[0].vectorLen, 4);
  EXPECT_EQ(anim.channels[0].flags, AnimChannelType::TIMECODED_Q);
  ASSERT_EQ(anim.channels[0].data.size(), 8);
  // First quaternion w
  EXPECT_FLOAT_EQ(anim.channels[0].data[3], 1.0f);
  // Second quaternion y
  EXPECT_NEAR(anim.channels[0].data[5], 0.707f, 0.001f);
}

TEST_F(AnimationParserTest, CompressedBitChannel) {
  std::vector<uint8_t> headerData;
  appendUint32(headerData, 1);
  appendFixedString(headerData, "VisAnim", 16);
  appendFixedString(headerData, "Skeleton", 16);
  appendUint32(headerData, 8);
  appendUint16(headerData, 30);
  appendUint16(headerData, 0);

  // Bit channel for visibility
  std::vector<uint8_t> bitData = {0xFF}; // All visible
  auto bitChannelData = makeBitChannel(0, 7, 0, 2, 0.0f, bitData);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::COMPRESSED_ANIMATION_HEADER, headerData);
  auto bitChunk = makeChunk(ChunkType::COMPRESSED_BIT_CHANNEL, bitChannelData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), bitChunk.begin(), bitChunk.end());

  ChunkReader reader(data);
  CompressedAnimation anim =
      AnimationParser::parseCompressed(reader, static_cast<uint32_t>(data.size()));

  ASSERT_EQ(anim.bitChannels.size(), 1);
  EXPECT_EQ(anim.bitChannels[0].pivot, 2);
  EXPECT_FLOAT_EQ(anim.bitChannels[0].defaultVal, 0.0f);
  EXPECT_EQ(anim.bitChannels[0].data[0], 0xFF);
}

TEST_F(AnimationParserTest, UnknownChunksInAnimationSkipped) {
  auto headerData = makeAnimHeader("Test", "Skeleton", 10, 30);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::ANIMATION_HEADER, headerData);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());

  // Unknown chunk
  std::vector<uint8_t> unknownChunk = {
      0xEF, 0xBE, 0xAD, 0xDE, 0x04, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
  };
  data.insert(data.end(), unknownChunk.begin(), unknownChunk.end());

  // Valid channel after unknown
  std::vector<float> values(10, 0.0f);
  auto channelData = makeAnimChannel(0, 9, 1, AnimChannelType::X, 0, values);
  auto channelChunk = makeChunk(ChunkType::ANIMATION_CHANNEL, channelData);
  data.insert(data.end(), channelChunk.begin(), channelChunk.end());

  ChunkReader reader(data);
  Animation anim = AnimationParser::parse(reader, static_cast<uint32_t>(data.size()));

  EXPECT_EQ(anim.name, "Test");
  ASSERT_EQ(anim.channels.size(), 1);
}

TEST_F(AnimationParserTest, MixedChannelsAndBitChannels) {
  auto headerData = makeAnimHeader("Mixed", "Skeleton", 20, 30);

  std::vector<float> xValues(20, 1.0f);
  auto xChannel = makeAnimChannel(0, 19, 1, AnimChannelType::X, 0, xValues);

  std::vector<uint8_t> visBits = {0xFF, 0xFF, 0x0F}; // 20 frames
  auto bitChannelData = makeBitChannel(0, 19, 0, 0, 1.0f, visBits);

  std::vector<float> qValues(80, 0.0f);
  for (int i = 0; i < 20; ++i) {
    qValues[i * 4 + 3] = 1.0f;
  }
  auto qChannel = makeAnimChannel(0, 19, 4, AnimChannelType::Q, 1, qValues);

  std::vector<uint8_t> data;
  auto headerChunk = makeChunk(ChunkType::ANIMATION_HEADER, headerData);
  auto xChunk = makeChunk(ChunkType::ANIMATION_CHANNEL, xChannel);
  auto bitChunk = makeChunk(ChunkType::BIT_CHANNEL, bitChannelData);
  auto qChunk = makeChunk(ChunkType::ANIMATION_CHANNEL, qChannel);
  data.insert(data.end(), headerChunk.begin(), headerChunk.end());
  data.insert(data.end(), xChunk.begin(), xChunk.end());
  data.insert(data.end(), bitChunk.begin(), bitChunk.end());
  data.insert(data.end(), qChunk.begin(), qChunk.end());

  ChunkReader reader(data);
  Animation anim = AnimationParser::parse(reader, static_cast<uint32_t>(data.size()));

  EXPECT_EQ(anim.channels.size(), 2);
  EXPECT_EQ(anim.bitChannels.size(), 1);
}
