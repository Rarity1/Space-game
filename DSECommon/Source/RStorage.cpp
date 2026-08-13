#include "RStorage.h"



  std::unordered_map<umID, RStorage::bmResource> RStorage::AvailableModels;
  std::list<umID> RStorage::ModelList;
  std::unordered_map<umID, ModelData> RStorage::LoadedModels;
  std::unordered_map<std::string, std::filesystem::path> RStorage::Textures;
  Exceptions::CheckerToken RStorage::chk;

RStorage::RStorage()
{
	for (auto& file : std::filesystem::directory_iterator{ _CURRENTPATH / "models" }) {
		if (file.path().extension() == ".dae") {
			bool toggle = false;
			auto name = file.path().filename().string().substr(0, file.path().filename().string().find(file.path().extension().string()));
			std::string ModelName;
      std::vector<char> UniqueID(1, '0');
			for (auto& chr : name) {
				if (chr == '#') {
					toggle = !toggle ? true : false;
				}
				if (toggle && chr != '#') {
					UniqueID.emplace_back(chr);
				}
				if (!toggle && chr != '#') {
					ModelName = ModelName + chr;
				}

			}
			umID mID = std::stoul(std::string(UniqueID.data(), UniqueID.size()));

			AvailableModels.emplace(mID, bmResource(mID, file.path().string(), ModelName));
		}
	}

	for (auto& file : std::filesystem::directory_iterator{ _CURRENTPATH / "textures" }) {
    auto name = file.path().filename().string().substr(0, file.path().filename().string().find(file.path().extension().string()));
		if (file.path().extension() == ".dds") {
			Textures.emplace(name,file.path());
		}
	}
}

ModelData* RStorage::GetModel(umID mID) {
  if(LoadedModels.contains(mID)){
    return &LoadedModels[mID];
  }else{
    return nullptr;
  }
}


ModelData* RStorage::loadModel(umID mID) {
  ModelData* Result = nullptr;
  if (AvailableModels.contains(mID)) {
    auto &currentModel = AvailableModels[mID];
    if (!isModelLoaded(mID)) {
      // Need to count referenced myself inorder to delete them
      LoadedModels.emplace(mID, ModelData(currentModel.modPath));
    }
    Result = &LoadedModels[mID];
  };
  return Result;
}

std::filesystem::path RStorage::getTexture(std::string name)
{
	return Textures.contains(name) ? Textures[name] : L"";
}
