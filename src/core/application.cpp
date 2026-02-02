#include "application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "ui/hover_tooltip.hpp"
#include "ui/ui_context.hpp"
#include "ui/viewport_window.hpp"

#include <imgui.h>

namespace w3d {

void Application::setTexturePath(const std::string &path) {
  customTexturePath_ = path;
  modelLoader_.setTexturePath(path);
}

void Application::setDebugMode(bool debug) {
  debugMode_ = debug;
  modelLoader_.setDebugMode(debug);
}

void Application::setInitialModel(const std::string &path) {
  initialModelPath_ = path;
}

void Application::framebufferResizeCallback(GLFWwindow *window, int /*width*/, int /*height*/) {
  auto *app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  app->renderer_.setFramebufferResized(true);
}

void Application::scrollCallback(GLFWwindow *window, double /*xoffset*/, double yoffset) {
  auto *app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  app->camera_.onScroll(static_cast<float>(yoffset));
}

void Application::initWindow() {
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window_ = glfwCreateWindow(WIDTH, HEIGHT, "W3D Viewer", nullptr, nullptr);
  if (!window_) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
  glfwSetScrollCallback(window_, scrollCallback);
}

void Application::initVulkan() {
#ifdef W3D_DEBUG
  context_.init(window_, true); // Enable validation in debug builds
#else
  context_.init(window_, false); // Disable validation in release builds
#endif

  // Create skeleton renderer
  skeletonRenderer_.create(context_);

  // Create bone matrix buffer for GPU skinning
  boneMatrixBuffer_.create(context_);

  // Initialize texture manager and create default texture
  textureManager_.init(context_);

  // Set texture path - use command line override if provided
  std::filesystem::path texturePath;
  if (!customTexturePath_.empty()) {
    texturePath = customTexturePath_;
  } else {
    // Default: relative to working directory
    texturePath = "resources/textures";
    if (!std::filesystem::exists(texturePath)) {
      // Try relative to executable location
      texturePath =
          std::filesystem::path(__FILE__).parent_path().parent_path() / "resources" / "textures";
    }
  }
  textureManager_.setTexturePath(texturePath);

#ifdef W3D_DEBUG
  if (debugMode_) {
    std::cerr << "[DEBUG] Texture path set to: " << textureManager_.texturePath().string() << "\n";
    std::cerr << "[DEBUG] Path exists: " << std::filesystem::exists(texturePath) << "\n";
  }
#endif

  // Initialize renderer
  renderer_.init(window_, context_, imguiBackend_, textureManager_, boneMatrixBuffer_);
}

void Application::initUI() {
  imguiBackend_.init(window_, context_);

  // Register windows with UI manager
  auto *viewport = uiManager_.addWindow<ViewportWindow>();
  console_ = uiManager_.addWindow<ConsoleWindow>();
  fileBrowser_ = uiManager_.addWindow<FileBrowser>();
  uiManager_.addWindow<HoverTooltip>();

  // Set initial visibility
  viewport->setVisible(true);
  console_->setVisible(true);
  fileBrowser_->setVisible(false);

  // Configure file browser
  fileBrowser_->setFilter(".w3d");
  fileBrowser_->setFileSelectedCallback([this](const std::filesystem::path &path) {
    loadW3DFile(path);
    fileBrowser_->setVisible(false);
  });

  // Welcome message
  console_->info("W3D Viewer initialized");
  console_->log("Use File > Open to load a W3D model");
}

void Application::loadW3DFile(const std::filesystem::path &path) {
  auto logCallback = [this](const std::string &msg) {
    // Determine message type based on content
    if (msg.find("Error") != std::string::npos || msg.find("Failed") != std::string::npos) {
      console_->error(msg);
    } else if (msg.find("Loading") != std::string::npos ||
               msg.find("Loaded") != std::string::npos) {
      console_->info(msg);
    } else {
      console_->addMessage(msg);
    }
  };

  auto result = modelLoader_.load(path, context_, textureManager_, boneMatrixBuffer_,
                                  renderableMesh_, hlodModel_, skeletonPose_, skeletonRenderer_,
                                  animationPlayer_, camera_, logCallback);

  if (!result.success) {
    console_->error(result.error);
    return;
  }

  renderState_.useHLodModel = result.useHLodModel;
  renderState_.useSkinnedRendering = result.useSkinnedRendering;
  renderState_.lastAppliedFrame = -1.0f; // Reset animation state for new model
}

void Application::updateHover() {
  // Reset hover state by default
  hoverDetector_.state().reset();

  // Skip if ImGui wants mouse (over UI elements)
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  // Get mouse position in window coordinates
  double mouseX, mouseY;
  glfwGetCursorPos(window_, &mouseX, &mouseY);

  // Get swapchain (full render target) dimensions
  auto extent = context_.swapchainExtent();

  // Get camera matrices (must match rendering)
  auto view = camera_.viewMatrix();
  auto proj = glm::perspective(glm::radians(45.0f),
                               static_cast<float>(extent.width) / static_cast<float>(extent.height),
                               0.01f, 10000.0f);
  proj[1][1] *= -1; // Vulkan Y-flip

  // Update hover detector with ray
  hoverDetector_.update(
      glm::vec2(static_cast<float>(mouseX), static_cast<float>(mouseY)),
      glm::vec2(static_cast<float>(extent.width), static_cast<float>(extent.height)), view, proj);

  // Test skeleton first (priority over meshes)
  if (renderState_.showSkeleton && skeletonRenderer_.hasData()) {
    hoverDetector_.testSkeleton(skeletonRenderer_, 0.05f);
  }

  // Test meshes
  if (renderState_.showMesh) {
    if (renderState_.useHLodModel && hlodModel_.hasData()) {
      // TODO: Implement HLod hover detection
    } else if (renderableMesh_.hasData()) {
      hoverDetector_.testMeshes(renderableMesh_);
    }
  }
}

void Application::drawUI() {
  // Build UI context with current application state
  UIContext ctx;
  ctx.window = window_;
  ctx.loadedFile = modelLoader_.loadedFile() ? &*modelLoader_.loadedFile() : nullptr;
  ctx.loadedFilePath = modelLoader_.loadedFilePath();
  ctx.renderState = &renderState_;
  ctx.hlodModel = &hlodModel_;
  ctx.renderableMesh = &renderableMesh_;
  ctx.camera = &camera_;
  ctx.skeletonPose = &skeletonPose_;
  ctx.animationPlayer = &animationPlayer_;
  ctx.hoverState = &hoverDetector_.state();

  // Set up callbacks
  ctx.onOpenFile = [this]() { fileBrowser_->setVisible(true); };
  ctx.onExit = [this]() { glfwSetWindowShouldClose(window_, GLFW_TRUE); };
  ctx.onResetCamera = [this]() {
    if (renderState_.useHLodModel && hlodModel_.hasData()) {
      const auto &bounds = hlodModel_.bounds();
      camera_.setTarget(bounds.center(), bounds.radius() * 2.5f);
    } else if (renderableMesh_.hasData()) {
      const auto &bounds = renderableMesh_.bounds();
      camera_.setTarget(bounds.center(), bounds.radius() * 2.5f);
    }
  };

  // Draw all UI through the manager
  uiManager_.draw(ctx);
}

void Application::mainLoop() {
  lastFrameTime_ = static_cast<float>(glfwGetTime());

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    // Calculate delta time
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentTime - lastFrameTime_;
    lastFrameTime_ = currentTime;

    // Update camera
    camera_.update(window_);

    // Update hover detection
    updateHover();

    // Update animation
    animationPlayer_.update(deltaTime);

    // Apply animation to pose only when frame changes
    if (modelLoader_.loadedFile() && animationPlayer_.animationCount() > 0 &&
        !modelLoader_.loadedFile()->hierarchies.empty()) {
      float currentFrame = animationPlayer_.currentFrame();
      if (currentFrame != renderState_.lastAppliedFrame || !animationPlayer_.isPlaying()) {
        animationPlayer_.applyToPose(skeletonPose_, modelLoader_.loadedFile()->hierarchies[0]);

        // Wait for current frame fence before updating any per-frame GPU resources
        renderer_.waitForCurrentFrame();
        uint32_t frameIndex = renderer_.currentFrame();

        // Update skeleton debug visualization (double-buffered)
        skeletonRenderer_.updateFromPose(context_, frameIndex, skeletonPose_);

        // Update bone matrix buffer for GPU skinning (double-buffered)
        if (renderState_.useSkinnedRendering && skeletonPose_.isValid()) {
          auto skinningMatrices = skeletonPose_.getSkinningMatrices();
          boneMatrixBuffer_.update(frameIndex, skinningMatrices);
        }

        renderState_.lastAppliedFrame = currentFrame;
      }
    }

    // Update LOD selection based on camera distance
    if (renderState_.useHLodModel && hlodModel_.hasData()) {
      auto extent = context_.swapchainExtent();
      float screenHeight = static_cast<float>(extent.height);
      float fovY = glm::radians(45.0f); // Must match projection FOV
      float cameraDistance = camera_.distance();

      hlodModel_.updateLOD(screenHeight, fovY, cameraDistance);
    }

    // Start ImGui frame
    imguiBackend_.newFrame();
    drawUI();

    // Draw frame
    FrameContext frameCtx{camera_,           renderableMesh_, hlodModel_,
                          skeletonRenderer_, hoverDetector_,  renderState_};
    renderer_.drawFrame(frameCtx);
  }

  context_.device().waitIdle();
}

void Application::cleanup() {
  imguiBackend_.cleanup();
  renderer_.cleanup();

  skeletonRenderer_.destroy();
  hlodModel_.destroy();
  renderableMesh_.destroy();
  textureManager_.destroy();
  boneMatrixBuffer_.destroy();
  context_.cleanup();

  if (window_) {
    glfwDestroyWindow(window_);
  }
  glfwTerminate();
}

void Application::run() {
  initWindow();
  initVulkan();
  initUI();

  // Load initial model if specified via command line
  if (!initialModelPath_.empty()) {
    loadW3DFile(initialModelPath_);
  }

  mainLoop();
  cleanup();
}

} // namespace w3d
