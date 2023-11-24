#include "X3DInt.h"

ReadX3D::ReadX3D(std::string path) :
	file(path)
{
	cvertexData();
};

std::vector<ReadX3D::Vertex> ReadX3D::vertexData()
{
	return vdata;
}

std::vector<ReadX3D::vFaceData> ReadX3D::indexData()
{
	return idata;
}

ReadX3D::Size ReadX3D::fSize() {
	return fsize;
}

void ReadX3D::cvertexData()
{

	std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	buffer.push_back('\0');
	doc.parse<0>(&buffer[0]);
	auto tnode = doc.first_node();
	int size = 0;
	rapidxml::xml_node<char>* positions = nullptr;
	rapidxml::xml_node<char>* pmap = nullptr;
	rapidxml::xml_node<char>* parray = nullptr;
	for (rapidxml::xml_node<>* node = tnode->first_node(); node; node = node->next_sibling()) {
		if (std::strcmp(node->name(), "library_geometries") == 0) {
			node = node->first_node("geometry");
			node = node->first_node("mesh");
			for (rapidxml::xml_node<>* source = node->first_node();
				source; source = source->next_sibling())
			{
				if (std::strstr(source->first_attribute()->value(), "positions") != nullptr) {
					positions = source->first_node("float_array");
					auto ssource = source->first_node("technique_common");
					ssource = ssource->first_node("accessor");
					std::stringstream(ssource->first_attribute("count")->value()) >> size;
				}
				if (std::strstr(source->first_attribute()->value(), "map") != nullptr) {
					pmap = source->first_node("float_array");
					
				}
				if (std::strcmp(source->name(), "triangles") == 0) {
					parray = source->first_node("p");
				}

			}

		}
	}

	std::stringstream ssvertex(positions->value());
	float x, y, z;
	while (ssvertex >> x >> y >> z) {
		vdata.emplace_back(Vertex{ .position{x, y, z}});
	}
	std::stringstream ssmap(pmap->value());

	float mx, my;
	std::vector<DirectX::XMFLOAT2> temp;
	while (ssmap >> mx >> my) {
		temp.emplace_back(mx, 1-my);
	}
	std::stringstream ssindex(parray->value());
	WORD vertex, normal, texcoord;
	while (ssindex >> vertex >> normal >> texcoord) {
		vFaceData t = {vertex};
		idata.emplace_back(t);
	}
	std::vector<Vertex> tempdata;
	std::vector<vFaceData> tempindex;
	for (auto c = 0; c < size; c++) {
		for (auto i : idata) {
			if (i.index == c) {
				tempindex.emplace_back(i);
			}

		}
	}

	
	auto c = std::size(tempindex)-1;
	for (auto& i : tempindex) {
		i.index = c;
		c-=1;
	}

	for (auto i : idata) {
		tempdata.push_back(vdata[i.index]);
	}
	for (auto i : tempindex) {
		tempdata[i.index].tc = temp[i.index];
	}
	
	vdata = tempdata;
	idata = tempindex;
	


	fsize.fSize = sizeof(ReadX3D::Vertex) * std::size(vdata);
	fsize.vCount = std::size(vdata);
}
