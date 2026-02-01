#pragma once

#include "core/renderer.hpp"
#include "core/vulkan_context.hpp"
#include "render/animation_player.hpp"
#include "render/bone_buffer.hpp"
#include "render/camera.hpp"
#include "render/hlod_model.hpp"
#include "render/hover_detector.hpp"
#include "render/renderable_mesh.hpp"
#include "render/skeleton.hpp"
#include "render/skeleton_renderer.hpp"
#include "render/texture.hpp"
#include "ui/console_window.hpp"
#include "ui/file_browser.hpp"
#include "ui/imgui_backend.hpp"
#include "ui/ui_manager.hpp"
#include "w3d/loader.hpp"
#include "w3d/model_loader.hpp"

#include <GLFW/glfw3.h>

#include <optional>
#include <string>

namespace w3d {

/**
 * Main application class managing the window, Vulkan context,
 * UI, and main loop.
 */
class Application {
public:
  Application() = default;
  ~Application() = default;

  // Non-copyable
  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  /**
   * Run the application.
   */
  void run();

  /**
   * Set custom texture path.
   */
  void setTexturePath(const std::string &path);

  /**
   * Enable/disable debug mode.
   */
  void setDebugMode(bool debug);

  /**
   * Set initial model to load.
   */
  void setInitialModel(const std::string &path);

private:
  static constexpr uint32_t WIDTH = 1280;
  static constexpr uint32_t HEIGHT = 720;

  // GLFW callbacks
  static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
  static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

  // Initialization
  void initWindow();
  void initVulkan();
  void initUI();

  // Main loop
  void mainLoop();
  void updateHover();
  void drawUI();

  // Cleanup
  void cleanup();

  // Model loading
  void loadW3DFile(const std::filesystem::path &path);

  // Command line options
  std::string customTexturePath_;
  std::string initialModelPath_;
  bool debugMode_ = false;

  // Window and context
  GLFWwindow *window_ = nullptr;
  VulkanContext context_;

  // Rendering
  Renderer renderer_;
  TextureManager textureManager_;
  BoneMatrixBuffer boneMatrixBuffer_;

  // Loaded W3D data
  ModelLoader modelLoader_;
  bool useHLodModel_ = false;
  bool useSkinnedRendering_ = false;

  // Mesh rendering
  RenderableMesh renderableMesh_;
  HLodModel hlodModel_;
  Camera camera_;

  // Skeleton rendering
  SkeletonRenderer skeletonRenderer_;
  SkeletonPose skeletonPose_;
  bool showSkeleton_ = true;
  bool showMesh_ = true;

  // Animation playback
  AnimationPlayer animationPlayer_;
  float lastFrameTime_ = 0.0f;
  float lastAppliedFrame_ = -1.0f;

  // Hover detection
  HoverDetector hoverDetector_;

  // UI components
  ImGuiBackend imguiBackend_;
  UIManager uiManager_;
  ConsoleWindow *console_ = nullptr;   // Owned by uiManager_
  FileBrowser *fileBrowser_ = nullptr; // Owned by uiManager_
};

} // namespace w3d
