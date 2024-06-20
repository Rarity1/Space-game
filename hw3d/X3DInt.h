#pragma once
#include "CWin.h"
#include <DirectXCollision.h>

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
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 tc;
		DirectX::XMFLOAT3 normal;
	};
	struct vFaceData {
		WORD index;
		WORD normal;
		//WORD texcoord;
	};
	struct Size {
		UINT fSize = 0;
		UINT vCount = 0;
	};
	struct pCollision {
		int index[3]{-1,-1,-1};
		Vertex verts[3];
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
	std::vector<pCollision> cdata;
	std::vector<vFaceData> idata;
	std::vector<int> WeightCIndex;
	std::vector<Bone> bdata;
	Node ndata;
	std::vector<boneweight> weights;
	DirectX::BoundingSphere Sphere;
	Size fsize;
private:
    float fDistance(DirectX::XMFLOAT3* pos1, DirectX::XMFLOAT3* pos2);
	DirectX::XMFLOAT4X4 strToMatrix(std::istringstream& rawmatri);
	Node ChildNodeRead(rapidxml::xml_node<char>* node);
	void cvertexData();
	void DeleteChild(Node* node);
	void GetAllChildBones(Node* node, std::vector<Node*>* Parent);
	std::ifstream file;
	rapidxml::xml_document<> doc;

};