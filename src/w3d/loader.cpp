#include "loader.hpp"

#include <fstream>
#include <sstream>

#include "animation_parser.hpp"
#include "chunk_reader.hpp"
#include "hierarchy_parser.hpp"
#include "hlod_parser.hpp"
#include "mesh_parser.hpp"

namespace w3d {

std::optional<W3DFile> Loader::load(const std::filesystem::path& path,
                                    std::string* outError) {
  // Read file into memory
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    if (outError) {
      *outError = "Failed to open file: " + path.string();
    }
    return std::nullopt;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> buffer(size);
  if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
    if (outError) {
      *outError = "Failed to read file: " + path.string();
    }
    return std::nullopt;
  }

  return loadFromMemory(buffer.data(), buffer.size(), outError);
}

std::optional<W3DFile> Loader::loadFromMemory(const uint8_t* data, size_t size,
                                              std::string* outError) {
  try {
    ChunkReader reader(std::span<const uint8_t>(data, size));
    W3DFile w3dFile;

    while (!reader.atEnd()) {
      // Check if we have enough data for a chunk header
      if (reader.remaining() < 8) {
        break;
      }

      auto header = reader.readChunkHeader();
      uint32_t dataSize = header.dataSize();

      // Validate chunk size
      if (dataSize > reader.remaining()) {
        if (outError) {
          std::ostringstream oss;
          oss << "Chunk size (" << dataSize << ") exceeds remaining data ("
              << reader.remaining() << ")";
          *outError = oss.str();
        }
        return std::nullopt;
      }

      switch (header.type) {
        case ChunkType::MESH:
          w3dFile.meshes.push_back(MeshParser::parse(reader, dataSize));
          break;

        case ChunkType::HIERARCHY:
          w3dFile.hierarchies.push_back(HierarchyParser::parse(reader, dataSize));
          break;

        case ChunkType::ANIMATION:
          w3dFile.animations.push_back(AnimationParser::parse(reader, dataSize));
          break;

        case ChunkType::COMPRESSED_ANIMATION:
          w3dFile.compressedAnimations.push_back(
              AnimationParser::parseCompressed(reader, dataSize));
          break;

        case ChunkType::HLOD:
          w3dFile.hlods.push_back(HLodParser::parse(reader, dataSize));
          break;

        case ChunkType::BOX:
          w3dFile.boxes.push_back(HLodParser::parseBox(reader, dataSize));
          break;

        default:
          // Skip unknown top-level chunks
          reader.skip(dataSize);
          break;
      }
    }

    return w3dFile;

  } catch (const ParseError& e) {
    if (outError) {
      *outError = std::string("Parse error: ") + e.what();
    }
    return std::nullopt;
  } catch (const std::exception& e) {
    if (outError) {
      *outError = std::string("Error: ") + e.what();
    }
    return std::nullopt;
  }
}

std::string Loader::describe(const W3DFile& file) {
  std::ostringstream oss;

  oss << "W3D File Contents:\n";
  oss << "==================\n\n";

  // Meshes
  if (!file.meshes.empty()) {
    oss << "Meshes (" << file.meshes.size() << "):\n";
    for (const auto& mesh : file.meshes) {
      oss << "  - " << mesh.header.meshName;
      if (!mesh.header.containerName.empty()) {
        oss << " (container: " << mesh.header.containerName << ")";
      }
      oss << "\n";
      oss << "    Vertices: " << mesh.vertices.size();
      oss << ", Triangles: " << mesh.triangles.size();
      oss << ", Materials: " << mesh.vertexMaterials.size();
      oss << ", Textures: " << mesh.textures.size();
      oss << "\n";

      if (!mesh.textures.empty()) {
        oss << "    Texture names: ";
        for (size_t i = 0; i < mesh.textures.size(); ++i) {
          if (i > 0) oss << ", ";
          oss << mesh.textures[i].name;
        }
        oss << "\n";
      }

      // Skinning info
      if (!mesh.vertexInfluences.empty()) {
        oss << "    Skinned: yes (" << mesh.vertexInfluences.size()
            << " influences)\n";
      }

      // Bounding info
      oss << "    Bounds: [" << mesh.header.min.x << "," << mesh.header.min.y
          << "," << mesh.header.min.z << "] to [" << mesh.header.max.x << ","
          << mesh.header.max.y << "," << mesh.header.max.z << "]\n";
    }
    oss << "\n";
  }

  // Hierarchies
  if (!file.hierarchies.empty()) {
    oss << "Hierarchies (" << file.hierarchies.size() << "):\n";
    for (const auto& hier : file.hierarchies) {
      oss << "  - " << hier.name << " (" << hier.pivots.size() << " bones)\n";

      // List some bone names
      if (!hier.pivots.empty()) {
        oss << "    Bones: ";
        size_t count = std::min(hier.pivots.size(), size_t(5));
        for (size_t i = 0; i < count; ++i) {
          if (i > 0) oss << ", ";
          oss << hier.pivots[i].name;
        }
        if (hier.pivots.size() > 5) {
          oss << ", ... (" << (hier.pivots.size() - 5) << " more)";
        }
        oss << "\n";
      }
    }
    oss << "\n";
  }

  // Animations
  if (!file.animations.empty()) {
    oss << "Animations (" << file.animations.size() << "):\n";
    for (const auto& anim : file.animations) {
      oss << "  - " << anim.name << " (hierarchy: " << anim.hierarchyName
          << ")\n";
      oss << "    Frames: " << anim.numFrames << " @ " << anim.frameRate
          << " fps\n";
      oss << "    Channels: " << anim.channels.size()
          << ", Bit channels: " << anim.bitChannels.size() << "\n";
    }
    oss << "\n";
  }

  // Compressed animations
  if (!file.compressedAnimations.empty()) {
    oss << "Compressed Animations (" << file.compressedAnimations.size()
        << "):\n";
    for (const auto& anim : file.compressedAnimations) {
      oss << "  - " << anim.name << " (hierarchy: " << anim.hierarchyName
          << ")\n";
      oss << "    Frames: " << anim.numFrames << " @ " << anim.frameRate
          << " fps\n";
      oss << "    Channels: " << anim.channels.size()
          << ", Bit channels: " << anim.bitChannels.size() << "\n";
    }
    oss << "\n";
  }

  // HLods
  if (!file.hlods.empty()) {
    oss << "HLods (" << file.hlods.size() << "):\n";
    for (const auto& hlod : file.hlods) {
      oss << "  - " << hlod.name << " (hierarchy: " << hlod.hierarchyName
          << ")\n";
      oss << "    LOD levels: " << hlod.lodArrays.size() << "\n";

      for (size_t i = 0; i < hlod.lodArrays.size(); ++i) {
        const auto& lod = hlod.lodArrays[i];
        oss << "      LOD " << i << ": " << lod.subObjects.size()
            << " sub-objects (max screen size: " << lod.maxScreenSize << ")\n";
      }

      if (!hlod.aggregates.empty()) {
        oss << "    Aggregates: " << hlod.aggregates.size() << "\n";
      }
      if (!hlod.proxies.empty()) {
        oss << "    Proxies: " << hlod.proxies.size() << "\n";
      }
    }
    oss << "\n";
  }

  // Boxes
  if (!file.boxes.empty()) {
    oss << "Boxes (" << file.boxes.size() << "):\n";
    for (const auto& box : file.boxes) {
      oss << "  - " << box.name << "\n";
      oss << "    Center: [" << box.center.x << "," << box.center.y << ","
          << box.center.z << "]\n";
      oss << "    Extent: [" << box.extent.x << "," << box.extent.y << ","
          << box.extent.z << "]\n";
    }
    oss << "\n";
  }

  return oss.str();
}

}  // namespace w3d
