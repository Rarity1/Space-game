#pragma once
#include "CWin.h"
#include "X3DInt.h"
#include "GraphicsErrors.h"


class RStorage {
public:
	RStorage();
	virtual void OnInit();
	~RStorage();
	enum pChange {
		ALL = 0,
		ONE = 1
	};
	struct unmappedData {
		std::filesystem::path model;
		UINT fsize;
		UINT vCount;
		UINT umID;
		ReadX3D* lModel = nullptr;
	};
	unmappedData* findUm(UINT umID);
	struct bmResource {
		UINT umID;
		ReadX3D* uData;
		std::string name;
		ID3D12Resource* vbuffer = nullptr;
		ID3D12Resource* ibuffer = nullptr;
		ID3D12Resource* tbuffer = nullptr;
		ID3D12Resource* uvbuffer = nullptr;
		ID3D12Resource* uibuffer = nullptr;
		cl::Buffer clBoneBuff;
		cl::Buffer clBuff;
		cl::Buffer clIndexBuff;
		std::atomic<bool> buffersWritten;
		DirectX::XMMATRIX cmatrix;
		std::atomic<bool> animate = false;
	};

	struct relposVect {
		relposVect(DirectX::XMFLOAT3 initPos);
		~relposVect();
		DirectX::XMFLOAT3* position = nullptr;
		DirectX::XMFLOAT4 rotation = { 0,0,0,0 };
		DirectX::XMFLOAT3 lastposition = { 0,0,0 };
		std::mutex& posMtx;
	};

	struct eResource {
		eResource(std::string name, RStorage::bmResource* model, float mScale, float mMass, float mFriction, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 initRot, DirectX::XMFLOAT3 initVelDir, float initSpeed);
		~eResource() {
			delete mPos;
			delete& currentMtx;
			delete& PhysicsUpdate;
			delete& Filled;
			delete& updated;
			delete& Collision;
		}
		eResource(const eResource& old) :
			currentMtx(*new std::mutex),
			PhysicsUpdate(*new std::mutex),
			Filled(*new std::atomic<bool>),
			updated(*new std::atomic<bool>),
			Collision(*new std::atomic<bool>),
			mPos(new relposVect(*old.mPos->position))
		{
			model = old.model;
			name = old.name;
			scale = old.scale;
			mass = old.mass;
			friction = old.friction;
			velDir = old.velDir;
			speed = old.speed;
			grav = old.grav;
			gravpull = old.gravpull;
			mworld = old.mworld;
			curTexture = old.curTexture;
			clPositionBuff = old.clPositionBuff;
			pDir = old.pDir;
		}
		eResource& operator=(const eResource& old) {
			if (this == &old)
				return *this;
			name = old.name;
			scale = old.scale;
			mass = old.mass;
			friction = old.friction;
			velDir = old.velDir;
			speed = old.speed;
			grav = old.grav;
			gravpull = old.gravpull;
			mworld = old.mworld;
			curTexture = old.curTexture;
			clPositionBuff = old.clPositionBuff;
			pDir = old.pDir;
			model = old.model;
			mPos->position = old.mPos->position;
		}
		std::string name = "";
		RStorage::bmResource* model;
		float scale = 1;
		float mass = 1;
		float friction = 0;
		relposVect* mPos;
		DirectX::XMFLOAT4 velDir{ 0,0,0,0 };
		float speed = 0;
		DirectX::XMFLOAT4 grav{ 0,0,0,0 };
		float gravpull = 0;
		eResource* mworld = nullptr;
		std::filesystem::path curTexture;
		cl::Buffer clPositionBuff;
		std::vector<DirectX::XMFLOAT4> pDir;
		void CollisionUp(DirectX::XMFLOAT4 Dir, float Dist = 1);
		void CollReset();
		bool CollCheck();
		DirectX::XMFLOAT4 CollDir(pChange Which = ALL, int Index = 0);
		std::atomic<bool>& updated;
		std::atomic<bool>& Collision;
		std::mutex& currentMtx;
		std::mutex& PhysicsUpdate;
		std::atomic<bool>& Filled;

	};

	std::vector<RStorage::eResource> initializedModels;
	RStorage::bmResource* lModel(UINT umID) noexcept;
 	RStorage::eResource* initResource(std::string name, int filebModelIndex = 0, float mScale = 1.0, float mMass = 0.0, float mFriction = 0.01, DirectX::XMFLOAT3 initPos = {0,0,0}, DirectX::XMFLOAT3 initRot = {0,0,0}, DirectX::XMFLOAT3 initVelDir = {0,0,0}, float initSpeed = 0);
	void CreateBuffers(std::vector<eResource>& bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,  UINT buffercount);
	void UpdBuffer(eResource& bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);
	ReadX3D* CheckLoaded(int umID);


private:
	std::vector<unmappedData*> Models;
	std::vector<std::filesystem::path> Textures;
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