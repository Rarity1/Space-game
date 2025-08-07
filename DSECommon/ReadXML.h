#pragma once
#include "CWin.h"
#include <rapidxml/rapidxml.hpp>

class DLL ReadXML{
public:
	enum PARSE{
		DEFAULT,
		MODEL,
		OTHER,
		WORLD,
		SAVE
	};


	ReadXML(std::string path, PARSE parse = DEFAULT);
	~ReadXML();
	ReadXML(const ReadXML& old):
		file(old.Path)
	{
		std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
		buffer.push_back('\0');
		doc.parse<0>(&buffer[0]);
		WeightCIndex = old.WeightCIndex;
		bdata = old.bdata;
		ndata = old.ndata;
		weights = old.weights;
		Sphere = old.Sphere;
		MappedVertices = old.MappedVertices;
		sIndex = old.sIndex;
		mIndex = old.mIndex;
	}
	struct boneweight {
		std::vector<uint32_t> bIndex{ 0 };
		std::vector<float> weight{ 1.0 };
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
		uint16_t bIndex;
		//Inverse bind pose matrix
		DirectX::XMFLOAT4X4 matrix;
		DirectX::XMFLOAT4X4 finalTransform;
		//sIndex index. Not index of vertices.
		std::vector<uint32_t> Indices;
		Node* node;
		DirectX::BoundingSphere sphere;
		DirectX::BoundingSphere smallsphere;
	};
	std::vector<int> WeightCIndex;
	std::vector<Bone> bdata;
	Node ndata;
	std::vector<boneweight> weights;
	DirectX::BoundingSphere Sphere;
	std::vector<std::array<Vertex, 3>> MappedVertices;
	std::vector<uint32_t> sIndex{};
	std::vector<uint32_t> mIndex{};
private:

	void ReadModel();
	static float fDistance(DirectX::XMFLOAT3& pos1, DirectX::XMFLOAT3& pos2);
	static DirectX::XMFLOAT4 fDirection(DirectX::XMFLOAT3& pos1, DirectX::XMFLOAT3& pos2);

	DirectX::XMFLOAT4X4 strToMatrix(std::istringstream& rawmatri);
	Node ChildNodeRead(rapidxml::xml_node<char>* node);
	void DeleteChild(Node* node);
	void GetAllChildBones(Node* node, std::vector<Node*>* Parent);
	std::ifstream file;
	std::string Path;
	rapidxml::xml_document<> doc;

};