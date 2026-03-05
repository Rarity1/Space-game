#pragma once
#include "CWin.h"
#include "ModelData.h"
#include "GraphicsErrors.h"
#include "text.h"
#include <CL/opencl.hpp>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
//#include <dstorage.h>


class DLL RStorage {
	//friend class Object;
	//friend class Graphics;
public:
	RStorage();
	~RStorage();

	//Do NOT copy.
	struct bmResource {
		bmResource() = default;
		bmResource(UINT& uemID, ModelData euData, std::string& ename) {
			umID = uemID;
			uData = std::make_unique<ModelData>(std::move(euData));
			name = ename;
		};
		bmResource(const bmResource& old) noexcept {
			umID = old.umID;
			uData = std::make_unique<ModelData>(*old.uData);
			name = old.name;
			clIndexBuff = old.clIndexBuff;
			clIndexMap = old.clIndexMap;
		};
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
		~bmResource() = default;
		bmResource& operator=(const bmResource& old) {
			umID = old.umID;
			uData = std::make_unique<ModelData>(*old.uData);
			name = old.name;
			clIndexBuff = old.clIndexBuff;
			clIndexMap = old.clIndexMap;
			return *this;
		}

		UINT umID = 0;
		std::unique_ptr<ModelData> uData;
		std::string name;
		//Instanced texture path and buffer.
		std::filesystem::path curTexture;
		Microsoft::WRL::ComPtr<ID3D12Resource> tbuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> vbuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> ibuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> uvbuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> uibuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> cbvwriteBuffer;
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGpuHandle;
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle;
		D3D12_VERTEX_BUFFER_VIEW vbuffView;
		D3D12_INDEX_BUFFER_VIEW ibuffView;
		cl::Buffer clBoneBuff;
		cl::Buffer clBuff;
		cl::Buffer clIndexBuff;
		cl::Buffer clIndexMap;
	};

	struct mData {
		UINT umID = 0;
		bmResource* model = nullptr;
		std::string name = "";
		UINT fsize = 0;
		UINT vCount = 0;
		std::filesystem::path modelPath;
	};

	bmResource* GetModel(UINT umID);
	bmResource* loadModel(UINT umID);
	std::filesystem::path getTexture(std::string name);


private:
	std::unordered_map<UINT, mData> AvailableModels;
	std::unordered_map<UINT, bmResource> LoadedModels;
	std::unordered_map<uint64_t, UINT> tmodelIndexMap;
	std::vector<std::filesystem::path> Textures;
	std::vector<uint8_t> defaultTexture;
	GErrors::CheckerToken chk;
};