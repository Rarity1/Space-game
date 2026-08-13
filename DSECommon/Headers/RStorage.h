#pragma once
#include "CommonStructs.h"
#include "Exceptions.h"
#include "GraphicsErrors.h"
#include "ModelData.h"
#include "text.h"
#include <CL/opencl.hpp>
#include <cstdint>
#include <filesystem>
#include <list>
#include <unordered_map>

//This is not threadsafe. Definitely need to fix that
// Unique model ID
typedef uint32_t umID;
class DLL RStorage {
  // friend class Object;
  // friend class Graphics;
public:
  RStorage();
  ~RStorage(){
    AvailableModels.clear();
  };

  struct bmResource {
    bmResource():mID(0),name(""),modPath(""){
      "Error">>chk;
    };
    bmResource(const bmResource&)=delete;
    bmResource(umID mID, std::string Path, std::string Name)
        : mID(mID), name(Name), modPath(Path) {
          ModelList.emplace_back(mID);
          listRef = --ModelList.end();
        };
    bmResource(bmResource &&old)
        : mID(std::move(old.mID)),
          name(std::move(old.name)), modPath(std::move(old.modPath)) {

      listRef = std::move(old.listRef);
      old.listRef = ModelList.end();
      curTexture = std::move(old.curTexture);
      clBoneBuff = std::move(old.clBoneBuff);
      clBuff = std::move(old.clBuff);
      clIndexBuff = std::move(old.clIndexBuff);
      clIndexMap = std::move(old.clIndexMap);
    };
    ~bmResource(){if(listRef != ModelList.end()){
      ModelList.erase(listRef);
    };}
    const umID mID;
    const std::string name;
    const std::string modPath;
    std::_List_iterator<umID> listRef;
    // Instanced texture path and buffer.
    std::filesystem::path curTexture;
    cl::Buffer clBoneBuff;
    cl::Buffer clBuff;
    cl::Buffer clIndexBuff;
    cl::Buffer clIndexMap;
    const ModelData* isModelLoaded(){return RStorage::isModelLoaded(mID) ? &LoadedModels[mID] : nullptr;};
    const ModelData* GetOrLoadModel(){return RStorage::isModelLoaded(mID) ? &LoadedModels[mID] : loadModel(mID);}
  };
  static ModelData* loadModel(umID mID);
  static ModelData* GetModel(umID mID);
  inline const std::list<umID>& GetModelList() const{return ModelList;};
  static inline bmResource* GetResource(umID mID){ return AvailableModels.contains(mID) ? &AvailableModels[mID] : nullptr;};
  static inline bool isModelLoaded(umID mID) {return LoadedModels.contains(mID);};
  std::filesystem::path getTexture(std::string name);
private:
  static std::unordered_map<umID, bmResource> AvailableModels;
  static std::list<umID> ModelList;
  static std::unordered_map<umID, ModelData> LoadedModels;
  static std::unordered_map<std::string, std::filesystem::path> Textures;
  static Exceptions::CheckerToken chk;
};
