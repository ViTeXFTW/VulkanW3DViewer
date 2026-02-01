#include <cstdlib>
#include <iostream>

#include "core/application.hpp"

#include "core/application.hpp"
#include "w3d/loader.hpp"

void printUsage(const char *programName) {
  std::cout << "Usage: " << programName << " [options] [model.w3d]\n"
            << "\nOptions:\n"
            << "  -h, --help              Show this help message\n"
            << "  -t, --textures <path>   Set texture search path\n"
            << "  -d, --debug             Enable verbose debug output\n"
            << "\nExamples:\n"
            << "  " << programName << " model.w3d\n"
            << "  " << programName << " -t resources/textures model.w3d\n"
            << "  " << programName << " -d model.w3d\n";
}

int main(int argc, char *argv[]) {
  std::string modelPath;
  std::string texturePath;
  bool debugMode = false;

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
    } else if (arg[0] != '-') {
      modelPath = arg;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      printUsage(argv[0]);
      return EXIT_FAILURE;
    }
  }
  // Normal GUI mode
  w3d::Application app;

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
