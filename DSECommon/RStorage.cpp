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







RStorage::bmResource* RStorage::GetModel(UINT umID) {
	if (LoadedModels.find(umID) != LoadedModels.end()) {
		return &LoadedModels[umID];
	}
	else return nullptr;
}

RStorage::bmResource* RStorage::loadModel(UINT umID)
{

	_ASSERT(AvailableModels.find(umID) != AvailableModels.end());
	auto& currentModel = AvailableModels[umID];
	if (currentModel.model == nullptr) {
		//Need to count referenced myself inorder to delete them
		LoadedModels.emplace( umID, RStorage::bmResource(umID, ModelData(currentModel.modelPath.string(), ModelData::PARSE::MODEL), currentModel.name) );
		currentModel.model = &LoadedModels[umID];
	}
	return AvailableModels[umID].model;
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

