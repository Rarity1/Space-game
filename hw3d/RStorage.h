#pragma once
#include "CWin.h"
#include "X3DInt.h"
#include "GraphicsErrors.h"


class RStorage {
public:
	RStorage();
	~RStorage();
	enum pChange {
		ALL = 0,
		ONE = 1
	};
	struct unmappedData {
		std::filesystem::path model;
		std::string name = "";
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
		Microsoft::WRL::ComPtr < ID3D12Resource> vbuffer;
		Microsoft::WRL::ComPtr < ID3D12Resource> ibuffer;
		Microsoft::WRL::ComPtr < ID3D12Resource> uvbuffer;
		Microsoft::WRL::ComPtr < ID3D12Resource> uibuffer;
		cl::Buffer clBoneBuff;
		cl::Buffer clBuff;
		cl::Buffer clIndexBuff;
		cl::Buffer clIndexMap;
		std::atomic<bool> buffersWritten;
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
			delete& mPos;
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
			mPos(*new relposVect(*old.mPos.position))
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
			tbuffer = old.tbuffer;
			cmatrix = old.cmatrix;
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
			mPos.position = old.mPos.position;
			cmatrix = old.cmatrix;
		}
		bool operator==(const eResource& comparison) {
			if (this != &comparison) return false;
			if(clPositionBuff != comparison.clPositionBuff)return false;
			if(mPos.position != comparison.mPos.position) return false;
			return true;
		}
		bool operator!=(const eResource& comparison) {
			if (this == &comparison) return false;
			if (clPositionBuff == comparison.clPositionBuff)return false;
			if (mPos.position == comparison.mPos.position) return false;
			return true;
		}
		std::string name = "";

		//Model loaded into memory when this isnt nullptr
		RStorage::bmResource* model;
		

		//All of this data could probably be stored better. Maybe a struct?
		float scale = 1;
		float mass = 1;
		float friction = 0;
		relposVect& mPos;
		DirectX::XMFLOAT4 velDir{ 0,0,0,0 };
		float speed = 0;
		DirectX::XMFLOAT4 grav{ 0,0,0,0 };
		float gravpull = 0;

		//Only important for gravity/ loading reasons. Dont impliment until necessary
		eResource* mworld = nullptr;

		
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

		//Instanced texture path and buffer.
		std::filesystem::path curTexture;
		Microsoft::WRL::ComPtr <ID3D12Resource> tbuffer;
		//View Matrix
		DirectX::XMMATRIX cmatrix;

		//Instanced buffer specific to object for physics calculations
		cl::Buffer clPositionBuff;
	};

	std::vector<RStorage::eResource> trackedObjects;
	std::unordered_map<void *, RStorage::bmResource*> loadedModels;

	RStorage::bmResource* lModel(UINT umID) noexcept;
 	RStorage::eResource* initObject(std::string textureName, int filebModelIndex = 0, float mScale = 1.0, float mMass = 0.0, float mFriction = 0.01, DirectX::XMFLOAT3 initPos = {0,0,0}, DirectX::XMFLOAT3 initRot = {0,0,0}, DirectX::XMFLOAT3 initVelDir = {0,0,0}, float initSpeed = 0);
	void CreateBuffers(std::vector<eResource>& bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,  UINT buffercount);
	void UpdBuffer(eResource& bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);
	ReadX3D* CheckLoaded(int umID);


private:
	std::vector<unmappedData*> Models;
	std::vector<std::filesystem::path> Textures;



	GErrors::CheckerToken chk;
	
};