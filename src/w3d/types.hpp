#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "chunk_types.hpp"

namespace w3d {

// Basic vector types (matching W3D file format)
struct Vector3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct Vector2 {
  float u = 0.0f;
  float v = 0.0f;
};

struct Quaternion {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float w = 1.0f;
};

struct RGB {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
};

struct RGBA {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 255;
};

// Triangle structure
struct Triangle {
  uint32_t vertexIndices[3] = {0, 0, 0};
  uint32_t attributes = 0;
  Vector3 normal;
  float distance = 0.0f;
};

// Vertex influence for skinning
struct VertexInfluence {
  uint16_t boneIndex = 0;
  uint16_t boneIndex2 = 0;  // For multi-bone skinning
  float weight = 1.0f;
  float weight2 = 0.0f;
};

// Shader definition
struct ShaderDef {
  uint8_t depthCompare = Shader::DEPTHCOMPARE_PASS_LEQUAL;
  uint8_t depthMask = Shader::DEPTHMASK_WRITE_ENABLE;
  uint8_t colorMask = 0;
  uint8_t destBlend = Shader::DESTBLENDFUNC_ZERO;
  uint8_t fogFunc = 0;
  uint8_t priGradient = Shader::PRIGRADIENT_MODULATE;
  uint8_t secGradient = Shader::SECGRADIENT_DISABLE;
  uint8_t srcBlend = Shader::SRCBLENDFUNC_ONE;
  uint8_t texturing = Shader::TEXTURING_DISABLE;
  uint8_t detailColorFunc = Shader::DETAILCOLORFUNC_DISABLE;
  uint8_t detailAlphaFunc = Shader::DETAILALPHAFUNC_DISABLE;
  uint8_t shaderPreset = 0;
  uint8_t alphaTest = Shader::ALPHATEST_DISABLE;
  uint8_t postDetailColorFunc = 0;
  uint8_t postDetailAlphaFunc = 0;
  uint8_t padding = 0;
};

// Vertex material
struct VertexMaterial {
  std::string name;
  uint32_t attributes = 0;
  RGB ambient;
  RGB diffuse;
  RGB specular;
  RGB emissive;
  float shininess = 0.0f;
  float opacity = 1.0f;
  float translucency = 0.0f;
  std::string mapperArgs0;
  std::string mapperArgs1;
};

// Texture info
struct TextureInfo {
  uint16_t attributes = 0;
  uint16_t animType = 0;
  uint32_t frameCount = 0;
  float frameRate = 0.0f;
};

// Texture definition
struct TextureDef {
  std::string name;
  TextureInfo info;
};

// Texture stage (for multi-texturing)
struct TextureStage {
  std::vector<uint32_t> textureIds;
  std::vector<Vector2> texCoords;
  std::vector<uint32_t> perFaceTexCoordIds;
};

// Material pass
struct MaterialPass {
  std::vector<uint32_t> vertexMaterialIds;
  std::vector<uint32_t> shaderIds;
  std::vector<RGBA> dcg;  // Diffuse color per-vertex
  std::vector<RGBA> dig;  // Diffuse illumination per-vertex
  std::vector<RGBA> scg;  // Specular color per-vertex
  std::vector<TextureStage> textureStages;
};

// Material info
struct MaterialInfo {
  uint32_t passCount = 0;
  uint32_t vertexMaterialCount = 0;
  uint32_t shaderCount = 0;
  uint32_t textureCount = 0;
};

// AABTree node for collision
struct AABTreeNode {
  Vector3 min;
  Vector3 max;
  uint32_t frontOrPoly0 = 0;
  uint32_t backOrPolyCount = 0;
};

// AABTree for collision detection
struct AABTree {
  uint32_t nodeCount = 0;
  uint32_t polyCount = 0;
  std::vector<uint32_t> polyIndices;
  std::vector<AABTreeNode> nodes;
};

// Mesh header info
struct MeshHeader {
  uint32_t version = 0;
  uint32_t attributes = 0;
  std::string meshName;
  std::string containerName;
  uint32_t numTris = 0;
  uint32_t numVertices = 0;
  uint32_t numMaterials = 0;
  uint32_t numDamageStages = 0;
  int32_t sortLevel = 0;
  uint32_t prelitVersion = 0;
  uint32_t futureCounts = 0;
  uint32_t vertexChannels = 0;
  uint32_t faceChannels = 0;
  Vector3 min;
  Vector3 max;
  Vector3 sphCenter;
  float sphRadius = 0.0f;
};

// Complete mesh
struct Mesh {
  MeshHeader header;
  std::string userText;

  // Geometry data
  std::vector<Vector3> vertices;
  std::vector<Vector3> normals;
  std::vector<Vector2> texCoords;
  std::vector<Triangle> triangles;
  std::vector<RGBA> vertexColors;
  std::vector<uint32_t> shadeIndices;

  // Skinning
  std::vector<VertexInfluence> vertexInfluences;

  // Materials
  MaterialInfo materialInfo;
  std::vector<ShaderDef> shaders;
  std::vector<VertexMaterial> vertexMaterials;
  std::vector<TextureDef> textures;
  std::vector<MaterialPass> materialPasses;

  // Collision
  AABTree aabTree;
};

// Pivot (bone) structure
struct Pivot {
  std::string name;
  uint32_t parentIndex = 0xFFFFFFFF;  // -1 = root
  Vector3 translation;
  Vector3 eulerAngles;
  Quaternion rotation;
};

// Hierarchy (skeleton)
struct Hierarchy {
  uint32_t version = 0;
  std::string name;
  Vector3 center;
  std::vector<Pivot> pivots;
  std::vector<Vector3> pivotFixups;
};

// Animation channel data
struct AnimChannel {
  uint16_t firstFrame = 0;
  uint16_t lastFrame = 0;
  uint16_t vectorLen = 0;
  uint16_t flags = 0;
  uint16_t pivot = 0;
  std::vector<float> data;
};

// Bit channel (visibility)
struct BitChannel {
  uint16_t firstFrame = 0;
  uint16_t lastFrame = 0;
  uint16_t flags = 0;
  uint16_t pivot = 0;
  float defaultVal = 1.0f;
  std::vector<uint8_t> data;
};

// Animation
struct Animation {
  uint32_t version = 0;
  std::string name;
  std::string hierarchyName;
  uint32_t numFrames = 0;
  uint32_t frameRate = 0;
  std::vector<AnimChannel> channels;
  std::vector<BitChannel> bitChannels;
};

// Compressed animation channel
struct CompressedAnimChannel {
  uint32_t numTimeCodes = 0;
  uint16_t pivot = 0;
  uint16_t vectorLen = 0;
  uint16_t flags = 0;
  std::vector<uint16_t> timeCodes;
  std::vector<float> data;
};

// Compressed animation
struct CompressedAnimation {
  uint32_t version = 0;
  std::string name;
  std::string hierarchyName;
  uint32_t numFrames = 0;
  uint32_t frameRate = 0;
  uint16_t flavor = 0;
  std::vector<CompressedAnimChannel> channels;
  std::vector<BitChannel> bitChannels;
};

// HLod sub-object
struct HLodSubObject {
  uint32_t boneIndex = 0;
  std::string name;
};

// HLod LOD array
struct HLodArray {
  uint32_t modelCount = 0;
  float maxScreenSize = 0.0f;
  std::vector<HLodSubObject> subObjects;
};

// HLod (Hierarchical Level of Detail)
struct HLod {
  uint32_t version = 0;
  uint32_t lodCount = 0;
  std::string name;
  std::string hierarchyName;
  std::vector<HLodArray> lodArrays;
  std::vector<HLodSubObject> aggregates;
  std::vector<HLodSubObject> proxies;
};

// Box collision object
struct Box {
  uint32_t version = 0;
  uint32_t attributes = 0;
  std::string name;
  RGB color;
  Vector3 center;
  Vector3 extent;
};

// Complete W3D file contents
struct W3DFile {
  std::vector<Mesh> meshes;
  std::vector<Hierarchy> hierarchies;
  std::vector<Animation> animations;
  std::vector<CompressedAnimation> compressedAnimations;
  std::vector<HLod> hlods;
  std::vector<Box> boxes;
};

}  // namespace w3d
