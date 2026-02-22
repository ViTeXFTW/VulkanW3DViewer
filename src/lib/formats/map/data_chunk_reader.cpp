#include "data_chunk_reader.hpp"

#include <cstring>

namespace map {

DictValue DictValue::makeBool(bool value) {
  DictValue v;
  v.type = DataType::Bool;
  v.boolValue = value;
  return v;
}

DictValue DictValue::makeInt(int32_t value) {
  DictValue v;
  v.type = DataType::Int;
  v.intValue = value;
  return v;
}

DictValue DictValue::makeReal(float value) {
  DictValue v;
  v.type = DataType::Real;
  v.realValue = value;
  return v;
}

DictValue DictValue::makeString(std::string value) {
  DictValue v;
  v.type = DataType::AsciiString;
  v.stringValue = std::move(value);
  return v;
}

DataChunkReader::DataChunkReader(std::span<const uint8_t> data) : data_(data) {}

std::optional<std::string> DataChunkReader::loadFromMemory(std::span<const uint8_t> data) {
  data_ = data;
  pos_ = 0;
  nameTable_.clear();
  chunkStack_.clear();
  dataLeftStack_.clear();

  return parseTOC();
}

std::optional<std::string> DataChunkReader::parseTOC() {
  if (data_.size() < 8) {
    return "File too small for TOC header";
  }

  uint32_t magic;
  std::memcpy(&magic, &data_[pos_], 4);
  pos_ += 4;

  if (magic != DATA_CHUNK_MAGIC) {
    return "Invalid magic number (expected 'CkMp')";
  }

  int32_t count;
  std::memcpy(&count, &data_[pos_], 4);
  pos_ += 4;

  if (count < 0) {
    return "Negative TOC entry count";
  }

  for (int32_t i = 0; i < count; ++i) {
    if (pos_ >= data_.size()) {
      return "Unexpected end of file in TOC";
    }

    uint8_t nameLen = data_[pos_++];

    if (pos_ + nameLen + 4 > data_.size()) {
      return "Unexpected end of file reading TOC entry";
    }

    std::string name(reinterpret_cast<const char *>(&data_[pos_]), nameLen);
    pos_ += nameLen;

    uint32_t id;
    std::memcpy(&id, &data_[pos_], 4);
    pos_ += 4;

    nameTable_[id] = std::move(name);
  }

  return std::nullopt;
}

bool DataChunkReader::atEnd() const {
  if (!chunkStack_.empty()) {
    return dataLeftStack_.back() == 0;
  }
  return pos_ >= data_.size();
}

std::optional<ChunkHeader> DataChunkReader::openChunk(std::string *outError) {
  if (pos_ + CHUNK_HEADER_SIZE > data_.size()) {
    if (outError) {
      *outError = "Not enough data for chunk header";
    }
    return std::nullopt;
  }

  ChunkHeader header;
  std::memcpy(&header.id, &data_[pos_], 4);
  pos_ += 4;

  std::memcpy(&header.version, &data_[pos_], 2);
  pos_ += 2;

  std::memcpy(&header.dataSize, &data_[pos_], 4);
  pos_ += 4;

  if (header.dataSize < 0) {
    if (outError) {
      *outError = "Negative chunk data size";
    }
    return std::nullopt;
  }

  if (pos_ + static_cast<size_t>(header.dataSize) > data_.size()) {
    if (outError) {
      *outError = "Chunk data extends beyond file";
    }
    return std::nullopt;
  }

  chunkStack_.push_back(header.id);
  dataLeftStack_.push_back(static_cast<uint32_t>(header.dataSize));

  if (dataLeftStack_.size() > 1) {
    for (size_t i = 0; i < dataLeftStack_.size() - 1; ++i) {
      if (dataLeftStack_[i] >= CHUNK_HEADER_SIZE) {
        dataLeftStack_[i] -= CHUNK_HEADER_SIZE;
      } else {
        dataLeftStack_[i] = 0;
      }
    }
  }

  return header;
}

void DataChunkReader::closeChunk() {
  if (chunkStack_.empty()) {
    return;
  }

  uint32_t remaining = dataLeftStack_.back();
  pos_ += remaining;

  chunkStack_.pop_back();
  dataLeftStack_.pop_back();
}

std::optional<std::string> DataChunkReader::lookupName(uint32_t id) const {
  auto it = nameTable_.find(id);
  if (it == nameTable_.end()) {
    return std::nullopt;
  }
  return it->second;
}

uint32_t DataChunkReader::remainingInChunk() const {
  if (dataLeftStack_.empty()) {
    return static_cast<uint32_t>(data_.size() - pos_);
  }
  return dataLeftStack_.back();
}

void DataChunkReader::decrementDataLeft(uint32_t count) {
  for (auto &dataLeft : dataLeftStack_) {
    if (dataLeft >= count) {
      dataLeft -= count;
    } else {
      dataLeft = 0;
    }
  }
}

std::optional<int8_t> DataChunkReader::readByte(std::string *outError) {
  if (pos_ >= data_.size()) {
    if (outError) {
      *outError = "End of file reading byte";
    }
    return std::nullopt;
  }

  int8_t value = static_cast<int8_t>(data_[pos_++]);
  decrementDataLeft(1);
  return value;
}

std::optional<int32_t> DataChunkReader::readInt(std::string *outError) {
  if (pos_ + 4 > data_.size()) {
    if (outError) {
      *outError = "Not enough data for int32";
    }
    return std::nullopt;
  }

  int32_t value;
  std::memcpy(&value, &data_[pos_], 4);
  pos_ += 4;
  decrementDataLeft(4);
  return value;
}

std::optional<float> DataChunkReader::readReal(std::string *outError) {
  if (pos_ + 4 > data_.size()) {
    if (outError) {
      *outError = "Not enough data for float";
    }
    return std::nullopt;
  }

  float value;
  std::memcpy(&value, &data_[pos_], 4);
  pos_ += 4;
  decrementDataLeft(4);
  return value;
}

std::optional<std::string> DataChunkReader::readAsciiString(std::string *outError) {
  if (pos_ + 2 > data_.size()) {
    if (outError) {
      *outError = "Not enough data for string length";
    }
    return std::nullopt;
  }

  uint16_t length;
  std::memcpy(&length, &data_[pos_], 2);
  pos_ += 2;

  if (pos_ + length > data_.size()) {
    if (outError) {
      *outError = "String extends beyond file";
    }
    return std::nullopt;
  }

  std::string value;
  if (length > 0) {
    value.assign(reinterpret_cast<const char *>(&data_[pos_]), length);
    pos_ += length;
  }
  decrementDataLeft(2 + length);
  return value;
}

std::optional<std::string> DataChunkReader::readUnicodeString(std::string *outError) {
  if (pos_ + 2 > data_.size()) {
    if (outError) {
      *outError = "Not enough data for unicode string char count";
    }
    return std::nullopt;
  }

  uint16_t charCount;
  std::memcpy(&charCount, &data_[pos_], 2);
  pos_ += 2;

  size_t byteCount = static_cast<size_t>(charCount) * 2;
  if (pos_ + byteCount > data_.size()) {
    if (outError) {
      *outError = "Unicode string extends beyond file";
    }
    return std::nullopt;
  }

  std::string result;
  result.reserve(charCount);

  for (uint16_t i = 0; i < charCount; ++i) {
    uint16_t utf16char;
    std::memcpy(&utf16char, &data_[pos_ + i * 2], 2);

    if (utf16char < 0x80) {
      result += static_cast<char>(utf16char);
    } else {
      result += '?';
    }
  }

  pos_ += byteCount;
  decrementDataLeft(static_cast<uint32_t>(2 + byteCount));
  return result;
}

std::optional<Dict> DataChunkReader::readDict(std::string *outError) {
  if (pos_ + 2 > data_.size()) {
    if (outError) {
      *outError = "Not enough data for dict pair count";
    }
    return std::nullopt;
  }

  uint16_t pairCount;
  std::memcpy(&pairCount, &data_[pos_], 2);
  pos_ += 2;
  decrementDataLeft(2);

  Dict dict;
  for (uint16_t i = 0; i < pairCount; ++i) {
    auto keyAndType = readInt(outError);
    if (!keyAndType) {
      return std::nullopt;
    }

    uint8_t typeValue = static_cast<uint8_t>(*keyAndType & 0xFF);
    uint32_t keyId = static_cast<uint32_t>(*keyAndType >> 8);

    auto keyName = lookupName(keyId);
    if (!keyName) {
      if (outError) {
        *outError = "Unknown key ID in dict";
      }
      return std::nullopt;
    }

    DataType type = static_cast<DataType>(typeValue);
    DictValue value;
    value.type = type;

    switch (type) {
    case DataType::Bool: {
      auto b = readByte(outError);
      if (!b) {
        return std::nullopt;
      }
      value.boolValue = (*b != 0);
      break;
    }
    case DataType::Int: {
      auto intVal = readInt(outError);
      if (!intVal) {
        return std::nullopt;
      }
      value.intValue = *intVal;
      break;
    }
    case DataType::Real: {
      auto r = readReal(outError);
      if (!r) {
        return std::nullopt;
      }
      value.realValue = *r;
      break;
    }
    case DataType::AsciiString: {
      auto s = readAsciiString(outError);
      if (!s) {
        return std::nullopt;
      }
      value.stringValue = *s;
      break;
    }
    case DataType::UnicodeString: {
      auto s = readUnicodeString(outError);
      if (!s) {
        return std::nullopt;
      }
      value.stringValue = *s;
      break;
    }
    default:
      if (outError) {
        *outError = "Unknown data type in dict";
      }
      return std::nullopt;
    }

    dict[*keyName] = value;
  }

  return dict;
}

bool DataChunkReader::readBytes(uint8_t *dest, size_t count, std::string *outError) {
  if (pos_ + count > data_.size()) {
    if (outError) {
      *outError = "Not enough data for byte array";
    }
    return false;
  }

  std::memcpy(dest, &data_[pos_], count);
  pos_ += count;
  decrementDataLeft(static_cast<uint32_t>(count));
  return true;
}

} // namespace map
