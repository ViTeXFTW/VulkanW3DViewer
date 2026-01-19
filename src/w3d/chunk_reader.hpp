#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "chunk_types.hpp"
#include "types.hpp"

namespace w3d {

// Chunk header as read from file
struct ChunkHeader {
  ChunkType type;
  uint32_t size;  // Size of data (not including this 8-byte header)

  // Check if this is a container chunk (has sub-chunks)
  bool isContainer() const {
    // Container chunks have the high bit set in the size field
    return (size & 0x80000000) != 0;
  }

  // Get the actual data size (mask off container bit)
  uint32_t dataSize() const { return size & 0x7FFFFFFF; }
};

// Exception for parsing errors
class ParseError : public std::runtime_error {
 public:
  explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

// Binary reader for W3D data
class ChunkReader {
 public:
  explicit ChunkReader(std::span<const uint8_t> data)
      : data_(data), pos_(0) {}

  // Current position in the data
  size_t position() const { return pos_; }

  // Total size of data
  size_t size() const { return data_.size(); }

  // Remaining bytes
  size_t remaining() const { return data_.size() - pos_; }

  // Check if we've reached the end
  bool atEnd() const { return pos_ >= data_.size(); }

  // Seek to a position
  void seek(size_t pos) {
    if (pos > data_.size()) {
      throw ParseError("Seek past end of data");
    }
    pos_ = pos;
  }

  // Skip bytes
  void skip(size_t count) {
    if (pos_ + count > data_.size()) {
      throw ParseError("Skip past end of data");
    }
    pos_ += count;
  }

  // Read raw bytes
  void readBytes(void* dest, size_t count) {
    if (pos_ + count > data_.size()) {
      throw ParseError("Read past end of data");
    }
    std::memcpy(dest, data_.data() + pos_, count);
    pos_ += count;
  }

  // Read a single value (little-endian)
  template <typename T>
  T read() {
    static_assert(std::is_trivially_copyable_v<T>);
    T value;
    readBytes(&value, sizeof(T));
    return value;
  }

  // Read multiple values into a vector
  template <typename T>
  std::vector<T> readArray(size_t count) {
    static_assert(std::is_trivially_copyable_v<T>);
    std::vector<T> result(count);
    if (count > 0) {
      readBytes(result.data(), count * sizeof(T));
    }
    return result;
  }

  // Read a fixed-length string (null-padded)
  std::string readFixedString(size_t length) {
    std::string result(length, '\0');
    readBytes(result.data(), length);
    // Trim at first null
    size_t nullPos = result.find('\0');
    if (nullPos != std::string::npos) {
      result.resize(nullPos);
    }
    return result;
  }

  // Read a null-terminated string (variable length, up to maxLen)
  std::string readNullString(size_t maxLen) {
    std::string result;
    result.reserve(maxLen);
    for (size_t i = 0; i < maxLen && !atEnd(); ++i) {
      char c = read<char>();
      if (c == '\0') break;
      result.push_back(c);
    }
    return result;
  }

  // Read a null-terminated string consuming all remaining bytes
  std::string readRemainingString() {
    std::string result;
    while (!atEnd()) {
      char c = read<char>();
      if (c == '\0') break;
      result.push_back(c);
    }
    return result;
  }

  // Read a chunk header
  ChunkHeader readChunkHeader() {
    ChunkHeader header;
    header.type = static_cast<ChunkType>(read<uint32_t>());
    header.size = read<uint32_t>();
    return header;
  }

  // Peek at chunk header without consuming
  std::optional<ChunkHeader> peekChunkHeader() {
    if (remaining() < 8) return std::nullopt;
    size_t savedPos = pos_;
    auto header = readChunkHeader();
    pos_ = savedPos;
    return header;
  }

  // Create a sub-reader for a chunk's data
  ChunkReader subReader(size_t length) {
    if (pos_ + length > data_.size()) {
      throw ParseError("Sub-reader extends past end of data");
    }
    ChunkReader sub(data_.subspan(pos_, length));
    pos_ += length;
    return sub;
  }

  // Get current data pointer
  const uint8_t* currentPtr() const { return data_.data() + pos_; }

  // Read Vector3
  Vector3 readVector3() {
    Vector3 v;
    v.x = read<float>();
    v.y = read<float>();
    v.z = read<float>();
    return v;
  }

  // Read Vector2
  Vector2 readVector2() {
    Vector2 v;
    v.u = read<float>();
    v.v = read<float>();
    return v;
  }

  // Read Quaternion
  Quaternion readQuaternion() {
    Quaternion q;
    q.x = read<float>();
    q.y = read<float>();
    q.z = read<float>();
    q.w = read<float>();
    return q;
  }

  // Read RGB
  RGB readRGB() {
    RGB c;
    c.r = read<uint8_t>();
    c.g = read<uint8_t>();
    c.b = read<uint8_t>();
    skip(1);  // padding byte
    return c;
  }

  // Read RGBA
  RGBA readRGBA() {
    RGBA c;
    c.r = read<uint8_t>();
    c.g = read<uint8_t>();
    c.b = read<uint8_t>();
    c.a = read<uint8_t>();
    return c;
  }

 private:
  std::span<const uint8_t> data_;
  size_t pos_;
};

}  // namespace w3d
