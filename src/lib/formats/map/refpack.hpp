#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace map::refpack {

constexpr uint8_t GENERALS_MAGIC[4] = {'E', 'A', 'R', '\0'};

[[nodiscard]] bool isCompressed(std::span<const uint8_t> data);

[[nodiscard]] uint32_t getUncompressedSize(std::span<const uint8_t> data);

[[nodiscard]] std::optional<std::vector<uint8_t>> decompress(std::span<const uint8_t> data,
                                                             std::string *outError = nullptr);

} // namespace map::refpack
