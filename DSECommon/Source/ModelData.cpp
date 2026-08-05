 #include "ModelData.h"
#include <cmath>
#include <cstdint>
 #include <fstream>
 #include <numeric>
 #include <sstream>
 #include <cstring>

ModelData::ModelData(std::string path, PARSE parse) :
	Path(path)
{

	std::unique_ptr<rapidxml::xml_document<char>> doc = std::make_unique<rapidxml::xml_document<>>();
	std::unique_ptr<std::vector<char>> buffer = std::make_unique<std::vector<char>>();
	{
		auto file = std::ifstream(Path);

		buffer->append_range(std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
		buffer->push_back('\0');
	}


	ReadModel(std::move(doc), std::move(buffer));
}


void ModelData::GetAllChildBones(Node& node, std::vector<Node*>& Parent)
{
	for (auto& n : node.children) {
		Parent.emplace_back(&n);
		GetAllChildBones(n, n.aChildren);
		Parent.insert(Parent.end(), n.aChildren.begin(), n.aChildren.end());
	}
		
		
}

ModelData::Node ModelData::ChildNodeRead(rapidxml::xml_node<char>* node) {
	ModelData::Node mParent;
	{
		std::istringstream rawmatri(node->first_node("matrix")->value());
		mParent.matrix = strToMatrix(rawmatri);
	}

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
				auto& temp = mParent.children.emplace_back(ChildNodeRead(tempnode));
				temp.name = tempnode->first_attribute("name")->value();
			}
		}
	}
	mParent.numchild = std::size(mParent.children);
	for (auto& n : mParent.children) {
		mParent.numchild += n.numchild;
	}
	return mParent;
}

void ModelData::ReadModel(std::unique_ptr<rapidxml::xml_document<char>> doc, std::unique_ptr<std::vector<char>> buffer)
{
	doc->parse<0>(buffer->data());
	auto tnode = doc->first_node();
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
				if (source->first_attribute() != nullptr)
					if ((std::strstr((source->first_attribute()->value()), "positions") != nullptr) && (positions == nullptr)) {
						positions = source->first_node("float_array");
						auto ssource = source->first_node("technique_common")->first_node("accessor");
						std::stringstream(ssource->first_attribute("count")->value()) >> size;
					}
					else if ((std::strstr((source->first_attribute()->value()), "normals") != nullptr) && (pnorm == nullptr)) {
						pnorm = source->first_node("float_array");

					}
					else if ((std::strstr((source->first_attribute()->value()), "map") != nullptr) && (pmap == nullptr)) {
						pmap = source->first_node("float_array");

					}
					else
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
				if (source->first_attribute() != nullptr)
					if ((std::strstr(source->first_attribute()->value(), "joints") != nullptr) && (bonenames == nullptr)) {


						bonenames = source->first_node("Name_array");
						auto ssource = source->first_node("technique_common");
						ssource = ssource->first_node("accessor");
						std::stringstream(ssource->first_attribute("count")->value()) >> numbones;
					}
					else
						if (std::strstr(source->first_attribute()->value(), "bind_poses") != nullptr) {
							bindpose = source->first_node("float_array");
						}
						else
							if (std::strstr(source->first_attribute()->value(), "weights") != nullptr) {
								skinweights = source->first_node("float_array");
							}
							else if (std::strcmp(source->name(), "vertex_weights") == 0) {
								vcount = source->first_node("vcount");
								vertexweights = source->first_node("v");
							}
				source = source->next_sibling();

			}



		}
		if (std::strcmp(node->name(), "library_visual_scenes") == 0) {
			auto source = node->first_node("visual_scene")->first_node("node");
			while (source != nullptr) {
				if (source->first_attribute() != nullptr) {

					ndata = ChildNodeRead(source);

				}
				source = source->next_sibling();
			}
		}
	}



	if (numbones > 0) {
		std::string bone;
		{
			uint16_t tempc = 0;
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
			auto cindJoin = 0;
			while (reet >> tempor) {
				auto& workw = weights.emplace_back(boneweight({}, {}));


				for (auto j = 0; j < tempor; j++) {
					workw.bIndex.emplace_back(join[cindJoin]);
					workw.weight.emplace_back(skeenweigh[cindJoin]);
					cindJoin++;
				}

			}
		}

	}

	GetAllChildBones(ndata, ndata.aChildren);
	ndata.name = "Armature";
	{


	}
	std::vector<Vertex> loadedVdata;
	for (auto i = 0; i < std::size(bdata); i++) {
		bdata[i].node = ndata.aChildren[i];
		bdata[i].node->bIndex = bdata[i].bIndex;
	}
	{
		std::stringstream ssvertex(positions->value());
		float x, y, z;
		while (ssvertex >> x >> y >> z) {
			loadedVdata.emplace_back(Vertex{ .position{x, y, z} });
		}

	}

	std::vector<FLOAT2> tempcoord;
	{
		std::stringstream ssmap(pmap->value());
		float mx, my;

		while (ssmap >> mx >> my) {
			tempcoord.emplace_back(mx, 1.0f - my);
		}


	}
	std::vector<FLOAT3> normals;
	{
		float nx, ny, nz;
		std::stringstream ssnorm(pnorm->value());
		while (ssnorm >> nx >> ny >> nz) {
			normals.emplace_back(nx, ny, nz );
		}
	}


	std::vector<vFaceData> idata;

	{
		std::stringstream ssindex(parray->value());
		uint32_t vertex, normal, texcoord;
		while (ssindex >> vertex >> normal >> texcoord) {
			vFaceData t = { vertex, normal, texcoord };
			idata.emplace_back(t);
		}

	}
	//Idata only correct if inverted with RH view
	std::reverse(idata.begin(), idata.end());

	sIndex.resize(idata.size());
	std::iota(sIndex.begin(), sIndex.end(), 0);


	mIndex.resize(idata.size());


	std::vector<Vertex> mVdata(std::size(idata));
	std::vector<boneweight> tempweights(std::size(idata));
	uint32_t modcount = 0;
	for (uint32_t s = 0; s < sIndex.size(); s++) {
		mIndex[s] = modcount;
		modcount = s % 3 == 2 ? modcount + 1 : modcount;
		mVdata[s] = loadedVdata[idata[s].index];
		mVdata[s].tc = tempcoord[idata[s].texcoord];
		mVdata[s].normal = normals[idata[s].normal];
	}
	if (weights.size() > 0) {
		for (int s = 0; s < sIndex.size(); s++) {
			tempweights[s] = weights[idata[s].index];
		}
	}
	weights = tempweights;

	//Triangle Mapped Vertices. Data is stored equal to non mapped.
	MappedVertices.resize(mVdata.size() / 3);

	memcpy(MappedVertices.data(), mVdata.data(), sizeof(Vertex) * mVdata.size());




	if (std::size(bdata) == 0) {
		auto& working = bdata.emplace_back(Bone{ "placeholder", 0 });
		working.Indices = sIndex;
	}
	else {
		for (auto i = 0; i < sIndex.size(); i++) {
			for (uint32_t j : weights[i].bIndex) {
				bdata[j].Indices.emplace_back(i);
			}
		}
	}




	{

		FLOAT3 Zero{ 0,0,0 };
		FLOAT3 tVert;
		for (auto i = 0; i < mVdata.size(); i++) {
      tVert = mVdata[i].position;
			auto temp = fDistance(tVert, Zero);
			Sphere.Radius = temp > Sphere.Radius ? temp : Sphere.Radius;
		}
		Sphere.Center = { 0,0,0 };


	}


	for (auto& b : bdata) {

		FLOAT3 pos{ 0,0,0 };
		std::for_each(b.Indices.begin(), b.Indices.end(), [&pos, &mVdata](auto& x) {
			pos = mVdata[x].position + pos;
		});
		float isize = b.Indices.size();
		if (isize > 0) {
			b.sphere.Center = pos / isize;
		}
		b.smallsphere = b.sphere;

		float dist = 0;
		float fdist = 0;
		float ldist = 0;
		float tdist = 0;
		for (auto& i : b.Indices) {
			fdist = fDistance(b.sphere.Center, mVdata[i].position);
			dist = fdist > dist ? fdist : dist;

			ldist = (fdist < ldist) || (ldist == 0) ? fdist : ldist;


		}
		b.sphere.Radius = dist;
		b.smallsphere.Radius = ldist;
	}

}

float ModelData::fDistance(FLOAT3& pos1, FLOAT3& pos2) {
	FLOAT3 Result{ 0,0,0 };
	FLOAT3 Dist{ 0,0,0 };
	float d = 0;
	Result = pos2 - pos1;
	Dist = Result * Result;
	d = sqrt(Dist.x);
	return d;
}

FLOAT4 ModelData::fDirection(FLOAT3& pos1, FLOAT3& pos2) {
	FLOAT3 Result{ 0,0,0};
	FLOAT3 Dist{ 0,0,0 };
	float d = 0;
	Result = pos2 - pos1;
	Dist = Result * Result;
	d = sqrt(Dist.x);
	if (d > 0) {
		return Result / d;
	}
	return { 0,0,0,0 };
}

FLOAT4X4 ModelData::strToMatrix(std::istringstream &rawmatri) {
  FLOAT4X4 float4x4;
  float x, y, z, w;
  int caser = 0;
  while (rawmatri >> x >> y >> z >> w) {
    switch (caser) {
    case (0):
      float4x4.a = {x, y, z, w};
      break;
    case (1):
      float4x4.b = {x, y, z, w};
      break;
    case (2):
      float4x4.c = {x, y, z, w};
      break;
    case (3):
      float4x4.d = {x, y, z, w};
      break;
    }

    caser++;
  }

  return float4x4;
}

ModelData::~ModelData()
{
}
