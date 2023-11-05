#pragma once
#include "CWin.h"
#include "X3DInt.h"
#include "GraphicsErrors.h"


class RStorage {
public:
	RStorage(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);
	virtual void OnInit();
	RStorage(const RStorage&) = delete;
	RStorage& operator=(const RStorage&) = delete;
	~RStorage();
	enum pChange {
		NONE = 0,
		ROTATION = 1,
		POSITION = 2,
		BOTH = 3
	};
	struct aRotation {
		float pitch;
		float yaw;
		float roll;
	};
	struct bmResource {
		std::string name;
		std::filesystem::path model;
		Microsoft::WRL::ComPtr<ID3D12Resource> vbuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> ibuffer;
		std::vector<ReadX3D::Vertex> vertexData;
		std::vector<ReadX3D::vFaceData> indexData;
		UINT fsize;
		UINT vCount;
		std::filesystem::path texture;
		Microsoft::WRL::ComPtr<ID3D12Resource> tbuffer;
		DirectX::XMFLOAT3 position = {0, 0, 0};
		aRotation rotation = {0, 0, 0};
		Microsoft::WRL::ComPtr<ID3D12Resource> uvbuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> uibuffer;
		pChange changepos = NONE;
		DirectX::XMMATRIX cmatrix;
	};

	RStorage::bmResource* Read(std::string name);
	void Delete(RStorage::bmResource* bm);
	RStorage::bmResource* lModel(std::string name) noexcept;
	//Always call after read
	void CreateBuffers(std::vector<RStorage::bmResource*> bm);
private:
	std::vector<bmResource> Models;
	struct DDS_HEADER {
		uint32_t dwSize;
		uint32_t dwFlags;
		uint32_t dwHeight;
		uint32_t dwWidth;
		uint32_t dwPitchOrLinearSize;
		uint32_t dwDepth;
		uint32_t dwMipMapCount;
		uint32_t dwReserved1[11];
		// ... other members are not shown for brevity
	};
	std::vector<RStorage::bmResource*> modelVect;
	RStorage::bmResource* UpCDStorage(RStorage::bmResource* bm);
	GErrors::CheckerToken chk;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;

	bmResource* cModel;
	
};