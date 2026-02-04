// Minimal ImGui stub for unit testing UI logic without actual ImGui
// This provides just enough to compile file_browser.cpp without the real ImGui library

#pragma once

// Minimal type definitions
struct ImVec2 {
  float x, y;
  ImVec2() : x(0), y(0) {}
  ImVec2(float _x, float _y) : x(_x), y(_y) {}
};

using ImGuiID = unsigned int;

// Flags (minimal set needed by file_browser.cpp)
enum ImGuiInputTextFlags_ { ImGuiInputTextFlags_EnterReturnsTrue = 1 << 5 };
using ImGuiInputTextFlags = int;

enum ImGuiSelectableFlags_ { ImGuiSelectableFlags_AllowDoubleClick = 1 << 2 };
using ImGuiSelectableFlags = int;

enum ImGuiChildFlags_ { ImGuiChildFlags_Borders = 1 << 0 };
using ImGuiChildFlags = int;

// Stub functions (no-op implementations for linking)
namespace ImGui {

inline bool Begin(const char *, bool * = nullptr, int = 0) {
  return true;
}
inline void End() {}
inline bool Button(const char *, const ImVec2 & = ImVec2(0, 0)) {
  return false;
}
inline bool InputText(const char *, char *, size_t, int = 0) {
  return false;
}
inline bool Selectable(const char *, bool = false, int = 0, const ImVec2 & = ImVec2(0, 0)) {
  return false;
}
inline void Text(const char *, ...) {}
inline void SameLine(float = 0.0f, float = -1.0f) {}
inline void Separator() {}
inline void BeginChild(const char *, const ImVec2 & = ImVec2(0, 0), int = 0, int = 0) {}
inline void EndChild() {}
inline bool IsMouseDoubleClicked(int) {
  return false;
}
inline float GetWindowWidth() {
  return 100.0f;
}
inline float GetFrameHeightWithSpacing() {
  return 20.0f;
}
inline void SetNextItemWidth(float) {}

} // namespace ImGui
