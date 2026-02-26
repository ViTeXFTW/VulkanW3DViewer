#include "refpack.hpp"

#include <cstring>

namespace map::refpack {

bool isCompressed(std::span<const uint8_t> data) {
  if (data.size() < 8) {
    return false;
  }
  return std::memcmp(data.data(), GENERALS_MAGIC, 4) == 0;
}

uint32_t getUncompressedSize(std::span<const uint8_t> data) {
  if (data.size() < 8) {
    return 0;
  }
  uint32_t size = 0;
  std::memcpy(&size, data.data() + 4, 4);
  return size;
}

std::optional<std::vector<uint8_t>> decompress(std::span<const uint8_t> data,
                                               std::string *outError) {
  if (data.size() < 8) {
    if (outError) {
      *outError = "Data too small for compression header";
    }
    return std::nullopt;
  }

  if (std::memcmp(data.data(), GENERALS_MAGIC, 4) != 0) {
    if (outError) {
      *outError = "Missing EAR compression magic";
    }
    return std::nullopt;
  }

  uint32_t uncompressedSize = getUncompressedSize(data);
  if (uncompressedSize == 0) {
    return std::vector<uint8_t>{};
  }

  constexpr uint32_t MAX_UNCOMPRESSED = 64 * 1024 * 1024;
  if (uncompressedSize > MAX_UNCOMPRESSED) {
    if (outError) {
      *outError = "Uncompressed size too large: " + std::to_string(uncompressedSize);
    }
    return std::nullopt;
  }

  const uint8_t *s = data.data() + 8;
  const uint8_t *sEnd = data.data() + data.size();

  if (sEnd - s < 5) {
    if (outError) {
      *outError = "Data too small for RefPack stream header";
    }
    return std::nullopt;
  }

  uint16_t packType = static_cast<uint16_t>((s[0] << 8) | s[1]);
  s += 2;

  if ((packType & 0x00FF) != 0x00FB) {
    if (outError) {
      *outError = "Invalid RefPack signature (expected 0xFB low byte)";
    }
    return std::nullopt;
  }

  int sizeBytes = (packType & 0x8000) ? 4 : 3;

  if (packType & 0x0100) {
    s += sizeBytes;
  }

  s += sizeBytes;

  if (s > sEnd) {
    if (outError) {
      *outError = "RefPack header extends past end of data";
    }
    return std::nullopt;
  }

  std::vector<uint8_t> dest(uncompressedSize);
  uint8_t *d = dest.data();
  uint8_t *dEnd = dest.data() + uncompressedSize;

  for (;;) {
    if (s >= sEnd) {
      if (outError) {
        *outError = "Unexpected end of compressed data";
      }
      return std::nullopt;
    }

    uint8_t first = *s++;

    if (!(first & 0x80)) {
      if (s >= sEnd) {
        if (outError) {
          *outError = "Truncated short reference command";
        }
        return std::nullopt;
      }
      uint8_t second = *s++;

      uint32_t numLiterals = first & 0x03;
      if (s + numLiterals > sEnd || d + numLiterals > dEnd) {
        if (outError) {
          *outError = "Short ref literal overflow";
        }
        return std::nullopt;
      }
      for (uint32_t i = 0; i < numLiterals; ++i) {
        *d++ = *s++;
      }

      uint32_t offset = (static_cast<uint32_t>(first & 0x60) << 3) + second + 1;
      uint32_t count = ((first & 0x1C) >> 2) + 3;

      if (offset > static_cast<uint32_t>(d - dest.data())) {
        if (outError) {
          *outError = "Short ref offset exceeds output position";
        }
        return std::nullopt;
      }
      if (d + count > dEnd) {
        if (outError) {
          *outError = "Short ref copy overflow";
        }
        return std::nullopt;
      }

      const uint8_t *ref = d - offset;
      for (uint32_t i = 0; i < count; ++i) {
        *d++ = *ref++;
      }

    } else if (!(first & 0x40)) {
      if (s + 2 > sEnd) {
        if (outError) {
          *outError = "Truncated medium reference command";
        }
        return std::nullopt;
      }
      uint8_t second = *s++;
      uint8_t third = *s++;

      uint32_t numLiterals = second >> 6;
      if (s + numLiterals > sEnd || d + numLiterals > dEnd) {
        if (outError) {
          *outError = "Medium ref literal overflow";
        }
        return std::nullopt;
      }
      for (uint32_t i = 0; i < numLiterals; ++i) {
        *d++ = *s++;
      }

      uint32_t offset = (static_cast<uint32_t>(second & 0x3F) << 8) + third + 1;
      uint32_t count = (first & 0x3F) + 4;

      if (offset > static_cast<uint32_t>(d - dest.data())) {
        if (outError) {
          *outError = "Medium ref offset exceeds output position";
        }
        return std::nullopt;
      }
      if (d + count > dEnd) {
        if (outError) {
          *outError = "Medium ref copy overflow";
        }
        return std::nullopt;
      }

      const uint8_t *ref = d - offset;
      for (uint32_t i = 0; i < count; ++i) {
        *d++ = *ref++;
      }

    } else if (!(first & 0x20)) {
      if (s + 3 > sEnd) {
        if (outError) {
          *outError = "Truncated long reference command";
        }
        return std::nullopt;
      }
      uint8_t second = *s++;
      uint8_t third = *s++;
      uint8_t forth = *s++;

      uint32_t numLiterals = first & 0x03;
      if (s + numLiterals > sEnd || d + numLiterals > dEnd) {
        if (outError) {
          *outError = "Long ref literal overflow";
        }
        return std::nullopt;
      }
      for (uint32_t i = 0; i < numLiterals; ++i) {
        *d++ = *s++;
      }

      uint32_t offset = ((static_cast<uint32_t>(first & 0x10) >> 4) << 16) +
                        (static_cast<uint32_t>(second) << 8) + third + 1;
      uint32_t count = ((static_cast<uint32_t>(first & 0x0C) >> 2) << 8) + forth + 5;

      if (offset > static_cast<uint32_t>(d - dest.data())) {
        if (outError) {
          *outError = "Long ref offset exceeds output position";
        }
        return std::nullopt;
      }
      if (d + count > dEnd) {
        if (outError) {
          *outError = "Long ref copy overflow";
        }
        return std::nullopt;
      }

      const uint8_t *ref = d - offset;
      for (uint32_t i = 0; i < count; ++i) {
        *d++ = *ref++;
      }

    } else {
      uint32_t run = ((first & 0x1F) << 2) + 4;
      if (run <= 112) {
        if (s + run > sEnd || d + run > dEnd) {
          if (outError) {
            *outError = "Literal run overflow";
          }
          return std::nullopt;
        }
        for (uint32_t i = 0; i < run; ++i) {
          *d++ = *s++;
        }
      } else {
        run = first & 0x03;
        if (s + run > sEnd || d + run > dEnd) {
          if (outError) {
            *outError = "EOF literal overflow";
          }
          return std::nullopt;
        }
        for (uint32_t i = 0; i < run; ++i) {
          *d++ = *s++;
        }
        break;
      }
    }
  }

  return dest;
}

} // namespace map::refpack
