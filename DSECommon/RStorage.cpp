#include "RStorage.h"





RStorage::RStorage()
{
	std::vector<std::filesystem::path> folders = {};
	WCHAR path[MAX_PATH];
	//Wow this is cringe
	GetModuleFileNameW(NULL, path, MAX_PATH);
	std::filesystem::current_path(std::wstring(path).substr(0, std::wstring(path).find(L"\\")));
	for (auto& file : std::filesystem::directory_iterator{ std::filesystem::current_path() / "models" }) {
		if (file.path().extension() == ".dae") {
			mData smData;
			smData.modelPath = file.path();
			bool toggle = false;
			auto name = file.path().filename().string().substr(0, file.path().filename().string().find(file.path().extension().string()));
			std::vector<char> UniqueID(1, '0');
			for (auto& chr : name) {
				if (chr == '#') {
					toggle = !toggle ? true : false;
				}
				if (toggle && chr != '#') {
					UniqueID.emplace_back(chr);
				}
				if (!toggle && chr != '#') {
					smData.name = smData.name + chr;
				}

			}
			smData.umID = std::stoul(std::string(UniqueID.data(), UniqueID.size()));
			AvailableModels.emplace(smData.umID, smData);
		}
	}
	for (auto& file : std::filesystem::directory_iterator{ std::filesystem::current_path() / "textures" }) {
		if (file.path().extension() == ".dds") {
			Textures.emplace_back(file.path());
		}
	}
}







RStorage::mData* RStorage::GetMData(UINT umID) {
	_ASSERT(AvailableModels.find(umID) != AvailableModels.end());
	return &AvailableModels[umID];
}

RStorage::bmResource* RStorage::loadModel(mData& umData)
{
	if (AvailableModels[umData.umID].model == nullptr) {
		//Need to count referenced myself inorder to delete them
		LoadedModels.emplace(umData.umID, RStorage::bmResource(umData.umID, std::make_unique<ReadXML>(umData.modelPath.string(), ReadXML::PARSE::MODEL), umData.name));
		AvailableModels[umData.umID].model = &LoadedModels[umData.umID];
	}
	return AvailableModels[umData.umID].model;
}

void RStorage::trackModelID(UINT umID, UINT UOID)
{
	tmodelIndexMap[UOID] = umID;
}

UINT RStorage::getModelID(UINT UOID)
{
	return tmodelIndexMap[UOID];
}

std::filesystem::path RStorage::getTexture(std::string name)
{
	auto result = std::find_if(Textures.begin(), Textures.end(), [&name](auto& e) {return e.filename().string().substr(0, e.filename().string().find(e.extension().string())) == name; });
	

	return result != Textures.end() ? *result : L"";
}



RStorage::~RStorage()
{
	for (auto& a : LoadedModels) {
		if (a.second.cbvwriteBuffer != nullptr) {
			a.second.cbvwriteBuffer->Unmap(0, nullptr);
		}
	}
}

