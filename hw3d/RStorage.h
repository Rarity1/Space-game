#pragma once
#include "CWin.h"
#include "X3DInt.h"
#include "GraphicsErrors.h"


class RStorage {
public:
	RStorage();
	virtual void OnInit();
	RStorage(const RStorage&) = delete;
	RStorage& operator=(const RStorage&) = delete;
	~RStorage();
	enum pChange {
		NONE = 0,
		ORBIT = 1,
		POSITION = 2,
		BOTH = 3,
		INIT = 4
	};
	struct aRotation {
		float pitch;
		float yaw;
		float roll;
	};
	struct unmappedData {
		std::filesystem::path texture;
		std::filesystem::path model;
		std::vector<ReadX3D::Vertex> vertexData;
		std::vector<ReadX3D::vFaceData> indexData;
		UINT fsize;
		UINT vCount;
		UINT umID;
		bool mappedBuffer;
		ID3D12Resource* vbuffer;
		ID3D12Resource* ibuffer;
		ID3D12Resource* tbuffer;
	};
	struct bmResource {
		std::string name;
		ID3D12Resource* vbuffer;
		ID3D12Resource* ibuffer;
		ID3D12Resource* tbuffer;
		ID3D12Resource* uvbuffer;
		ID3D12Resource* uibuffer;
		ReadX3D* uData;
		DirectX::XMMATRIX cmatrix;
		UINT umID;
	};
	std::vector<RStorage::bmResource*> modelVect;
	void Delete(RStorage::bmResource* bm);
	void lModel(UINT umID, RStorage::bmResource* model) noexcept;
	//Always call after read
	void CreateBuffers(std::vector<RStorage::bmResource*> bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,  UINT buffercount);
private:
	std::vector<ID3D12Resource*> vbufferPtrs;
	std::vector<unmappedData> Models;
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


	GErrors::CheckerToken chk;

	bmResource* cModel;
	
};