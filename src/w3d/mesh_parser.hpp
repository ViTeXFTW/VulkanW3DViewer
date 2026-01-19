#pragma once

#include "chunk_reader.hpp"
#include "types.hpp"

namespace w3d {

class MeshParser {
 public:
  // Parse a mesh from a chunk reader positioned at W3D_CHUNK_MESH data
  static Mesh parse(ChunkReader& reader, uint32_t chunkSize);

 private:
  static MeshHeader parseMeshHeader(ChunkReader& reader);
  static Triangle parseTriangle(ChunkReader& reader);
  static ShaderDef parseShader(ChunkReader& reader);
  static VertexMaterial parseVertexMaterial(ChunkReader& reader,
                                            uint32_t chunkSize);
  static TextureDef parseTexture(ChunkReader& reader, uint32_t chunkSize);
  static MaterialPass parseMaterialPass(ChunkReader& reader,
                                        uint32_t chunkSize);
  static TextureStage parseTextureStage(ChunkReader& reader,
                                        uint32_t chunkSize);
  static AABTree parseAABTree(ChunkReader& reader, uint32_t chunkSize);
};

}  // namespace w3d
