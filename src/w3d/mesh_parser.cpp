#include "mesh_parser.hpp"

#include <iostream>

namespace w3d {

Mesh MeshParser::parse(ChunkReader &reader, uint32_t chunkSize) {
  Mesh mesh;
  size_t endPos = reader.position() + chunkSize;

  while (reader.position() < endPos) {
    auto header = reader.readChunkHeader();
    uint32_t dataSize = header.dataSize();
    size_t chunkEnd = reader.position() + dataSize;

    switch (header.type) {
    case ChunkType::MESH_HEADER3:
      mesh.header = parseMeshHeader(reader);
      break;

    case ChunkType::VERTICES: {
      size_t count = dataSize / (3 * sizeof(float));
      mesh.vertices.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        mesh.vertices.push_back(reader.readVector3());
      }
      break;
    }

    case ChunkType::VERTEX_NORMALS: {
      size_t count = dataSize / (3 * sizeof(float));
      mesh.normals.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        mesh.normals.push_back(reader.readVector3());
      }
      break;
    }

    case ChunkType::TEXCOORDS: {
      size_t count = dataSize / (2 * sizeof(float));
      mesh.texCoords.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        auto uv = reader.readVector2();
        uv.v = 1.0f - uv.v;  // Flip V to match legacy behavior (W3D -> OpenGL/Vulkan)
        mesh.texCoords.push_back(uv);
      }
      break;
    }

    case ChunkType::TRIANGLES: {
      // Each triangle is: 3 uint32 indices + 1 uint32 attributes +
      // 3 float normal + 1 float distance = 32 bytes
      size_t count = dataSize / 32;
      mesh.triangles.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        mesh.triangles.push_back(parseTriangle(reader));
      }
      break;
    }

    case ChunkType::VERTEX_COLORS: {
      size_t count = dataSize / sizeof(uint32_t);
      mesh.vertexColors.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        mesh.vertexColors.push_back(reader.readRGBA());
      }
      break;
    }

    case ChunkType::VERTEX_SHADE_INDICES: {
      size_t count = dataSize / sizeof(uint32_t);
      mesh.shadeIndices = reader.readArray<uint32_t>(count);
      break;
    }

    case ChunkType::VERTEX_INFLUENCES: {
      // Each influence is: 2 uint16 bone indices
      size_t count = dataSize / (2 * sizeof(uint16_t));
      mesh.vertexInfluences.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        VertexInfluence vi;
        vi.boneIndex = reader.read<uint16_t>();
        vi.boneIndex2 = reader.read<uint16_t>();
        mesh.vertexInfluences.push_back(vi);
      }
      break;
    }

    case ChunkType::MESH_USER_TEXT:
      mesh.userText = reader.readFixedString(dataSize);
      break;

    case ChunkType::MATERIAL_INFO: {
      mesh.materialInfo.passCount = reader.read<uint32_t>();
      mesh.materialInfo.vertexMaterialCount = reader.read<uint32_t>();
      mesh.materialInfo.shaderCount = reader.read<uint32_t>();
      mesh.materialInfo.textureCount = reader.read<uint32_t>();
      break;
    }

    case ChunkType::SHADERS: {
      size_t count = dataSize / 16; // W3dShaderStruct is 16 bytes
      mesh.shaders.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        mesh.shaders.push_back(parseShader(reader));
      }
      break;
    }

    case ChunkType::VERTEX_MATERIALS: {
      // Container chunk with multiple VERTEX_MATERIAL sub-chunks
      auto subReader = reader.subReader(dataSize);
      while (!subReader.atEnd()) {
        auto subHeader = subReader.readChunkHeader();
        if (subHeader.type == ChunkType::VERTEX_MATERIAL) {
          mesh.vertexMaterials.push_back(parseVertexMaterial(subReader, subHeader.dataSize()));
        } else {
          subReader.skip(subHeader.dataSize());
        }
      }
      break;
    }

    case ChunkType::TEXTURES: {
      // Container chunk with multiple TEXTURE sub-chunks
      auto subReader = reader.subReader(dataSize);
      while (!subReader.atEnd()) {
        auto subHeader = subReader.readChunkHeader();
        if (subHeader.type == ChunkType::TEXTURE) {
          mesh.textures.push_back(parseTexture(subReader, subHeader.dataSize()));
        } else {
          subReader.skip(subHeader.dataSize());
        }
      }
      break;
    }

    case ChunkType::MATERIAL_PASS:
      mesh.materialPasses.push_back(parseMaterialPass(reader, dataSize));
      break;

    case ChunkType::AABTREE:
      mesh.aabTree = parseAABTree(reader, dataSize);
      break;

    case ChunkType::PRELIT_UNLIT:
    case ChunkType::PRELIT_VERTEX:
    case ChunkType::PRELIT_LIGHTMAP_MULTI_PASS:
    case ChunkType::PRELIT_LIGHTMAP_MULTI_TEXTURE:
      // These contain material passes for prelit meshes
      // Skip for now, can be implemented later
      reader.skip(dataSize);
      break;

    default:
      // Skip unknown chunks
      reader.skip(dataSize);
      break;
    }

    // Ensure we're at the right position
    reader.seek(chunkEnd);
  }

  return mesh;
}

MeshHeader MeshParser::parseMeshHeader(ChunkReader &reader) {
  MeshHeader header;

  header.version = reader.read<uint32_t>();
  header.attributes = reader.read<uint32_t>();
  header.meshName = reader.readFixedString(W3D_NAME_LEN);
  header.containerName = reader.readFixedString(W3D_NAME_LEN);
  header.numTris = reader.read<uint32_t>();
  header.numVertices = reader.read<uint32_t>();
  header.numMaterials = reader.read<uint32_t>();
  header.numDamageStages = reader.read<uint32_t>();
  header.sortLevel = reader.read<int32_t>();
  header.prelitVersion = reader.read<uint32_t>();
  header.futureCounts = reader.read<uint32_t>();
  header.vertexChannels = reader.read<uint32_t>();
  header.faceChannels = reader.read<uint32_t>();

  // Bounding box
  header.min = reader.readVector3();
  header.max = reader.readVector3();

  // Bounding sphere
  header.sphCenter = reader.readVector3();
  header.sphRadius = reader.read<float>();

  return header;
}

Triangle MeshParser::parseTriangle(ChunkReader &reader) {
  Triangle tri;
  tri.vertexIndices[0] = reader.read<uint32_t>();
  tri.vertexIndices[1] = reader.read<uint32_t>();
  tri.vertexIndices[2] = reader.read<uint32_t>();
  tri.attributes = reader.read<uint32_t>();
  tri.normal = reader.readVector3();
  tri.distance = reader.read<float>();
  return tri;
}

ShaderDef MeshParser::parseShader(ChunkReader &reader) {
  ShaderDef shader;
  shader.depthCompare = reader.read<uint8_t>();
  shader.depthMask = reader.read<uint8_t>();
  shader.colorMask = reader.read<uint8_t>();
  shader.destBlend = reader.read<uint8_t>();
  shader.fogFunc = reader.read<uint8_t>();
  shader.priGradient = reader.read<uint8_t>();
  shader.secGradient = reader.read<uint8_t>();
  shader.srcBlend = reader.read<uint8_t>();
  shader.texturing = reader.read<uint8_t>();
  shader.detailColorFunc = reader.read<uint8_t>();
  shader.detailAlphaFunc = reader.read<uint8_t>();
  shader.shaderPreset = reader.read<uint8_t>();
  shader.alphaTest = reader.read<uint8_t>();
  shader.postDetailColorFunc = reader.read<uint8_t>();
  shader.postDetailAlphaFunc = reader.read<uint8_t>();
  shader.padding = reader.read<uint8_t>();
  return shader;
}

VertexMaterial MeshParser::parseVertexMaterial(ChunkReader &reader, uint32_t chunkSize) {
  VertexMaterial mat;
  size_t endPos = reader.position() + chunkSize;

  while (reader.position() < endPos) {
    auto header = reader.readChunkHeader();
    uint32_t dataSize = header.dataSize();

    switch (header.type) {
    case ChunkType::VERTEX_MATERIAL_NAME:
      mat.name = reader.readFixedString(dataSize);
      break;

    case ChunkType::VERTEX_MATERIAL_INFO: {
      mat.attributes = reader.read<uint32_t>();
      mat.ambient = reader.readRGB();
      mat.diffuse = reader.readRGB();
      mat.specular = reader.readRGB();
      mat.emissive = reader.readRGB();
      mat.shininess = reader.read<float>();
      mat.opacity = reader.read<float>();
      mat.translucency = reader.read<float>();
      break;
    }

    case ChunkType::VERTEX_MAPPER_ARGS0:
      mat.mapperArgs0 = reader.readFixedString(dataSize);
      break;

    case ChunkType::VERTEX_MAPPER_ARGS1:
      mat.mapperArgs1 = reader.readFixedString(dataSize);
      break;

    default:
      reader.skip(dataSize);
      break;
    }
  }

  return mat;
}

TextureDef MeshParser::parseTexture(ChunkReader &reader, uint32_t chunkSize) {
  TextureDef tex;
  size_t endPos = reader.position() + chunkSize;

  while (reader.position() < endPos) {
    auto header = reader.readChunkHeader();
    uint32_t dataSize = header.dataSize();

    switch (header.type) {
    case ChunkType::TEXTURE_NAME:
      tex.name = reader.readFixedString(dataSize);
      break;

    case ChunkType::TEXTURE_INFO: {
      tex.info.attributes = reader.read<uint16_t>();
      tex.info.animType = reader.read<uint16_t>();
      tex.info.frameCount = reader.read<uint32_t>();
      tex.info.frameRate = reader.read<float>();
      break;
    }

    default:
      reader.skip(dataSize);
      break;
    }
  }

  return tex;
}

TextureStage MeshParser::parseTextureStage(ChunkReader &reader, uint32_t chunkSize) {
  TextureStage stage;
  size_t endPos = reader.position() + chunkSize;

  while (reader.position() < endPos) {
    auto header = reader.readChunkHeader();
    uint32_t dataSize = header.dataSize();

    switch (header.type) {
    case ChunkType::TEXTURE_IDS: {
      size_t count = dataSize / sizeof(uint32_t);
      stage.textureIds = reader.readArray<uint32_t>(count);
      break;
    }

    case ChunkType::STAGE_TEXCOORDS: {
      size_t count = dataSize / (2 * sizeof(float));
      stage.texCoords.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        auto uv = reader.readVector2();
        uv.v = 1.0f - uv.v;  // Flip V to match legacy behavior (W3D -> OpenGL/Vulkan)
        stage.texCoords.push_back(uv);
      }
      break;
    }

    case ChunkType::PER_FACE_TEXCOORD_IDS: {
      size_t count = dataSize / sizeof(uint32_t);
      stage.perFaceTexCoordIds = reader.readArray<uint32_t>(count);
      break;
    }

    default:
      reader.skip(dataSize);
      break;
    }
  }

  return stage;
}

MaterialPass MeshParser::parseMaterialPass(ChunkReader &reader, uint32_t chunkSize) {
  MaterialPass pass;
  size_t endPos = reader.position() + chunkSize;

  while (reader.position() < endPos) {
    auto header = reader.readChunkHeader();
    uint32_t dataSize = header.dataSize();
    size_t chunkEnd = reader.position() + dataSize;

    switch (header.type) {
    case ChunkType::VERTEX_MATERIAL_IDS: {
      size_t count = dataSize / sizeof(uint32_t);
      pass.vertexMaterialIds = reader.readArray<uint32_t>(count);
      break;
    }

    case ChunkType::SHADER_IDS: {
      size_t count = dataSize / sizeof(uint32_t);
      pass.shaderIds = reader.readArray<uint32_t>(count);
      break;
    }

    case ChunkType::DCG: {
      size_t count = dataSize / sizeof(uint32_t);
      pass.dcg.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        pass.dcg.push_back(reader.readRGBA());
      }
      break;
    }

    case ChunkType::DIG: {
      size_t count = dataSize / sizeof(uint32_t);
      pass.dig.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        pass.dig.push_back(reader.readRGBA());
      }
      break;
    }

    case ChunkType::SCG: {
      size_t count = dataSize / sizeof(uint32_t);
      pass.scg.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        pass.scg.push_back(reader.readRGBA());
      }
      break;
    }

    case ChunkType::TEXTURE_STAGE:
      pass.textureStages.push_back(parseTextureStage(reader, dataSize));
      break;

    default:
      reader.skip(dataSize);
      break;
    }

    // Ensure we're at the right position for the next chunk
    reader.seek(chunkEnd);
  }

  return pass;
}

AABTree MeshParser::parseAABTree(ChunkReader &reader, uint32_t chunkSize) {
  AABTree tree;
  size_t endPos = reader.position() + chunkSize;

  while (reader.position() < endPos) {
    auto header = reader.readChunkHeader();
    uint32_t dataSize = header.dataSize();

    switch (header.type) {
    case ChunkType::AABTREE_HEADER: {
      tree.nodeCount = reader.read<uint32_t>();
      tree.polyCount = reader.read<uint32_t>();
      break;
    }

    case ChunkType::AABTREE_POLYINDICES: {
      size_t count = dataSize / sizeof(uint32_t);
      tree.polyIndices = reader.readArray<uint32_t>(count);
      break;
    }

    case ChunkType::AABTREE_NODES: {
      // Each node: 6 floats (min/max) + 2 uint32 = 32 bytes
      size_t count = dataSize / 32;
      tree.nodes.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        AABTreeNode node;
        node.min = reader.readVector3();
        node.max = reader.readVector3();
        node.frontOrPoly0 = reader.read<uint32_t>();
        node.backOrPolyCount = reader.read<uint32_t>();
        tree.nodes.push_back(node);
      }
      break;
    }

    default:
      reader.skip(dataSize);
      break;
    }
  }

  return tree;
}

} // namespace w3d
