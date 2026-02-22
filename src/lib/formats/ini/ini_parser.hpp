#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ini {

struct RGBColor {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
};

struct RGBAColorInt {
  int32_t r = 0;
  int32_t g = 0;
  int32_t b = 0;
  int32_t a = 255;
};

class IniParser {
public:
  using BlockHandler = std::function<void(IniParser &parser, const std::string &blockName)>;

  IniParser() = default;

  void registerBlock(const std::string &blockType, BlockHandler handler);

  std::optional<std::string> parse(std::string_view text);

  std::string getNextToken();

  std::string parseAsciiString();
  bool parseBool();
  int32_t parseInt();
  float parseReal();
  RGBColor parseRGBColor();
  RGBAColorInt parseRGBAColorInt();
  int32_t parseIndexList(const std::vector<std::string> &names);

  void parseBlock(
      const std::vector<
          std::pair<std::string, std::function<void(IniParser &parser, void *object)>>> &fields,
      void *object);

  bool atEndOfLine() const;

private:
  void skipWhitespaceAndComments();
  void skipToNextLine();
  bool isEndToken(const std::string &token) const;

  std::string_view text_;
  size_t pos_ = 0;
  int32_t line_ = 1;

  std::unordered_map<std::string, BlockHandler> blockHandlers_;
};

} // namespace ini
