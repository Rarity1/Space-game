#pragma once
#include "CWin.h"

class ReadX3D{
public:
	ReadX3D(std::string path);
	~ReadX3D();
	void cvertexData();
	struct boneweight {
		std::vector<int> bIndex;
		std::vector<float> weight;
	};
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 tc;
		DirectX::XMFLOAT3 normal;
	};
	std::vector<ReadX3D::Vertex*>& FindTri(int& Index);
	int FindIndex(int& Index);

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
	std::vector<Vertex> Vertdata;
	std::vector<vFaceData> idata;
	std::vector<int> WeightCIndex;
	std::vector<Bone> bdata;
	Node ndata;
	std::vector<boneweight> weights;
	DirectX::BoundingSphere Sphere;
	//Size fsize;

private:
	float fDistance(DirectX::XMFLOAT3* pos1, DirectX::XMFLOAT3* pos2);
	DirectX::XMFLOAT4 fDirection(DirectX::XMFLOAT3* pos1, DirectX::XMFLOAT3* pos2);
	std::vector<std::vector<Vertex*>> TriData;
	std::vector<int> Map;
	DirectX::XMFLOAT4X4 strToMatrix(std::istringstream& rawmatri);
	Node ChildNodeRead(rapidxml::xml_node<char>* node);
	void DeleteChild(Node* node);
	void GetAllChildBones(Node* node, std::vector<Node*>* Parent);
	std::ifstream file;
	rapidxml::xml_document<> doc;

};