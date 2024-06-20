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
		ID3D12Resource* uvbuffer;
	};
	struct bmResource {
		UINT umID;
		ReadX3D* uData;
		std::string name;
		ID3D12Resource* vbuffer;
		ID3D12Resource* ibuffer;
		ID3D12Resource* tbuffer;
		ID3D12Resource* uvbuffer;
		ID3D12Resource* uibuffer;
		DirectX::XMMATRIX cmatrix;
		std::atomic<bool> animate = false;
	};

	struct eResource {
		//Enum model name
		std::string name = "";
		RStorage::bmResource* model = nullptr;
		float scale = 1;
		float mass = 1;
		float friction = 0;
		struct relposVect {
			DirectX::XMFLOAT3* position = nullptr;
			DirectX::XMFLOAT4 rotation = { 0,0,0,0 };
			DirectX::XMFLOAT3 lastposition = { 0,0,0 };
			std::mutex posMtx;
		};
		relposVect mPos;
		DirectX::XMFLOAT4 velDir{ 0,0,0,0 };
		float speed = 0;
		DirectX::XMFLOAT4 grav{ 0,0,0,0 };
		float gravpull = 0;
		DirectX::XMFLOAT4 pDir{ 0,0,0,0 };
		float pspeed = 0;
		eResource* mworld = nullptr;
		RStorage::pChange which = RStorage::INIT;
		std::mutex currentMtx;
		cl::Buffer clBuff;
		cl::Buffer clPositionBuff;
		cl::Buffer clBoneBuff;
		cl::Buffer clCollIndBuff;
		//std::vector<ReadX3D::pCollision*> currentCollision;
		std::atomic<bool> updated = false;
		std::atomic<bool> Collision = false;
	};

	//Always call after read
	std::vector<RStorage::eResource*> initializedModels;
	RStorage::bmResource* lModel(UINT umID) noexcept;

 	RStorage::eResource* initResource(char name, int filebModelIndex = 0, float mScale = 1.0, float mMass = 0.0, float mFriction = 0.01, DirectX::XMFLOAT3 initPos = {0,0,0}, DirectX::XMFLOAT3 initRot = {0,0,0}, DirectX::XMFLOAT3 initVelDir = {0,0,0}, float initSpeed = 0);
	void CreateBuffers(std::vector<eResource*>& bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,  UINT buffercount);
	void UpdBuffer(eResource* bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);


private:
	ReadX3D* CheckLoaded(int umID);
	std::vector<ID3D12Resource*> vbufferPtrs;
	std::vector<unmappedData> Models;
	std::vector<ReadX3D*> loadedModels;
	std::vector<RStorage::bmResource*> TrackedPtrs;
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
	
};