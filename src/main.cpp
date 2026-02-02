#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>

#include "core/application.hpp"
#include "w3d/loader.hpp"

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
