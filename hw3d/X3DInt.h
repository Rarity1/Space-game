#pragma once
#include "CWin.h"

class ReadX3D{
public:
	ReadX3D(std::string path);
	~ReadX3D();
	struct boneweight {
		std::vector<int> bIndex;
		std::vector<float> weight;
	};
	struct Vertex
	{
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 tc;
	};
	//Dont use wayyyy to slow
	int FindIndex(int Index);
	//Use this to get normal map on stack
	struct vFaceData {
		WORD index;
		WORD normal;
		WORD texcoord;
	};

	struct Node {
		float mass = 0;
		std::string name;
		int bIndex = -1;
		DirectX::XMFLOAT4X4 matrix;
		DirectX::XMFLOAT4 pos{0,0,0,0};
		DirectX::XMFLOAT4X4 LocalTransform;
		std::vector<Node*> children = {};
		int numchild = 0;
		std::vector<Node*> aChildren = {};
	};

	struct Bone {
		std::string name;
		int bIndex;
		//Inverse bind pose matrix
		DirectX::XMFLOAT4X4 matrix;
		DirectX::XMFLOAT4X4 finalTransform;
		std::vector<int> Indices;
		Node* node;
		DirectX::BoundingSphere sphere;
		DirectX::BoundingSphere smallsphere;
	};
	std::vector<vFaceData> idata;
	std::vector<int> WeightCIndex;
	std::vector<Bone> bdata;
	Node ndata;
	std::vector<boneweight> weights;
	DirectX::BoundingSphere Sphere;
	std::vector<std::array<Vertex, 3>> MappedVertices;
	//std::vector<Vertex> iVerts;

private:
	static float fDistance(DirectX::XMFLOAT3& pos1, DirectX::XMFLOAT3& pos2);
	static DirectX::XMFLOAT4 fDirection(DirectX::XMFLOAT3& pos1, DirectX::XMFLOAT3& pos2);

	DirectX::XMFLOAT4X4 strToMatrix(std::istringstream& rawmatri);
	Node ChildNodeRead(rapidxml::xml_node<char>* node);
	void DeleteChild(Node* node);
	void GetAllChildBones(Node* node, std::vector<Node*>* Parent);
	std::ifstream file;
	rapidxml::xml_document<> doc;

};