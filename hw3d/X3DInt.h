#pragma once
#include "CWin.h"

class ReadX3D{
public:
	ReadX3D(std::string path);
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
	std::vector<Vertex> vertexData();
	std::vector<vFaceData> indexData();
	Size fSize();
private:
	std::vector<Vertex> vdata;
	std::vector<vFaceData> idata;
	Size fsize;
	void cvertexData();
	std::ifstream file;
	rapidxml::xml_document<> doc;
	struct lData {
		rapidxml::xml_node<char>* mX3D;
		rapidxml::xml_node<char>* mScene;
		rapidxml::xml_node<char>* mVertices;
	};
};