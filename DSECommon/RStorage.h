#pragma once
#include "CWin.h"
#include "ReadXML.h"
#include "GraphicsErrors.h"
#include "text.h"
#include <CL/opencl.hpp>


class DLL RStorage {
	//friend class Object;
	//friend class Graphics;
public:
	RStorage();
	~RStorage();

	//Do NOT copy.
	struct bmResource {
		bmResource(UINT uemID, std::unique_ptr<ReadXML> uData, std::string& name):
			umID(uemID),
			uData(std::move(uData)),
			name(name)
		{};
		~bmResource() {};
		bmResource(bmResource&& old) noexcept {
			umID = std::move(old.umID);
			uData = std::move(old.uData);
			name = std::move(old.name);
			vbuffer = std::move(old.vbuffer);
			ibuffer = std::move(old.ibuffer);
			uvbuffer = std::move(old.uvbuffer);
			uibuffer = std::move(old.uibuffer);
			vbuffView = std::move(old.vbuffView);
			ibuffView = std::move(old.ibuffView);
			clBoneBuff = std::move(old.clBoneBuff);
			clBuff = std::move(old.clBuff);
			clIndexBuff = std::move(old.clIndexBuff);
			clIndexMap = std::move(old.clIndexMap);
		};
		bmResource& operator = (bmResource&& old) noexcept {
			if (this != &old) {
				umID = std::move(old.umID);
				uData = std::move(old.uData);
				name = std::move(old.name);
				vbuffer = std::move(old.vbuffer);
				ibuffer = std::move(old.ibuffer);
				uvbuffer = std::move(old.uvbuffer);
				uibuffer = std::move(old.uibuffer);
				vbuffView = std::move(old.vbuffView);
				ibuffView = std::move(old.ibuffView);
				clBoneBuff = std::move(old.clBoneBuff);
				clBuff = std::move(old.clBuff);
				clIndexBuff = std::move(old.clIndexBuff);
				clIndexMap = std::move(old.clIndexMap);
			}
			return *this;
		}
		bmResource(const bmResource&) { };
		bmResource& operator=(const bmResource&) { return *this; };
		/*
		{

			umID = old.umID;
			uData = std::make_unique<ReadXML>(*old.uData);
			name = old.name;
			vbuffer = old.vbuffer;
			ibuffer = old.ibuffer;
			uvbuffer = old.uvbuffer;
			uibuffer = old.uibuffer;
			vbuffView = old.vbuffView;
			ibuffView = old.ibuffView;
			clBoneBuff = old.clBoneBuff;
			clBuff = old.clBuff;
			clIndexBuff = old.clIndexBuff;
			clIndexMap = old.clIndexMap;
		};
		
		*/
		UINT umID;
		std::unique_ptr<ReadXML> uData;
		std::string name;
		Microsoft::WRL::ComPtr <ID3D12Resource> vbuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> ibuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> uvbuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> uibuffer;
		D3D12_VERTEX_BUFFER_VIEW vbuffView;
		D3D12_INDEX_BUFFER_VIEW ibuffView;
		cl::Buffer clBoneBuff;
		cl::Buffer clBuff;
		cl::Buffer clIndexBuff;
		cl::Buffer clIndexMap;
	};

	struct mData {
		UINT umID = 0;
		bmResource* bmData = nullptr;
		std::string name = "";
		UINT fsize = 0;
		UINT vCount = 0;
		std::filesystem::path model;
	};

	
	mData* GetModel(UINT umID);
	bmResource* loadModel(mData& umData);
	void trackModelID(UINT umID, UINT UOID);
	UINT getModelID(UINT UOID);
	std::filesystem::path getTexture(std::string name);


private:
	std::list<mData> AvailableModels;
	std::list<bmResource> LoadedModels;
	std::map<uint64_t, std::list<uint64_t>> trackedModels;
	std::unordered_map<uint64_t, UINT> tmodelIndexMap;
	std::vector<std::filesystem::path> Textures;
	std::vector<uint8_t> defaultTexture;
	GErrors::CheckerToken chk;
};