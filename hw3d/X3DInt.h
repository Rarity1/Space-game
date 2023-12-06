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
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 tc;
		DirectX::XMFLOAT3 normal;
	};
	struct vFaceData {
		WORD index;
		WORD normal;
		WORD texcoord;
	};
	struct Size {
		UINT fSize = 0;
		UINT vCount = 0;
	};
	struct pCollision {
		DirectX::XMFLOAT3 pos{0,0,0};
		float radius = 0;
		int index = -1;
		std::vector<DirectX::XMFLOAT3> verts ={{}, {}, {}};
	};
	struct Bone {
		std::string name;
		int bIndex;
		DirectX::XMFLOAT4X4 matrix;
		std::vector<int> Indices;
	};

	struct Node {
		std::string name;
		DirectX::XMFLOAT4X4 matrix;
		std::vector<Node*> children = {};
		int numchild = 0;
		std::vector<Node*> aChildren = {};
	};
	std::vector<Vertex> vdata;
	std::vector<vFaceData> idata;
	std::vector<pCollision> cdata;
	std::vector<Bone> bdata;
	Node ndata;
	float collradius;
	Size fsize;
private:
	DirectX::XMFLOAT4X4 strToMatrix(std::istringstream& rawmatri);
	Node ChildNodeRead(rapidxml::xml_node<char>* node);
	void cvertexData();
	void DeleteChild(Node* node);
	void GetAllChildBones(Node* node, std::vector<Node*>* Parent);
	std::ifstream file;
	rapidxml::xml_document<> doc;

};