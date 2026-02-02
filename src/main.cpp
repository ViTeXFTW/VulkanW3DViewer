#include <cstdlib>
#include <iostream>

#include "core/application.hpp"

#include <CLI/CLI.hpp>

int main(int argc, char *argv[]) {
  CLI::App app{
      "W3D Viewer - A Vulkan-based viewer for W3D 3D model files from Command & Conquer Generals"};

  std::string modelPath;
  std::string texturePath;
  bool debugMode = false;

  // Define command line options
  app.add_option("model", modelPath, "W3D model file to load on startup")->check(CLI::ExistingFile);
  app.add_option("-t,--textures", texturePath, "Set custom texture search path")
      ->check(CLI::ExistingDirectory);
  app.add_flag("-d,--debug", debugMode, "Enable verbose debug output");

  // Parse command line arguments
  CLI11_PARSE(app, argc, argv);

  // Create and configure application
  w3d::Application viewer;

  // Pass configuration to application
  if (!texturePath.empty()) {
    viewer.setTexturePath(texturePath);
  }
  if (debugMode) {
    viewer.setDebugMode(true);
  }
  if (!modelPath.empty()) {
    viewer.setInitialModel(modelPath);
  }

  try {
    viewer.run();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
