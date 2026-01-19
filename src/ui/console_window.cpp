#include "console_window.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <imgui.h>

namespace w3d {

void ConsoleWindow::draw(bool* open) {
  if (!ImGui::Begin("Console", open)) {
    ImGui::End();
    return;
  }

  // Toolbar
  if (ImGui::Button("Clear")) {
    clear();
  }
  ImGui::SameLine();
  ImGui::Checkbox("Auto-scroll", &autoScroll_);

  ImGui::Separator();

  // Log content
  ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_None,
                    ImGuiWindowFlags_HorizontalScrollbar);

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

  for (const auto& entry : entries_) {
    ImVec4 color;
    const char* prefix = "";

    switch (entry.level) {
      case LogEntry::Level::Info:
        color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
        prefix = "[INFO] ";
        break;
      case LogEntry::Level::Warning:
        color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
        prefix = "[WARN] ";
        break;
      case LogEntry::Level::Error:
        color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
        prefix = "[ERROR] ";
        break;
      case LogEntry::Level::Plain:
      default:
        color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        break;
    }

    if (!entry.timestamp.empty()) {
      ImGui::TextDisabled("[%s]", entry.timestamp.c_str());
      ImGui::SameLine();
    }

    if (entry.level != LogEntry::Level::Plain) {
      ImGui::PushStyleColor(ImGuiCol_Text, color);
      ImGui::TextUnformatted(prefix);
      ImGui::PopStyleColor();
      ImGui::SameLine();
    }

    ImGui::TextUnformatted(entry.message.c_str());
  }

  ImGui::PopStyleVar();

  if (scrollToBottom_ || (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
    ImGui::SetScrollHereY(1.0f);
  }
  scrollToBottom_ = false;

  ImGui::EndChild();
  ImGui::End();
}

void ConsoleWindow::clear() {
  entries_.clear();
}

void ConsoleWindow::addMessage(const std::string& message) {
  LogEntry entry;
  entry.level = LogEntry::Level::Plain;
  entry.message = message;
  entries_.push_back(entry);
  scrollToBottom_ = true;
}

static std::string getCurrentTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#ifdef _WIN32
  localtime_s(&tm, &time);
#else
  localtime_r(&time, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S");
  return oss.str();
}

void ConsoleWindow::log(const std::string& message) {
  LogEntry entry;
  entry.level = LogEntry::Level::Plain;
  entry.timestamp = getCurrentTimestamp();
  entry.message = message;
  entries_.push_back(entry);
  scrollToBottom_ = true;
}

void ConsoleWindow::info(const std::string& message) {
  LogEntry entry;
  entry.level = LogEntry::Level::Info;
  entry.timestamp = getCurrentTimestamp();
  entry.message = message;
  entries_.push_back(entry);
  scrollToBottom_ = true;
}

void ConsoleWindow::warning(const std::string& message) {
  LogEntry entry;
  entry.level = LogEntry::Level::Warning;
  entry.timestamp = getCurrentTimestamp();
  entry.message = message;
  entries_.push_back(entry);
  scrollToBottom_ = true;
}

void ConsoleWindow::error(const std::string& message) {
  LogEntry entry;
  entry.level = LogEntry::Level::Error;
  entry.timestamp = getCurrentTimestamp();
  entry.message = message;
  entries_.push_back(entry);
  scrollToBottom_ = true;
}

}  // namespace w3d
