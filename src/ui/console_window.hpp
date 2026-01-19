#pragma once

#include <string>
#include <vector>

namespace w3d {

class ConsoleWindow {
 public:
  ConsoleWindow() = default;

  // Draw the console window
  void draw(bool* open = nullptr);

  // Clear all messages
  void clear();

  // Add a message
  void addMessage(const std::string& message);

  // Add message with timestamp
  void log(const std::string& message);

  // Add an info message
  void info(const std::string& message);

  // Add a warning message
  void warning(const std::string& message);

  // Add an error message
  void error(const std::string& message);

 private:
  struct LogEntry {
    enum class Level { Info, Warning, Error, Plain };
    Level level;
    std::string timestamp;
    std::string message;
  };

  std::vector<LogEntry> entries_;
  bool autoScroll_ = true;
  bool scrollToBottom_ = false;
};

}  // namespace w3d
