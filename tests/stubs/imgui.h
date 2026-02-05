#pragma once

// Minimal ImGui stub for testing UI logic without actual ImGui

#include <cmath>
#include <cstring>

struct ImVec2 {
  float x, y;
  ImVec2() : x(0), y(0) {}
  ImVec2(float _x, float _y) : x(_x), y(_y) {}
};

struct ImVec4 {
  float x, y, z, w;
  ImVec4() : x(0), y(0), z(0), w(0) {}
  ImVec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

enum ImGuiKey {
  ImGuiKey_Escape,
  ImGuiKey_COUNT
};

enum ImGuiTreeNodeFlags_ {
  ImGuiTreeNodeFlags_None = 0,
  ImGuiTreeNodeFlags_OpenOnArrow = 1 << 0,
  ImGuiTreeNodeFlags_OpenOnDoubleClick = 1 << 1,
  ImGuiTreeNodeFlags_Leaf = 1 << 2,
  ImGuiTreeNodeFlags_DefaultOpen = 1 << 3
};
typedef int ImGuiTreeNodeFlags;

enum ImGuiSelectableFlags_ {
  ImGuiSelectableFlags_None = 0,
  ImGuiSelectableFlags_AllowDoubleClick = 1 << 0
};
typedef int ImGuiSelectableFlags;

enum ImGuiChildFlags_ {
  ImGuiChildFlags_None = 0,
  ImGuiChildFlags_Borders = 1 << 0
};
typedef int ImGuiChildFlags;

enum ImGuiInputTextFlags_ {
  ImGuiInputTextFlags_None = 0,
  ImGuiInputTextFlags_EnterReturnsTrue = 1 << 0
};
typedef int ImGuiInputTextFlags;

enum ImGuiWindowFlags_ {
  ImGuiWindowFlags_None = 0,
  ImGuiWindowFlags_AlwaysAutoResize = 1 << 1
};
typedef int ImGuiWindowFlags;

enum ImGuiCond_ {
  ImGuiCond_Always = 0,
  ImGuiCond_Appearing = 1 << 1
};
typedef int ImGuiCond;

struct ImGuiStyle {
  float ItemSpacing;
};

namespace ImGui {

inline ImGuiStyle &GetStyle() {
  static ImGuiStyle style;
  style.ItemSpacing = 5.0f;
  return style;
}

inline ImVec2 GetContentRegionAvail() {
  return ImVec2(400, 300);
}

inline float GetFrameHeightWithSpacing() {
  return 20.0f;
}

inline float GetWindowWidth() {
  return 400.0f;
}

inline bool Begin(const char *, bool * = nullptr, int = 0) { return true; }
inline void End() {}

inline bool BeginPopupModal(const char *, bool * = nullptr, int = 0) { return true; }
inline void EndPopup() {}
inline void OpenPopup(const char *) {}
inline void CloseCurrentPopup() {}

inline bool BeginChild(const char *, ImVec2 = ImVec2(), bool = false, int = 0) { return true; }
inline void EndChild() {}

inline bool Button(const char *, ImVec2 = ImVec2(0, 0)) { return false; }
inline void SameLine([[maybe_unused]] float offset = 0, [[maybe_unused]] float spacing = -1) {}
inline bool Checkbox(const char *, bool *v) { return *v; }
inline bool Selectable(const char *, bool = false, int = 0) { return false; }
inline void Text(const char *, ...) {}
inline void TextDisabled(const char *, ...) {}
inline bool TreeNode(const char *) { return false; }
inline void TreePop() {}
inline void SetNextItemWidth(float) {}
inline bool InputText(const char *, char *, size_t, int = 0) { return false; }
inline bool IsMouseDoubleClicked(int = 0) { return false; }
inline bool IsKeyPressed(int, bool = false) { return false; }
inline void Separator() {}
inline void Spacing() {}
inline ImVec2 GetMainViewport() { return ImVec2(); }
inline ImVec2 GetCenter() { return ImVec2(0, 0); }

} // namespace ImGui
