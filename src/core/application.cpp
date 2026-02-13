#include "application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "ui/hover_tooltip.hpp"
#include "ui/settings_window.hpp"
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

  // Use window size from settings, or defaults
  int width = appSettings_.windowWidth > 0 ? appSettings_.windowWidth : WIDTH;
  int height = appSettings_.windowHeight > 0 ? appSettings_.windowHeight : HEIGHT;

  window_ = glfwCreateWindow(width, height, "W3D Viewer", nullptr, nullptr);
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

  // Set texture path - priority: CLI arg > settings > default
  std::filesystem::path texturePath;
  if (!customTexturePath_.empty()) {
    texturePath = customTexturePath_;
  } else if (!appSettings_.texturePath.empty()) {
    texturePath = appSettings_.texturePath;
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

  // Initialize BIG archive manager
  initializeBigArchiveManager();

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
  uiManager_.addWindow<SettingsWindow>();

  // Set initial visibility
  viewport->setVisible(true);
  console_->setVisible(true);
  fileBrowser_->setVisible(false);

  // Configure file browser
  fileBrowser_->setFilter(".w3d");
  fileBrowser_->setPathSelectedCallback([this](const std::filesystem::path &path) {
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
      if (renderState_.useSkinnedRendering && hlodModel_.hasSkinning()) {
        // Test skinned meshes (uses rest-pose geometry)
        hoverDetector_.testHLodSkinnedMeshes(hlodModel_);
      } else {
        // Test regular meshes with bone-space ray transformation
        const SkeletonPose *pose = skeletonPose_.isValid() ? &skeletonPose_ : nullptr;
        hoverDetector_.testHLodMeshes(hlodModel_, pose);
      }
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
  ctx.settings = &appSettings_;

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

    // Skip rendering when window is minimized
    if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED)) {
      continue;
    }

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
  // Save window size to settings before cleanup
  if (window_) {
    int width, height;
    glfwGetWindowSize(window_, &width, &height);
    appSettings_.windowWidth = width;
    appSettings_.windowHeight = height;
  }

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
  // Load settings first (before any initialization)
  appSettings_ = Settings::loadDefault();

  // Apply display settings from persistent storage to render state
  renderState_.showMesh = appSettings_.showMesh;
  renderState_.showSkeleton = appSettings_.showSkeleton;

  initWindow();
  initVulkan();
  initUI();

  // Load initial model if specified via command line
  if (!initialModelPath_.empty()) {
    loadW3DFile(initialModelPath_);
  }

  mainLoop();

  cleanup();

  // Save settings after cleanup (which captures final window size)
  appSettings_.saveDefault();
}

void Application::initializeBigArchiveManager() {
  // Try to initialize from settings game directory
  if (!appSettings_.gameDirectory.empty()) {
    std::filesystem::path gameDir(appSettings_.gameDirectory);

    // Check if directory exists
    std::error_code ec;
    if (std::filesystem::exists(gameDir, ec)) {
      // Initialize BIG archive manager
      std::string error;
      if (bigArchiveManager_.initialize(gameDir, &error)) {
        // Log to console if available, otherwise stderr
        if (console_) {
          console_->info("BIG archive manager initialized");
          console_->log("Game directory: " + gameDir.string());
          console_->log("Cache directory: " + bigArchiveManager_.cacheDirectory().string());
        } else {
          std::cerr << "BIG archive manager initialized\n";
          std::cerr << "Game directory: " << gameDir.string() << "\n";
          std::cerr << "Cache directory: " << bigArchiveManager_.cacheDirectory().string() << "\n";
        }

        // Scan archives to build registry
        if (assetRegistry_.scanArchives(gameDir, &error)) {
          if (console_) {
            console_->info("Asset registry scanned");
            console_->log("Models found: " + std::to_string(assetRegistry_.availableModels().size()));
            console_->log("Textures found: " + std::to_string(assetRegistry_.availableTextures().size()));
            console_->log("INI files found: " + std::to_string(assetRegistry_.availableIniFiles().size()));
          } else {
            std::cerr << "Asset registry scanned\n";
            std::cerr << "Models found: " << assetRegistry_.availableModels().size() << "\n";
            std::cerr << "Textures found: " << assetRegistry_.availableTextures().size() << "\n";
            std::cerr << "INI files found: " << assetRegistry_.availableIniFiles().size() << "\n";
          }
        } else {
          if (console_) {
            console_->error("Failed to scan asset registry: " + error);
          } else {
            std::cerr << "Failed to scan asset registry: " << error << "\n";
          }
        }
      } else {
        if (console_) {
          console_->error("Failed to initialize BIG archive manager: " + error);
        } else {
          std::cerr << "Failed to initialize BIG archive manager: " << error << "\n";
        }
      }
    } else {
      if (console_) {
        console_->warning("Game directory does not exist: " + gameDir.string());
      } else {
        std::cerr << "Game directory does not exist: " << gameDir.string() << "\n";
      }
    }
  }

  // Set up managers for texture and model loading
  textureManager_.setAssetRegistry(&assetRegistry_);
  textureManager_.setBigArchiveManager(&bigArchiveManager_);
  modelLoader_.setAssetRegistry(&assetRegistry_);
  modelLoader_.setBigArchiveManager(&bigArchiveManager_);
}

void Application::rescanAssetRegistry() {
  if (!bigArchiveManager_.isInitialized()) {
    console_->warning("Cannot rescan: BIG archive manager not initialized");
    return;
  }

  console_->info("Rescanning asset registry...");

  std::string error;
  if (assetRegistry_.scanArchives(bigArchiveManager_.gameDirectory(), &error)) {
    console_->info("Asset registry rescanned");
    console_->log("Models found: " + std::to_string(assetRegistry_.availableModels().size()));
    console_->log("Textures found: " + std::to_string(assetRegistry_.availableTextures().size()));
    console_->log("INI files found: " + std::to_string(assetRegistry_.availableIniFiles().size()));
  } else {
    console_->error("Failed to rescan asset registry: " + error);
  }
}

} // namespace w3d
