#include "ui_manager.hpp"

#include <imgui.h>

namespace w3d {

UIWindow *UIManager::addWindow(std::unique_ptr<UIWindow> window) {
  UIWindow *ptr = window.get();
  windows_.push_back(std::move(window));
  return ptr;
}

void UIManager::draw(UIContext &ctx) {
  // Call frame begin hooks
  for (auto &window : windows_) {
    window->onFrameBegin(ctx);
  }

  // Draw dockspace and menu
  drawDockspace();
  drawMenuBar(ctx);

  // End dockspace window
  ImGui::End();

  // Draw all visible windows
  for (auto &window : windows_) {
    if (window->isVisible()) {
      window->draw(ctx);
    }
  }

#ifdef W3D_DEBUG
  // Draw demo window in debug builds
  if (showDemoWindow_) {
    ImGui::ShowDemoWindow(&showDemoWindow_);
  }
#endif

  // Call frame end hooks
  for (auto &window : windows_) {
    window->onFrameEnd(ctx);
  }
}

void UIManager::drawDockspace() {
  // Create dockspace over the entire window
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
  windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  windowFlags |= ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  ImGui::Begin("DockSpace", nullptr, windowFlags);
  ImGui::PopStyleVar(3);

  // Create the dockspace
  ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
  ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
}

void UIManager::drawMenuBar(UIContext &ctx) {
  if (!ImGui::BeginMenuBar()) {
    return;
  }

  // File menu
  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Open W3D...", "Ctrl+O")) {
      if (ctx.onOpenFile) {
        ctx.onOpenFile();
      }
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Exit", "Alt+F4")) {
      if (ctx.onExit) {
        ctx.onExit();
      }
    }
    ImGui::EndMenu();
  }

  // View menu
  if (ImGui::BeginMenu("View")) {
    // Add menu items for all registered windows
    for (auto &window : windows_) {
      ImGui::MenuItem(window->name(), nullptr, window->visiblePtr());
    }

#ifdef W3D_DEBUG
    ImGui::Separator();
    ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow_);
#endif

    ImGui::EndMenu();
  }

  // Help menu
  if (ImGui::BeginMenu("Help")) {
    if (ImGui::MenuItem("About")) {
      // TODO: Could trigger an about dialog window
    }
    ImGui::EndMenu();
  }

  ImGui::EndMenuBar();
}

} // namespace w3d
