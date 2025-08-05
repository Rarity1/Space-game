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
			auto& smData = AvailableModels.emplace_back(mData());
			smData.model = file.path();
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
			
		}
	}
	for (auto& file : std::filesystem::directory_iterator{ std::filesystem::current_path() / "textures" }) {
		if (file.path().extension() == ".dds") {
			Textures.emplace_back(file.path());
		}
	}
}







RStorage::mData* RStorage::GetModel(UINT umID) {
	auto result = std::find_if(AvailableModels.begin(), AvailableModels.end(), [umID](const RStorage::mData& UDat) {
		return UDat.umID == umID;
	});
	_ASSERT(result != AvailableModels.end());
	return &*result;
}

RStorage::bmResource* RStorage::loadModel(mData& umData)
{
	auto result = std::find_if(LoadedModels.begin(), LoadedModels.end(), [umData](const RStorage::bmResource& UDat) {
		return umData.umID == UDat.umID;
	});
	if (result == LoadedModels.end()) {
		return &LoadedModels.emplace_back(RStorage::bmResource(umData.umID, std::make_unique<ReadXML>(umData.model.string(), ReadXML::PARSE::MODEL ), umData.name));
	}
	return &*result;
}

void RStorage::trackModelID(UINT umID, UINT UOID)
{
	trackedModels[umID].emplace_back(UOID);
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
}

