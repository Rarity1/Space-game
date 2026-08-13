#pragma once
#include "CommonStructs.h"
#include "Exceptions.h"
#include <cstdint>
#include <vector>
#include <string>


struct vFaceData {
  uint32_t index;
  uint32_t normal;
  uint32_t texcoord;
};
struct boneweight {
  std::vector<uint32_t> bIndex{0};
  std::vector<float> weight{1.0};
};
struct Vertex {
  FLOAT3 normal;
  FLOAT3 position;
  FLOAT2 tc;
  uint16_t test = 0;
};
struct Node {
  float mass = 0;
  std::string name;
  int bIndex = -1;
  FLOAT4X4 matrix;
  FLOAT4 pos = {0, 0, 0, 0};
  FLOAT4X4 LocalTransform;
  std::vector<Node> children = {};
  int numchild = 0;
  std::vector<Node *> aChildren = {};
};
struct Bone {
  std::string name;
  uint32_t bIndex;
  // Inverse bind pose matrix
  FLOAT4X4 matrix;
  FLOAT4X4 finalTransform;
  // sIndex index. Not index of vertices.
  std::vector<uint32_t> Indices;
  Node *node;
  SphereCollider sphere;
  SphereCollider smallsphere;
};

class ModelData{
public:
  ModelData(){
    "Error" >> Exceptions::CheckerToken();
  };
	ModelData(std::string path);
  ~ModelData() = default;
	std::vector<Bone> bdata;
	Node ndata;
	std::vector<boneweight> weights;
	SphereCollider Sphere;
	std::vector<std::array<Vertex, 3>> MappedVertices;
  std::vector<uint32_t> IndexToVertex;
	std::vector<uint32_t> sIndex{};
	const std::string Path;
};