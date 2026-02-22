#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace map {

constexpr uint32_t DATA_CHUNK_MAGIC = 0x704D6B43;
constexpr uint32_t CHUNK_HEADER_SIZE = 10;

enum class DataType : uint8_t { Bool = 0, Int = 1, Real = 2, AsciiString = 3, UnicodeString = 4 };

struct DictValue {
  DataType type;
  union {
    bool boolValue;
    int32_t intValue;
    float realValue;
  };
  std::string stringValue;

  static DictValue makeBool(bool value);
  static DictValue makeInt(int32_t value);
  static DictValue makeReal(float value);
  static DictValue makeString(std::string value);
};

using Dict = std::unordered_map<std::string, DictValue>;

struct ChunkHeader {
  uint32_t id;
  uint16_t version;
  int32_t dataSize;
};

class DataChunkReader {
public:
  DataChunkReader() = default;
  explicit DataChunkReader(std::span<const uint8_t> data);

  std::optional<std::string> loadFromMemory(std::span<const uint8_t> data);

  bool atEnd() const;
  std::optional<ChunkHeader> openChunk(std::string *outError = nullptr);
  void closeChunk();

  std::optional<std::string> lookupName(uint32_t id) const;
  uint32_t remainingInChunk() const;

  std::optional<int8_t> readByte(std::string *outError = nullptr);
  std::optional<int32_t> readInt(std::string *outError = nullptr);
  std::optional<float> readReal(std::string *outError = nullptr);
  std::optional<std::string> readAsciiString(std::string *outError = nullptr);
  std::optional<std::string> readUnicodeString(std::string *outError = nullptr);
  std::optional<Dict> readDict(std::string *outError = nullptr);
  bool readBytes(uint8_t *dest, size_t count, std::string *outError = nullptr);

private:
  std::optional<std::string> parseTOC();
  void decrementDataLeft(uint32_t count);

  std::span<const uint8_t> data_;
  size_t pos_ = 0;
  std::unordered_map<uint32_t, std::string> nameTable_;
  std::vector<uint32_t> chunkStack_;
  std::vector<uint32_t> dataLeftStack_;
};

} // namespace map
