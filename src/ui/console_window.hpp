#pragma once

#include <string>
#include <vector>

#include "ui_window.hpp"

namespace w3d {

/// Debug console window for displaying log messages.
/// Supports different log levels with color coding and auto-scroll.
class ConsoleWindow : public UIWindow {
public:
  ConsoleWindow() = default;

  // UIWindow interface
  void draw(UIContext &ctx) override;
  const char *name() const override { return "Console"; }

  // Clear all messages
  void clear();

  // Add a message (no timestamp, no level)
  void addMessage(const std::string &message);

  // Add message with timestamp
  void log(const std::string &message);

  // Add an info message (green)
  void info(const std::string &message);

  // Add a warning message (yellow)
  void warning(const std::string &message);

  // Add an error message (red)
  void error(const std::string &message);

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

} // namespace w3d
