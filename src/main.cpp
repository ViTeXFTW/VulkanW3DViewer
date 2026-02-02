#include "core/buffer.hpp"
#include "core/pipeline.hpp"
#include "core/vulkan_context.hpp"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>

#include "render/animation_player.hpp"
#include "render/bone_buffer.hpp"
#include "render/camera.hpp"
#include "render/hlod_model.hpp"
#include "render/hover_detector.hpp"
#include "render/material.hpp"
#include "render/renderable_mesh.hpp"
#include "render/skeleton.hpp"
#include "render/skeleton_renderer.hpp"
#include "render/texture.hpp"
#include "ui/console_window.hpp"
#include "ui/file_browser.hpp"
#include "ui/hover_tooltip.hpp"
#include "ui/imgui_backend.hpp"
#include "ui/ui_context.hpp"
#include "ui/ui_manager.hpp"
#include "ui/viewport_window.hpp"
#include "w3d/loader.hpp"

#include <imgui.h>

class VulkanW3DViewer {
public:
  void run() {
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

  void setTexturePath(const std::string &path) { customTexturePath_ = path; }
  void setDebugMode(bool debug) { debugMode_ = debug; }
  void setInitialModel(const std::string &path) { initialModelPath_ = path; }

private:
  // Command line options
  std::string customTexturePath_;
  std::string initialModelPath_;
  bool debugMode_ = false;
  GLFWwindow *window_ = nullptr;
  w3d::VulkanContext context_;
  w3d::Pipeline pipeline_;
  w3d::Pipeline skinnedPipeline_;
  w3d::DescriptorManager descriptorManager_;
  w3d::SkinnedDescriptorManager skinnedDescriptorManager_;
  w3d::UniformBuffer<w3d::UniformBufferObject> uniformBuffers_;
  w3d::BoneMatrixBuffer boneMatrixBuffer_;

  std::vector<vk::CommandBuffer> commandBuffers_;
  std::vector<vk::Semaphore> imageAvailableSemaphores_;
  std::vector<vk::Semaphore> renderFinishedSemaphores_;
  std::vector<vk::Fence> inFlightFences_;

  uint32_t currentFrame_ = 0;
  bool framebufferResized_ = false;

  // UI components
  w3d::ImGuiBackend imguiBackend_;
  w3d::UIManager uiManager_;
  w3d::ConsoleWindow *console_ = nullptr;   // Owned by uiManager_
  w3d::FileBrowser *fileBrowser_ = nullptr; // Owned by uiManager_

  // Loaded W3D data
  std::optional<w3d::W3DFile> loadedFile_;
  std::string loadedFilePath_;

  // Mesh rendering
  w3d::RenderableMesh renderableMesh_;
  w3d::HLodModel hlodModel_;
  w3d::Camera camera_;
  bool useHLodModel_ = false;        // True when an HLod is present
  bool useSkinnedRendering_ = false; // True when using GPU skinning

  // Texture and material system
  w3d::TextureManager textureManager_;
  w3d::Material defaultMaterial_;

  // Skeleton rendering
  w3d::SkeletonRenderer skeletonRenderer_;
  w3d::SkeletonPose skeletonPose_;
  bool showSkeleton_ = true;
  bool showMesh_ = true;

  // Animation playback
  w3d::AnimationPlayer animationPlayer_;
  float lastFrameTime_ = 0.0f;
  float lastAppliedFrame_ = -1.0f; // Track last frame applied to pose

  // Hover detection
  w3d::HoverDetector hoverDetector_;

  static constexpr uint32_t WIDTH = 1280;
  static constexpr uint32_t HEIGHT = 720;
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  static void framebufferResizeCallback(GLFWwindow *window, int /*width*/, int /*height*/) {
    auto *app = reinterpret_cast<VulkanW3DViewer *>(glfwGetWindowUserPointer(window));
    app->framebufferResized_ = true;
  }

  static void scrollCallback(GLFWwindow *window, double /*xoffset*/, double yoffset) {
    auto *app = reinterpret_cast<VulkanW3DViewer *>(glfwGetWindowUserPointer(window));
    app->camera_.onScroll(static_cast<float>(yoffset));
  }

  void initWindow() {
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

  void initVulkan() {
#ifdef W3D_DEBUG
    context_.init(window_, true); // Enable validation in debug builds
#else
    context_.init(window_, false); // Disable validation in release builds
#endif
    pipeline_.create(context_, "shaders/basic.vert.spv", "shaders/basic.frag.spv");
    skinnedPipeline_.createSkinned(context_, "shaders/skinned.vert.spv", "shaders/basic.frag.spv");
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
      std::cerr << "[DEBUG] Texture path set to: " << textureManager_.texturePath().string()
                << "\n";
      std::cerr << "[DEBUG] Path exists: " << std::filesystem::exists(texturePath) << "\n";
    }
#endif

    defaultMaterial_ = w3d::createDefaultMaterial();

    uniformBuffers_.create(context_, MAX_FRAMES_IN_FLIGHT);

    descriptorManager_.create(context_, pipeline_.descriptorSetLayout(), MAX_FRAMES_IN_FLIGHT);
    skinnedDescriptorManager_.create(context_, skinnedPipeline_.descriptorSetLayout(),
                                     MAX_FRAMES_IN_FLIGHT);

    // Get default texture for descriptor binding
    const auto &defaultTex = textureManager_.texture(0);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      descriptorManager_.updateUniformBuffer(i, uniformBuffers_.buffer(i),
                                             sizeof(w3d::UniformBufferObject));
      descriptorManager_.updateTexture(i, defaultTex.view, defaultTex.sampler);

      // Initialize skinned descriptor manager
      skinnedDescriptorManager_.updateUniformBuffer(i, uniformBuffers_.buffer(i),
                                                    sizeof(w3d::UniformBufferObject));
      skinnedDescriptorManager_.updateBoneBuffer(
          i, boneMatrixBuffer_.buffer(), sizeof(glm::mat4) * w3d::BoneMatrixBuffer::MAX_BONES);
    }

    createCommandBuffers();
    createSyncObjects();
  }

  void initUI() {
    imguiBackend_.init(window_, context_);

    // Register windows with UI manager
    auto *viewport = uiManager_.addWindow<w3d::ViewportWindow>();
    console_ = uiManager_.addWindow<w3d::ConsoleWindow>();
    fileBrowser_ = uiManager_.addWindow<w3d::FileBrowser>();
    uiManager_.addWindow<w3d::HoverTooltip>();

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

  void loadW3DFile(const std::filesystem::path &path) {
    console_->info("Loading: " + path.string());

    std::string error;
    auto file = w3d::Loader::load(path, &error);

    if (!file) {
      console_->error("Failed to load: " + error);
      return;
    }

    loadedFile_ = std::move(file);
    loadedFilePath_ = path.string();

    console_->info("Successfully loaded: " + path.filename().string());

    // Output the description to console
    std::string description = w3d::Loader::describe(*loadedFile_);

    // Split description into lines and add to console
    std::istringstream stream(description);
    std::string line;
    while (std::getline(stream, line)) {
      console_->addMessage(line);
    }

    // Compute skeleton pose first (needed for mesh positioning)
    context_.device().waitIdle();
    if (!loadedFile_->hierarchies.empty()) {
      skeletonPose_.computeRestPose(loadedFile_->hierarchies[0]);
      skeletonRenderer_.updateFromPose(context_, skeletonPose_);

      // Initialize bone matrix buffer with rest pose transforms
      // This ensures models without animations still have correct bone positioning
      if (skeletonPose_.isValid()) {
        auto skinningMatrices = skeletonPose_.getSkinningMatrices();
        boneMatrixBuffer_.update(skinningMatrices);
      }

      console_->info("Loaded skeleton with " + std::to_string(skeletonPose_.boneCount()) +
                     " bones");
    }

    // Load animations if present
    animationPlayer_.clear();
    if (!loadedFile_->animations.empty() || !loadedFile_->compressedAnimations.empty()) {
      animationPlayer_.load(*loadedFile_);
      console_->info("Loaded " + std::to_string(animationPlayer_.animationCount()) +
                     " animation(s)");
    }

    const w3d::SkeletonPose *posePtr = skeletonPose_.isValid() ? &skeletonPose_ : nullptr;

    // Load textures referenced by meshes
    size_t texturesLoaded = 0;
    size_t texturesMissing = 0;
    std::set<std::string> uniqueTextures;

    for (const auto &mesh : loadedFile_->meshes) {
      for (const auto &tex : mesh.textures) {
        // Skip if we already processed this texture
        if (uniqueTextures.count(tex.name) > 0) {
          continue;
        }
        uniqueTextures.insert(tex.name);

#ifdef W3D_DEBUG
        if (debugMode_) {
          std::cerr << "[DEBUG] Loading texture: " << tex.name << "\n";
        }
#endif

        uint32_t texIdx = textureManager_.loadTexture(tex.name);
        if (texIdx > 0) {
          texturesLoaded++;
#ifdef W3D_DEBUG
          if (debugMode_) {
            std::cerr << "[DEBUG]   -> Loaded as index " << texIdx << "\n";
          }
#endif
        } else {
          texturesMissing++;
#ifdef W3D_DEBUG
          if (debugMode_) {
            std::cerr << "[DEBUG]   -> NOT FOUND\n";
          }
#endif
        }
      }
    }

    console_->info("Textures: " + std::to_string(texturesLoaded) + " loaded, " +
                   std::to_string(texturesMissing) + " missing");

#ifdef W3D_DEBUG
    if (debugMode_) {
      std::cerr << "[DEBUG] Total textures in manager: " << textureManager_.textureCount() << "\n";
    }
#endif

    // Check if file has HLod data - use HLodModel for proper LOD support
    if (!loadedFile_->hlods.empty()) {
      useHLodModel_ = true;
      renderableMesh_.destroy(); // Clean up old mesh data

      // Use skinned rendering if we have a hierarchy (for animation support)
      // This handles both per-vertex skinned meshes AND rigidly attached meshes
      if (!loadedFile_->hierarchies.empty()) {
        useSkinnedRendering_ = true;
        hlodModel_.loadSkinned(context_, *loadedFile_);
        console_->info("Using GPU skinned rendering");
      } else {
        useSkinnedRendering_ = false;
        hlodModel_.load(context_, *loadedFile_, nullptr);
        console_->info("Using static rendering (no skeleton)");
      }

      const auto &hlod = loadedFile_->hlods[0];
      console_->info("Loaded HLod: " + hlod.name);
      console_->info("  LOD levels: " + std::to_string(hlodModel_.lodCount()));
      console_->info("  Aggregates: " + std::to_string(hlodModel_.aggregateCount()));
      console_->info("  Total GPU meshes: " + std::to_string(hlodModel_.totalMeshCount()));
      if (useSkinnedRendering_) {
        console_->info("  Skinned meshes: " + std::to_string(hlodModel_.skinnedMeshCount()));
      }

      // Log LOD level details
      for (size_t i = 0; i < hlodModel_.lodCount(); ++i) {
        const auto &level = hlodModel_.lodLevel(i);
        std::string lodInfo =
            "  LOD " + std::to_string(i) + ": " + std::to_string(level.meshes.size()) +
            " meshes, maxScreenSize=" + std::to_string(static_cast<int>(level.maxScreenSize));
        console_->log(lodInfo);
      }

      if (hlodModel_.hasData()) {
        const auto &bounds = hlodModel_.bounds();
        camera_.setTarget(bounds.center(), bounds.radius() * 2.5f);
      }
    } else {
      // No HLod - use simple mesh rendering
      useHLodModel_ = false;
      hlodModel_.destroy(); // Clean up old HLod data

      renderableMesh_.loadWithPose(context_, *loadedFile_, posePtr);

      if (renderableMesh_.hasData()) {
        const auto &bounds = renderableMesh_.bounds();
        camera_.setTarget(bounds.center(), bounds.radius() * 2.5f);
        console_->info("Uploaded " + std::to_string(renderableMesh_.meshCount()) +
                       " meshes to GPU (no HLod)");
      }
    }

    // Center on skeleton if no mesh data
    bool hasMeshData =
        (useHLodModel_ && hlodModel_.hasData()) || (!useHLodModel_ && renderableMesh_.hasData());
    if (!hasMeshData && skeletonPose_.isValid()) {
      glm::vec3 center(0.0f);
      float maxDist = 1.0f;
      for (size_t i = 0; i < skeletonPose_.boneCount(); ++i) {
        glm::vec3 pos = skeletonPose_.bonePosition(i);
        center += pos;
        maxDist = std::max(maxDist, glm::length(pos));
      }
      center /= static_cast<float>(skeletonPose_.boneCount());
      camera_.setTarget(center, maxDist * 2.5f);
    }
  }

  void createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo{context_.commandPool(),
                                            vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT};

    commandBuffers_ = context_.device().allocateCommandBuffers(allocInfo);
  }

  void createSyncObjects() {
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semaphoreInfo{};
    vk::FenceCreateInfo fenceInfo{vk::FenceCreateFlagBits::eSignaled};

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      imageAvailableSemaphores_[i] = context_.device().createSemaphore(semaphoreInfo);
      renderFinishedSemaphores_[i] = context_.device().createSemaphore(semaphoreInfo);
      inFlightFences_[i] = context_.device().createFence(fenceInfo);
    }
  }

  void updateUniformBuffer(uint32_t frameIndex) {
    w3d::UniformBufferObject ubo{};

    // Always use camera-based view
    ubo.model = glm::mat4(1.0f);
    ubo.view = camera_.viewMatrix();

    auto extent = context_.swapchainExtent();
    ubo.proj = glm::perspective(
        glm::radians(45.0f), static_cast<float>(extent.width) / static_cast<float>(extent.height),
        0.01f, 10000.0f);
    ubo.proj[1][1] *= -1; // Flip Y for Vulkan

    uniformBuffers_.update(frameIndex, ubo);
  }

  void drawUI() {
    // Build UI context with current application state
    w3d::UIContext ctx;
    ctx.window = window_;
    ctx.loadedFile = loadedFile_ ? &*loadedFile_ : nullptr;
    ctx.loadedFilePath = loadedFilePath_;
    ctx.hlodModel = &hlodModel_;
    ctx.renderableMesh = &renderableMesh_;
    ctx.useHLodModel = useHLodModel_;
    ctx.useSkinnedRendering = useSkinnedRendering_;
    ctx.camera = &camera_;
    ctx.skeletonPose = &skeletonPose_;
    ctx.animationPlayer = &animationPlayer_;
    ctx.showMesh = &showMesh_;
    ctx.showSkeleton = &showSkeleton_;
    ctx.hoverState = &hoverDetector_.state();

    // Set up callbacks
    ctx.onOpenFile = [this]() { fileBrowser_->setVisible(true); };
    ctx.onExit = [this]() { glfwSetWindowShouldClose(window_, GLFW_TRUE); };
    ctx.onResetCamera = [this]() {
      if (useHLodModel_ && hlodModel_.hasData()) {
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

  void updateHover() {
    // Reset hover state by default
    hoverDetector_.state().reset();

    // Skip if ImGui wants mouse (over UI elements)
    if (ImGui::GetIO().WantCaptureMouse) {
      return;
    }

    // We need to check if mouse is over the viewport window and get its coordinates
    // This must be done during ImGui frame, but we're outside of ImGui rendering here
    // So we'll store viewport info during drawViewportWindow() and use it here

    // For now, use a simpler approach: check if viewport is hovered during next frame
    // We'll need to refactor to call this from within the viewport window context

    // Get mouse position in window coordinates
    double mouseX, mouseY;
    glfwGetCursorPos(window_, &mouseX, &mouseY);

    // Get swapchain (full render target) dimensions
    auto extent = context_.swapchainExtent();

    // Get camera matrices (must match rendering)
    auto view = camera_.viewMatrix();
    auto proj = glm::perspective(
        glm::radians(45.0f), static_cast<float>(extent.width) / static_cast<float>(extent.height),
        0.01f, 10000.0f);
    proj[1][1] *= -1; // Vulkan Y-flip

    // Update hover detector with ray using full window coordinates
    // Note: This assumes the viewport fills the entire window
    hoverDetector_.update(
        glm::vec2(static_cast<float>(mouseX), static_cast<float>(mouseY)),
        glm::vec2(static_cast<float>(extent.width), static_cast<float>(extent.height)), view, proj);

    // Test skeleton first (priority over meshes)
    if (showSkeleton_ && skeletonRenderer_.hasData()) {
      hoverDetector_.testSkeleton(skeletonRenderer_, 0.05f);
    }

    // Test meshes
    if (showMesh_) {
      if (useHLodModel_ && hlodModel_.hasData()) {
        // TODO: Implement HLod hover detection
        // For now, skip HLod models
      } else if (renderableMesh_.hasData()) {
        hoverDetector_.testMeshes(renderableMesh_);
      }
    }
  }

  void recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex) {
    vk::CommandBufferBeginInfo beginInfo{};
    cmd.begin(beginInfo);

    auto extent = context_.swapchainExtent();

    // Clear values for color and depth attachments
    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].color = vk::ClearColorValue{
        std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f}
    };
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

    // Begin render pass
    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.renderPass = context_.renderPass();
    renderPassInfo.framebuffer = context_.framebuffer(imageIndex);
    renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    // Draw 3D content
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_.pipeline());

    vk::Viewport viewport{
        0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height),
        0.0f, 1.0f};
    cmd.setViewport(0, viewport);

    vk::Rect2D scissor{
        {0, 0},
        extent
    };
    cmd.setScissor(0, scissor);

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_.layout(), 0,
                           descriptorManager_.descriptorSet(currentFrame_), {});

    // Draw loaded mesh (either HLod model or simple renderable mesh)
    if (showMesh_) {
      if (useHLodModel_ && hlodModel_.hasData()) {
        if (useSkinnedRendering_ && hlodModel_.hasSkinning()) {
          // Draw with skinned pipeline (GPU skinning)
          cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, skinnedPipeline_.pipeline());

          hlodModel_.drawSkinnedWithTextures(cmd, [&](const std::string &textureName) {
            w3d::MaterialPushConstant materialData{};
            materialData.diffuseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            materialData.emissiveColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            materialData.specularColor = glm::vec4(0.2f, 0.2f, 0.2f, 32.0f);
            materialData.hoverTint = glm::vec3(1.0f); // No tint for HLod (not yet implemented)
            materialData.flags = 0;
            materialData.alphaThreshold = 0.5f;

            // Look up texture by name
            uint32_t texIdx = 0;
            if (!textureName.empty()) {
              texIdx = textureManager_.findTexture(textureName);
            }

            if (texIdx > 0) {
              const auto &tex = textureManager_.texture(texIdx);
              vk::DescriptorSet texDescSet = skinnedDescriptorManager_.getDescriptorSet(
                  currentFrame_, texIdx, tex.view, tex.sampler, boneMatrixBuffer_.buffer(),
                  sizeof(glm::mat4) * w3d::BoneMatrixBuffer::MAX_BONES);
              cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skinnedPipeline_.layout(), 0,
                                     texDescSet, {});
              materialData.useTexture = 1;
            } else {
              const auto &defaultTex = textureManager_.texture(0);
              vk::DescriptorSet defaultDescSet = skinnedDescriptorManager_.getDescriptorSet(
                  currentFrame_, 0, defaultTex.view, defaultTex.sampler, boneMatrixBuffer_.buffer(),
                  sizeof(glm::mat4) * w3d::BoneMatrixBuffer::MAX_BONES);
              cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skinnedPipeline_.layout(), 0,
                                     defaultDescSet, {});
              materialData.useTexture = 0;
            }

            cmd.pushConstants(skinnedPipeline_.layout(), vk::ShaderStageFlagBits::eFragment, 0,
                              sizeof(w3d::MaterialPushConstant), &materialData);
          });

          // Switch back to regular pipeline for skeleton overlay
          cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_.pipeline());
        } else {
          // Draw with regular pipeline (CPU-transformed vertices)
          hlodModel_.drawWithTextures(cmd, [&](const std::string &textureName) {
            w3d::MaterialPushConstant materialData{};
            materialData.diffuseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            materialData.emissiveColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            materialData.specularColor = glm::vec4(0.2f, 0.2f, 0.2f, 32.0f);
            materialData.hoverTint = glm::vec3(1.0f); // No tint for HLod (not yet implemented)
            materialData.flags = 0;
            materialData.alphaThreshold = 0.5f;

            // Look up texture by name
            uint32_t texIdx = 0;
            if (!textureName.empty()) {
              texIdx = textureManager_.findTexture(textureName);
            }

            if (texIdx > 0) {
              // Get pre-allocated descriptor set for this texture
              const auto &tex = textureManager_.texture(texIdx);
              vk::DescriptorSet texDescSet = descriptorManager_.getTextureDescriptorSet(
                  currentFrame_, texIdx, tex.view, tex.sampler);
              cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_.layout(), 0,
                                     texDescSet, {});
              materialData.useTexture = 1;
            } else {
              // Use default texture descriptor set
              const auto &defaultTex = textureManager_.texture(0);
              vk::DescriptorSet defaultDescSet = descriptorManager_.getTextureDescriptorSet(
                  currentFrame_, 0, defaultTex.view, defaultTex.sampler);
              cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_.layout(), 0,
                                     defaultDescSet, {});
              materialData.useTexture = 0;
            }

            cmd.pushConstants(pipeline_.layout(), vk::ShaderStageFlagBits::eFragment, 0,
                              sizeof(w3d::MaterialPushConstant), &materialData);
          });
        }
      } else if (renderableMesh_.hasData()) {
        // Simple mesh without textures
        w3d::MaterialPushConstant materialData{};
        materialData.diffuseColor = glm::vec4(defaultMaterial_.diffuse, defaultMaterial_.opacity);
        materialData.emissiveColor = glm::vec4(defaultMaterial_.emissive, 1.0f);
        materialData.specularColor =
            glm::vec4(defaultMaterial_.specular, defaultMaterial_.shininess);
        materialData.flags = 0;
        materialData.alphaThreshold = 0.5f;
        materialData.useTexture = 0;

        // Use hover detection for simple meshes
        const glm::vec3 hoverTint(1.5f, 1.5f, 1.3f); // Warm highlight
        const auto &hover = hoverDetector_.state();

        renderableMesh_.drawWithHover(
            cmd, hover.type == w3d::HoverType::Mesh ? static_cast<int>(hover.objectIndex) : -1,
            hoverTint, [&](size_t /*meshIndex*/, const glm::vec3 &tint) {
              materialData.hoverTint = tint;
              cmd.pushConstants(pipeline_.layout(), vk::ShaderStageFlagBits::eFragment, 0,
                                sizeof(w3d::MaterialPushConstant), &materialData);
            });
      }
    }

    // Draw skeleton overlay
    if (showSkeleton_ && skeletonRenderer_.hasData()) {
      // Skeleton uses same descriptor set layout, so we can reuse the bound descriptor
      cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skeletonRenderer_.pipelineLayout(),
                             0, descriptorManager_.descriptorSet(currentFrame_), {});

      // Apply hover tint if hovering over skeleton
      const glm::vec3 hoverTint(1.5f, 1.5f, 1.3f); // Warm highlight
      const auto &hover = hoverDetector_.state();
      glm::vec3 skeletonTint =
          (hover.type == w3d::HoverType::Bone || hover.type == w3d::HoverType::Joint)
              ? hoverTint
              : glm::vec3(1.0f);

      skeletonRenderer_.drawWithHover(cmd, skeletonTint);
    }

    // Draw ImGui
    imguiBackend_.render(cmd);

    cmd.endRenderPass();
    cmd.end();
  }

  void drawFrame() {
    auto device = context_.device();

    // Wait for previous frame
    auto waitResult = device.waitForFences(inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);
    if (waitResult != vk::Result::eSuccess) {
      throw std::runtime_error("Failed waiting for fence");
    }

    // Acquire next image
    uint32_t imageIndex;
    auto acquireResult = device.acquireNextImageKHR(
        context_.swapchain(), UINT64_MAX, imageAvailableSemaphores_[currentFrame_], nullptr);

    if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
      recreateSwapchain();
      return;
    } else if (acquireResult.result != vk::Result::eSuccess &&
               acquireResult.result != vk::Result::eSuboptimalKHR) {
      throw std::runtime_error("Failed to acquire swap chain image");
    }
    imageIndex = acquireResult.value;

    device.resetFences(inFlightFences_[currentFrame_]);

    // Update uniform buffer
    updateUniformBuffer(currentFrame_);

    // Start ImGui frame
    imguiBackend_.newFrame();
    drawUI();

    // Record command buffer
    commandBuffers_[currentFrame_].reset();
    recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex);

    // Submit
    vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphores_[currentFrame_];
    submitInfo.pWaitDstStageMask = &waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[currentFrame_];

    context_.graphicsQueue().submit(submitInfo, inFlightFences_[currentFrame_]);

    // Present
    vk::SwapchainKHR swapchain = context_.swapchain();
    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores_[currentFrame_];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;

    auto presentResult = context_.presentQueue().presentKHR(presentInfo);

    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR || framebufferResized_) {
      framebufferResized_ = false;
      recreateSwapchain();
    } else if (presentResult != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to present swap chain image");
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void recreateSwapchain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window_, &width, &height);
      glfwWaitEvents();
    }

    context_.device().waitIdle();
    context_.recreateSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    imguiBackend_.onSwapchainRecreate();
  }

  void mainLoop() {
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
      if (loadedFile_ && animationPlayer_.animationCount() > 0 &&
          !loadedFile_->hierarchies.empty()) {
        float currentFrame = animationPlayer_.currentFrame();
        if (currentFrame != lastAppliedFrame_ || !animationPlayer_.isPlaying()) {
          animationPlayer_.applyToPose(skeletonPose_, loadedFile_->hierarchies[0]);
          // Wait for GPU to finish before updating skeleton buffers
          // This prevents device lost errors from buffer recreation during rendering
          context_.device().waitIdle();
          skeletonRenderer_.updateFromPose(context_, skeletonPose_);

          // Update bone matrix buffer for GPU skinning
          if (useSkinnedRendering_ && skeletonPose_.isValid()) {
            auto skinningMatrices = skeletonPose_.getSkinningMatrices();
            boneMatrixBuffer_.update(skinningMatrices);
          }

          lastAppliedFrame_ = currentFrame;
        }
      }

      // Update LOD selection based on camera distance
      if (useHLodModel_ && hlodModel_.hasData()) {
        auto extent = context_.swapchainExtent();
        float screenHeight = static_cast<float>(extent.height);
        float fovY = glm::radians(45.0f); // Must match projection FOV
        float cameraDistance = camera_.distance();

        hlodModel_.updateLOD(screenHeight, fovY, cameraDistance);
      }

      drawFrame();
    }

    context_.device().waitIdle();
  }

  void cleanup() {
    auto device = context_.device();

    imguiBackend_.cleanup();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      device.destroySemaphore(imageAvailableSemaphores_[i]);
      device.destroySemaphore(renderFinishedSemaphores_[i]);
      device.destroyFence(inFlightFences_[i]);
    }

    skeletonRenderer_.destroy();
    hlodModel_.destroy();
    renderableMesh_.destroy();
    textureManager_.destroy();
    boneMatrixBuffer_.destroy();
    skinnedDescriptorManager_.destroy();
    descriptorManager_.destroy();
    uniformBuffers_.destroy();
    skinnedPipeline_.destroy();
    pipeline_.destroy();
    context_.cleanup();

    if (window_) {
      glfwDestroyWindow(window_);
    }
    glfwTerminate();
  }
};

void printUsage(const char *programName) {
  std::cout << "Usage: " << programName << " [options] [model.w3d]\n"
            << "\nOptions:\n"
            << "  -h, --help              Show this help message\n"
            << "  -t, --textures <path>   Set texture search path\n"
            << "  -d, --debug             Enable verbose debug output\n"
            << "  -l, --list-textures     List all textures referenced by the model\n"
            << "\nExamples:\n"
            << "  " << programName << " model.w3d\n"
            << "  " << programName << " -t resources/textures model.w3d\n"
            << "  " << programName << " -d -l model.w3d\n";
}

int main(int argc, char *argv[]) {
  std::string modelPath;
  std::string texturePath;
  bool debugMode = false;
  bool listTextures = false;

  // Parse command line arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      printUsage(argv[0]);
      return EXIT_SUCCESS;
    } else if (arg == "-t" || arg == "--textures") {
      if (i + 1 < argc) {
        texturePath = argv[++i];
      } else {
        std::cerr << "Error: -t requires a path argument\n";
        return EXIT_FAILURE;
      }
    } else if (arg == "-d" || arg == "--debug") {
      debugMode = true;
    } else if (arg == "-l" || arg == "--list-textures") {
      listTextures = true;
    } else if (arg[0] != '-') {
      modelPath = arg;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      printUsage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  // If list-textures mode with a model, just analyze and exit
  if (listTextures && !modelPath.empty()) {
    std::cout << "Analyzing W3D file: " << modelPath << "\n\n";

    std::string error;
    auto file = w3d::Loader::load(modelPath, &error);
    if (!file) {
      std::cerr << "Failed to load: " << error << "\n";
      return EXIT_FAILURE;
    }

    std::cout << "=== Textures referenced in model ===\n";
    std::set<std::string> uniqueTextures;
    for (const auto &mesh : file->meshes) {
      for (const auto &tex : mesh.textures) {
        uniqueTextures.insert(tex.name);
      }
    }

    if (uniqueTextures.empty()) {
      std::cout << "(No textures referenced)\n";
    } else {
      for (const auto &name : uniqueTextures) {
        std::cout << "  " << name << "\n";
      }
    }

    // Check texture path resolution
    std::filesystem::path searchPath = texturePath.empty() ? "resources/textures" : texturePath;
    std::cout << "\n=== Texture path resolution (searching in: " << searchPath.string()
              << ") ===\n";

    if (!std::filesystem::exists(searchPath)) {
      std::cout << "WARNING: Texture directory does not exist!\n";
    } else {
      std::cout << "Files in texture directory:\n";
      for (const auto &entry : std::filesystem::directory_iterator(searchPath)) {
        std::cout << "  " << entry.path().filename().string() << "\n";
      }

      std::cout << "\nTexture resolution results:\n";
      for (const auto &name : uniqueTextures) {
        // Try to resolve the texture
        std::string baseName = name;
        // Remove extension
        size_t dotPos = baseName.find_last_of('.');
        if (dotPos != std::string::npos) {
          baseName = baseName.substr(0, dotPos);
        }
        // Convert to lowercase
        std::transform(baseName.begin(), baseName.end(), baseName.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        bool found = false;
        std::string foundPath;
        for (const auto &ext : {".dds", ".tga", ".DDS", ".TGA"}) {
          auto path = searchPath / (baseName + ext);
          if (std::filesystem::exists(path)) {
            found = true;
            foundPath = path.string();
            break;
          }
        }

        if (found) {
          std::cout << "  [OK] " << name << " -> " << foundPath << "\n";
        } else {
          std::cout << "  [MISSING] " << name << "\n";
        }
      }
    }

    return EXIT_SUCCESS;
  }

  // Normal GUI mode
  VulkanW3DViewer app;

  // Pass configuration to app
  if (!texturePath.empty()) {
    app.setTexturePath(texturePath);
  }
  if (debugMode) {
    app.setDebugMode(true);
  }
  if (!modelPath.empty()) {
    app.setInitialModel(modelPath);
  }

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
