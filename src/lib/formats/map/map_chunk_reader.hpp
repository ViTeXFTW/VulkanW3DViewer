#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "chunk_types.hpp"

namespace map {

// Exception for parsing errors
class ParseError : public std::runtime_error {
public:
  explicit ParseError(const std::string &msg) : std::runtime_error(msg) {}
};

// Binary reader for map data
// Map format uses text-based chunk names (4 bytes) + version (4 bytes) + size (4
// bytes)
class MapChunkReader {
public:
  explicit MapChunkReader(std::span<const uint8_t> data) : data_(data), pos_(0) {}

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
      throw ParseError("Seek past end of data (pos=" + std::to_string(pos) +
                       ", size=" + std::to_string(data_.size()) + ")");
    }
    pos_ = pos;
  }

  // Skip bytes
  void skip(size_t count) {
    if (pos_ + count > data_.size()) {
      throw ParseError("Skip past end of data (pos=" + std::to_string(pos_) +
                       ", skip=" + std::to_string(count) +
                       ", size=" + std::to_string(data_.size()) + ")");
    }
    pos_ += count;
  }

  // Read raw bytes
  void readBytes(void *dest, size_t count) {
    if (pos_ + count > data_.size()) {
      throw ParseError("Read past end of data (pos=" + std::to_string(pos_) +
                       ", read=" + std::to_string(count) +
                       ", size=" + std::to_string(data_.size()) + ")");
    }
    std::memcpy(dest, data_.data() + pos_, count);
    pos_ += count;
  }

  // Read a single value (little-endian)
  template <typename T> T read() {
    static_assert(std::is_trivially_copyable_v<T>);
    T value;
    readBytes(&value, sizeof(T));
    return value;
  }

  // Read multiple values into a vector
  template <typename T> std::vector<T> readArray(size_t count) {
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

  // Read a 4-character chunk name
  std::string readChunkName() {
    char name[5] = {0};
    readBytes(name, 4);
    return std::string(name);
  }

  // Read a null-terminated string (variable length, up to maxLen)
  std::string readNullString(size_t maxLen) {
    std::string result;
    result.reserve(maxLen);
    for (size_t i = 0; i < maxLen && !atEnd(); ++i) {
      char c = read<char>();
      if (c == '\0')
        break;
      result.push_back(c);
    }
    return result;
  }

  // Read a null-terminated string consuming all remaining bytes
  std::string readRemainingString() {
    std::string result;
    while (!atEnd()) {
      char c = read<char>();
      if (c == '\0')
        break;
      result.push_back(c);
    }
    return result;
  }

  // Read a map chunk header
  // Format: 4-byte name + 4-byte version + 4-byte size
  MapChunkHeader readChunkHeader() {
    MapChunkHeader header;
    header.name = readChunkName();
    header.version = read<uint32_t>();
    header.size = read<uint32_t>();
    return header;
  }

  // Peek at chunk header without consuming
  std::optional<MapChunkHeader> peekChunkHeader() {
    if (remaining() < 12)
      return std::nullopt;
    size_t savedPos = pos_;
    auto header = readChunkHeader();
    pos_ = savedPos;
    return header;
  }

  // Create a sub-reader for a chunk's data
  MapChunkReader subReader(size_t length) {
    if (pos_ + length > data_.size()) {
      throw ParseError("Sub-reader extends past end of data (pos=" +
                       std::to_string(pos_) + ", length=" +
                       std::to_string(length) + ", size=" +
                       std::to_string(data_.size()) + ")");
    }
    MapChunkReader sub(data_.subspan(pos_, length));
    pos_ += length;
    return sub;
  }

  // Read an array of bytes
  std::vector<uint8_t> readByteArray(size_t count) {
    std::vector<uint8_t> result(count);
    if (count > 0) {
      readBytes(result.data(), count);
    }
    return result;
  }

  // Read real (float) - helper for consistency with legacy code
  float readReal() { return read<float>(); }

  // Read int - helper for consistency with legacy code
  int32_t readInt() { return read<int32_t>(); }

  // Read byte
  uint8_t readByte() { return read<uint8_t>(); }

private:
  std::span<const uint8_t> data_;
  size_t pos_;
};

} // namespace map
