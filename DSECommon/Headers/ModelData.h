#pragma once

#include "CommonStructs.h"
#include "ModelData.h"

#include <rapidxml/rapidxml.hpp>
#include <array>
#include <vector>
#include <string>
#include <memory>



class ModelData{
public:
	enum PARSE{
		DEFAULT,
		MODEL,
		OTHER,
		WORLD,
		SAVE
	};
	struct vFaceData {
		uint32_t index;
		uint32_t normal;
		uint32_t texcoord;
	};
	struct boneweight {
		std::vector<uint32_t> bIndex{ 0 };
		std::vector<float> weight{ 1.0 };
	};
	struct Vertex
	{
		FLOAT3 normal;
		FLOAT3 position;
		FLOAT2 tc;
	};
	struct Node {
		float mass = 0;
		std::string name;
		int bIndex = -1;
		FLOAT4X4 matrix;
		FLOAT4 pos = {0, 0, 0,0};
		FLOAT4X4 LocalTransform;
		std::vector<Node> children = {};
		int numchild = 0;
		std::vector<Node*> aChildren = {};
	};

	struct Bone {
		std::string name;
		uint32_t bIndex;
		//Inverse bind pose matrix
		FLOAT4X4 matrix;
		FLOAT4X4 finalTransform;
		//sIndex index. Not index of vertices.
		std::vector<uint32_t> Indices;
		Node* node;
		SphereCollider sphere;
		SphereCollider smallsphere;
	};

	ModelData(std::string path, PARSE parse = DEFAULT);
	~ModelData();
	ModelData(const ModelData& old)
	{
		bdata = old.bdata;
		ndata = old.ndata;
		weights = old.weights;
		Sphere = old.Sphere;
		MappedVertices = old.MappedVertices;
		sIndex = old.sIndex;
		mIndex = old.mIndex;
	}

	//Use this to get normal map on stack

	std::vector<Bone> bdata;
	Node ndata;
	std::vector<boneweight> weights;
	SphereCollider Sphere;
	std::vector<std::array<Vertex, 3>> MappedVertices;
	std::vector<uint32_t> sIndex{};
	std::vector<uint32_t> mIndex{};
	std::string Path;

private:

	void ReadModel(std::unique_ptr<rapidxml::xml_document<char>> doc, std::unique_ptr<std::vector<char>> buffer);
	static float fDistance(FLOAT3& pos1, FLOAT3& pos2);
	static FLOAT4 fDirection(FLOAT3& pos1, FLOAT3& pos2);
	FLOAT4X4 strToMatrix(std::istringstream& rawmatri);
	Node ChildNodeRead(rapidxml::xml_node<char>* node);
	void GetAllChildBones(Node& node, std::vector<Node*>& Parent);

};