#include "lib/formats/ini/ini_parser.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace ini {

void IniParser::registerBlock(const std::string &blockType, BlockHandler handler) {
  blockHandlers_[blockType] = std::move(handler);
}

std::optional<std::string> IniParser::parse(std::string_view text) {
  text_ = text;
  pos_ = 0;
  line_ = 1;

  while (pos_ < text_.size()) {
    skipWhitespaceAndComments();
    if (pos_ >= text_.size()) {
      break;
    }

    auto token = getNextToken();
    if (token.empty()) {
      break;
    }

    auto it = blockHandlers_.find(token);
    if (it != blockHandlers_.end()) {
      std::string blockName;
      if (pos_ < text_.size() && text_[pos_] != '\n' && text_[pos_] != '\r' && text_[pos_] != ';') {
        blockName = getNextToken();
      }
      it->second(*this, blockName);
    } else {
      skipToNextLine();
    }
  }

  return std::nullopt;
}

std::string IniParser::getNextToken() {
  while (pos_ < text_.size() && (text_[pos_] == ' ' || text_[pos_] == '\t' || text_[pos_] == '=')) {
    ++pos_;
  }

  if (pos_ >= text_.size() || text_[pos_] == '\n' || text_[pos_] == '\r' || text_[pos_] == ';') {
    return {};
  }

  size_t start = pos_;
  while (pos_ < text_.size() && text_[pos_] != ' ' && text_[pos_] != '\t' && text_[pos_] != '\n' &&
         text_[pos_] != '\r' && text_[pos_] != '=' && text_[pos_] != ';') {
    ++pos_;
  }

  return std::string(text_.substr(start, pos_ - start));
}

std::string IniParser::parseAsciiString() {
  return getNextToken();
}

bool IniParser::parseBool() {
  auto token = getNextToken();
  std::string lower = token;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return lower == "yes" || lower == "true" || lower == "1";
}

int32_t IniParser::parseInt() {
  auto token = getNextToken();
  if (token.empty()) {
    return 0;
  }
  return std::stoi(token);
}

float IniParser::parseReal() {
  auto token = getNextToken();
  if (token.empty()) {
    return 0.0f;
  }
  return std::stof(token);
}

RGBColor IniParser::parseRGBColor() {
  RGBColor color;

  auto token = getNextToken();
  if (token.size() >= 2 && (token[0] == 'R' || token[0] == 'r') && token[1] == ':') {
    color.r = std::stof(token.substr(2));
  }

  token = getNextToken();
  if (token.size() >= 2 && (token[0] == 'G' || token[0] == 'g') && token[1] == ':') {
    color.g = std::stof(token.substr(2));
  }

  token = getNextToken();
  if (token.size() >= 2 && (token[0] == 'B' || token[0] == 'b') && token[1] == ':') {
    color.b = std::stof(token.substr(2));
  }

  return color;
}

RGBAColorInt IniParser::parseRGBAColorInt() {
  RGBAColorInt color;

  auto token = getNextToken();
  if (token.size() >= 2 && (token[0] == 'R' || token[0] == 'r') && token[1] == ':') {
    color.r = std::stoi(token.substr(2));
  }

  token = getNextToken();
  if (token.size() >= 2 && (token[0] == 'G' || token[0] == 'g') && token[1] == ':') {
    color.g = std::stoi(token.substr(2));
  }

  token = getNextToken();
  if (token.size() >= 2 && (token[0] == 'B' || token[0] == 'b') && token[1] == ':') {
    color.b = std::stoi(token.substr(2));
  }

  token = getNextToken();
  if (!token.empty() && token.size() >= 2 && (token[0] == 'A' || token[0] == 'a') &&
      token[1] == ':') {
    color.a = std::stoi(token.substr(2));
  }

  return color;
}

int32_t IniParser::parseIndexList(const std::vector<std::string> &names) {
  auto token = getNextToken();
  for (size_t i = 0; i < names.size(); ++i) {
    if (names[i] == token) {
      return static_cast<int32_t>(i);
    }
  }
  return 0;
}

void IniParser::parseBlock(
    const std::vector<std::pair<std::string, std::function<void(IniParser &parser, void *object)>>>
        &fields,
    void *object) {
  skipToNextLine();

  while (pos_ < text_.size()) {
    skipWhitespaceAndComments();
    if (pos_ >= text_.size()) {
      break;
    }

    auto token = getNextToken();
    if (token.empty()) {
      continue;
    }

    if (isEndToken(token)) {
      skipToNextLine();
      return;
    }

    bool found = false;
    for (const auto &[fieldName, handler] : fields) {
      if (fieldName == token) {
        handler(*this, object);
        found = true;
        break;
      }
    }

    if (!found) {
      skipToNextLine();
      continue;
    }

    skipToNextLine();
  }
}

bool IniParser::atEndOfLine() const {
  size_t p = pos_;
  while (p < text_.size() && (text_[p] == ' ' || text_[p] == '\t' || text_[p] == '=')) {
    ++p;
  }
  return p >= text_.size() || text_[p] == '\n' || text_[p] == '\r' || text_[p] == ';';
}

void IniParser::skipWhitespaceAndComments() {
  while (pos_ < text_.size()) {
    char c = text_[pos_];
    if (c == '\n') {
      ++pos_;
      ++line_;
    } else if (c == '\r') {
      ++pos_;
      if (pos_ < text_.size() && text_[pos_] == '\n') {
        ++pos_;
      }
      ++line_;
    } else if (c == ' ' || c == '\t') {
      ++pos_;
    } else if (c == ';') {
      while (pos_ < text_.size() && text_[pos_] != '\n' && text_[pos_] != '\r') {
        ++pos_;
      }
    } else {
      break;
    }
  }
}

void IniParser::skipToNextLine() {
  while (pos_ < text_.size() && text_[pos_] != '\n' && text_[pos_] != '\r') {
    ++pos_;
  }
  if (pos_ < text_.size()) {
    if (text_[pos_] == '\r') {
      ++pos_;
      if (pos_ < text_.size() && text_[pos_] == '\n') {
        ++pos_;
      }
    } else {
      ++pos_;
    }
    ++line_;
  }
}

bool IniParser::isEndToken(const std::string &token) const {
  if (token.size() != 3) {
    return false;
  }
  return (token[0] == 'E' || token[0] == 'e') && (token[1] == 'N' || token[1] == 'n') &&
         (token[2] == 'D' || token[2] == 'd');
}

} // namespace ini
