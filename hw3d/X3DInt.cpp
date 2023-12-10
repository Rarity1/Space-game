#include "X3DInt.h"

ReadX3D::ReadX3D(std::string path) :
	file(path)
{
	cvertexData();
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
	rapidxml::xml_node<char>* pnorm = nullptr;
	rapidxml::xml_node<char>* parray = nullptr;

	rapidxml::xml_node<char>* bonenames = nullptr;
	int numbones = 0;
	rapidxml::xml_node<char>* bindpose = nullptr;
	rapidxml::xml_node<char>* skinweights = nullptr;
	rapidxml::xml_node<char>* vertexweights = nullptr;
	rapidxml::xml_node<char>* vcount = nullptr;
	std::vector<int> countofv;
	for (rapidxml::xml_node<>* node = tnode->first_node(); node; node = node->next_sibling()) {
		if (std::strcmp(node->name(), "library_geometries") == 0) {
			auto source = node->first_node("geometry")->first_node("mesh")->first_node();
			while (source != nullptr)
			{
				if(source->first_attribute() != nullptr)
				if ((std::strstr((source->first_attribute()->value()), "positions") != nullptr) && (positions == nullptr)) {
					positions = source->first_node("float_array");
					auto ssource = source->first_node("technique_common")->first_node("accessor");
					std::stringstream(ssource->first_attribute("count")->value()) >> size;
				}else if((std::strstr((source->first_attribute()->value()), "normals") != nullptr) && (pnorm == nullptr)) {
					pnorm = source->first_node("float_array");

				}
				else if ((std::strstr((source->first_attribute()->value()), "map") != nullptr) && (pmap == nullptr)) {
					pmap = source->first_node("float_array");
					
				}else
				if ((std::strcmp(source->name(), "triangles") == 0) && (parray == nullptr)) {
					parray = source->first_node("p");
				}
				source = source->next_sibling();

			}

		}
		if (std::strcmp(node->name(), "library_controllers") == 0) {
			auto source = node->first_node("controller")->first_node("skin")->first_node();
				while (source != nullptr)
				{
					if(source->first_attribute() != nullptr)
					if ((std::strstr(source->first_attribute()->value(), "joints") != nullptr) && (bonenames == nullptr)) {


						bonenames = source->first_node("Name_array");
						auto ssource = source->first_node("technique_common");
						ssource = ssource->first_node("accessor");
						std::stringstream(ssource->first_attribute("count")->value()) >> numbones;
					}else
					if (std::strstr(source->first_attribute()->value(), "bind_poses") != nullptr) {
						bindpose = source->first_node("float_array");
					}else
					if (std::strstr(source->first_attribute()->value(), "weights") != nullptr) {
						skinweights = source->first_node("float_array");
						auto pee = 0;
					}else if (std::strcmp(source->name(), "vertex_weights") == 0) {
						vcount = source->first_node("vcount");
						vertexweights = source->first_node("v");
					}
					source = source->next_sibling();
					
				}

				
				
		}
		if (std::strcmp(node->name(), "library_visual_scenes") == 0) {
			auto source = node->first_node("visual_scene")->first_node("node");
			while (source != nullptr) {
				if(source->first_attribute() != nullptr){
					
					ndata = ChildNodeRead(source);
				
				}
				source = source->next_sibling();
			}
		}
	}



	if (numbones > 0) {
		std::string bone;
		{
			auto tempc = 0;
			std::istringstream bn(bonenames->value());
			while (bn >> bone) {
				bdata.emplace_back(Bone{ bone, tempc });
				tempc++;
			}
		} 
		{
			std::istringstream tm(bindpose->value());

			float matr;
			int counter = 0;
			int counter2 = 0;

			std::vector<std::string> matrix;

			int tp;
			std::istringstream(bindpose->first_attribute("count")->value()) >> tp;
			matrix.resize(tp / 16);
			while (tm >> matr) {
				matrix[counter2] += " " + std::to_string(matr);
				counter++;
				if ((counter % 16) == 0) {
					counter2++;
				}
				
				
			}


			for (auto& b : bdata) {
				std::istringstream temp(matrix[b.bIndex]);
				b.matrix = strToMatrix(temp);
			}


			std::istringstream sw(skinweights->value());
			std::vector<float> skeenweigh;
			float m;
			while (sw >> m) {
				skeenweigh.emplace_back(m);
			}
			std::vector<int> join;
			std::vector<int> weig;
			int joint, weight;
			std::istringstream vw(vertexweights->value());
			while (vw >> joint >> weight) {
				join.emplace_back(joint);
				weig.emplace_back(weight);
			}
			int tempor;
			std::istringstream reet(vcount->value());
			while (reet >> tempor) {
				countofv.emplace_back(tempor);
			}
			weights.resize(std::size(countofv));
			auto tempc = 0;
			for (auto i = 0; i < std::size(countofv); i++) {
				for (auto j = 0; j < countofv[i]; j++) {
					weights[i].bIndex.emplace_back(join[tempc]);
					weights[i].weight.emplace_back(skeenweigh[tempc]);
					tempc++;
				}
			}
		}
		
	}

	GetAllChildBones(&ndata, &ndata.aChildren);
	ndata.name = "Armature";
	
	for (auto i = 0; i < std::size(bdata); i++) {
		bdata[i].node = ndata.aChildren[i];
		bdata[i].node->bIndex = bdata[i].bIndex;
	}
	
	{
		std::stringstream ssvertex(positions->value());
		float x, y, z;
	while (ssvertex >> x >> y >> z) {
		vdata.emplace_back(Vertex{ .position{x, y, z} });
	}
	}
	std::vector<DirectX::XMFLOAT2> tempcoord;
	{
		std::stringstream ssmap(pmap->value());
		float mx, my;
		
		while (ssmap >> mx >> my) {
			tempcoord.emplace_back(mx, 1 - my);
	}
	}
	std::vector<DirectX::XMFLOAT3> normals;
	{
		float nx, ny, nz;
		std::stringstream ssnorm(pnorm->value());
		while (ssnorm >> nx >> ny >> nz) {
			normals.emplace_back(DirectX::XMFLOAT3{nx, ny, nz});
		}
	}
	



	std::stringstream ssindex(parray->value());
	WORD vertex, normal, texcoord;
	while (ssindex >> vertex >> normal >> texcoord) {
		vFaceData t = {vertex, normal};
		idata.emplace_back(t);
	}
	std::vector<Vertex> tempdata;
	std::vector<vFaceData> tempindex;
	std::vector<boneweight> tempvcount;
	for (auto c = 0; c < size; c++) {
		for (auto& i : idata) {
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

	for (auto& i : idata) {
		tempdata.push_back(vdata[i.index]);
		if(std::size(weights) > 0)
		tempvcount.push_back(weights[i.index]);
	}
	for (auto& i : tempindex) {
		tempdata[i.index].tc = tempcoord[i.index];
		tempdata[i.index].normal = normals[i.normal];
	}
	
	vdata = tempdata;
	idata = tempindex;
	weights = tempvcount;
	
	for (auto i = 0; i < std::size(tempvcount); i++) {
		for (auto& j : tempvcount[i].bIndex) {
			bdata[j].Indices.emplace_back(i);
		}
	}

	cdata.resize(std::size(idata)/3);

	auto tempcount = 0;
	for (auto& i : idata) {
		
		cdata[tempcount].verts[i.index%3] = vdata[i.index].position;
		cdata[tempcount].index = tempcount;
		tempcount += i.index % 3 == 0 ? 1 : 0;
	}
	collradius = 0;
	for (auto& c : cdata) {
		c.pos = { (c.verts[0].x + c.verts[1].x + c.verts[2].x) / 3, (c.verts[0].y + c.verts[1].y + c.verts[2].y) / 3, (c.verts[0].z + c.verts[1].z + c.verts[2].z) / 3};
		auto x = c.pos.x - c.verts[0].x;
		auto y = c.pos.y - c.verts[0].y;
		auto z = c.pos.z - c.verts[0].z;
		c.radius = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
		auto temp = sqrt(pow(c.pos.x, 2) + pow(c.pos.y, 2) + pow(c.pos.z, 2));
		collradius = temp > collradius ? temp : collradius;
	}

	fsize.fSize = sizeof(ReadX3D::Vertex) * std::size(vdata);
	fsize.vCount = std::size(vdata);
}

void ReadX3D::GetAllChildBones(Node* node, std::vector<Node*>* Parent)
{
	for (auto& n : node->children) {
		Parent->emplace_back(n);
		GetAllChildBones(n, &n->aChildren);
		Parent->insert(Parent->end(), n->aChildren.begin(), n->aChildren.end());
	}
		
		
}

ReadX3D::Node ReadX3D::ChildNodeRead(rapidxml::xml_node<char>* node) {
	ReadX3D::Node mParent;
	std::istringstream rawmatri(node->first_node("matrix")->value());
	mParent.matrix = strToMatrix(rawmatri);
	if (std::strcmp(node->first_attribute("type")->value(), "JOINT") == 0 && node->first_node("extra")->first_node("technique")->first_node("tip_x") != nullptr) {
		float x;
		std::stringstream(node->first_node("extra")->first_node("technique")->first_node("tip_x")->value()) >> x;
		float y;
		std::stringstream(node->first_node("extra")->first_node("technique")->first_node("tip_y")->value()) >> y;
		float z;
		std::stringstream(node->first_node("extra")->first_node("technique")->first_node("tip_z")->value()) >> z;
		float roll;
		//std::stringstream(node->first_node("extra")->first_node("technique")->first_node("roll")->value()) >> roll;
		mParent.pos = { x, y, z, 0.f };
	}
	for (auto tempnode = node->first_node("node"); tempnode; tempnode = tempnode->next_sibling()) {
		if (tempnode != nullptr && tempnode->first_attribute("type") != nullptr) {
			if (std::strcmp(tempnode->first_attribute("type")->value(), "JOINT") == 0) {
				auto temp = new ReadX3D::Node(ChildNodeRead(tempnode));
				temp->name = tempnode->first_attribute("name")->value();
				mParent.children.emplace_back(temp);
			}
		}
	}
	mParent.numchild = std::size(mParent.children);
	for (auto& n : mParent.children) {
		mParent.numchild += n->numchild;
	}
	return mParent;
}

DirectX::XMFLOAT4X4 ReadX3D::strToMatrix(std::istringstream& rawmatri) {
	float m;
	std::vector<float> tempfloats;
	while (rawmatri >> m) {
		tempfloats.emplace_back(m);
	}
	int rowcount = 0;
	DirectX::XMFLOAT4X4 float4x4;
	for (auto i = 0; i < 16; i++) {
		switch (rowcount) {
		case(0): {
			switch (i % 4) {
			case(0): {
				float4x4._11 = tempfloats[i];
				break;
			}
			case(1): {
				float4x4._12 = tempfloats[i];
				break;
			}
			case(2): {
				float4x4._13 = tempfloats[i];
				break;
			}
			case(3): {
				float4x4._14 = tempfloats[i];
				rowcount++;
				break;
			}
			}
			break;
		}
		case(1): {
			switch (i % 4) {
			case(0): {
				float4x4._21 = tempfloats[i];
				break;
			}
			case(1): {
				float4x4._22 = tempfloats[i];
				break;
			}
			case(2): {
				float4x4._23 = tempfloats[i];
				break;
			}
			case(3): {
				float4x4._24 = tempfloats[i];
				rowcount++;
				break;
			}
			}
			break;
		}
		case(2): {
			switch (i % 4) {
			case(0): {
				float4x4._31 = tempfloats[i];
				break;
			}
			case(1): {
				float4x4._32 = tempfloats[i];
				break;
			}
			case(2): {
				float4x4._33 = tempfloats[i];
				break;
			}
			case(3): {
				float4x4._34 = tempfloats[i];
				rowcount++;
				break;
			}
			}
			break;
		}
		case(3): {
			switch (i % 4) {
			case(0): {
				float4x4._41 = tempfloats[i];
				break;
			}
			case(1): {
				float4x4._42 = tempfloats[i];
				break;
			}
			case(2): {
				float4x4._43 = tempfloats[i];
				break;
			}
			case(3): {
				float4x4._44 = tempfloats[i];
				rowcount++;
				break;
			}
			}
			break;
		}
		}
	}
	return float4x4;
}

void ReadX3D::DeleteChild(Node* node) {
	if (std::size(node->children) > 0)
		for (auto& n : node->children) {
			DeleteChild(n);
		}
	else
		delete node;
}

ReadX3D::~ReadX3D()
{
	for (auto& n : ndata.children) {
		DeleteChild(n);
	}
}
