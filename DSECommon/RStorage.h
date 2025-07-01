#pragma once
#include "CWin.h"
#include "X3DInt.h"
#include "GraphicsErrors.h"
#include "text.h"
#include <CL/opencl.hpp>


class DLL RStorage {
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
		relposVect():
			position(new DirectX::XMFLOAT3(0,0,0)){
		};
		relposVect(DirectX::XMFLOAT3 initPos):
		position(new DirectX::XMFLOAT3(initPos)){
		};
		~relposVect() { delete position; };
		relposVect(relposVect&& old) noexcept {
			position = new DirectX::XMFLOAT3(*old.position);
			lastposition = std::move(old.lastposition);
			rotation = std::move(old.rotation);
		};
		relposVect(const relposVect& old) {
			position = new DirectX::XMFLOAT3(*old.position);
			lastposition = old.lastposition;
			rotation = old.rotation;
		};
		relposVect& operator=(const relposVect& old) {
			*position = *old.position;
			lastposition = old.lastposition;
			rotation = old.rotation;
		}

		DirectX::XMFLOAT3* position;
		DirectX::XMFLOAT3 lastposition = { 0,0,0 };
		DirectX::XMFLOAT4 rotation{0,0,0,1};
		std::mutex posMtx;
	};

	struct eResource {
		eResource(std::string name, RStorage::bmResource* model, float mScale, float mMass, float mFriction, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 initRot, DirectX::XMFLOAT3 initVelDir, float initSpeed) :
			name(name),
			model(model),
			scale(mScale),
			mass(mMass),
			friction(mFriction),
			velDir(),
			speed(),
			mPos(relposVect(initPos))
		{
		};
		~eResource() {};
		eResource(eResource&& old) noexcept :
		model(std::move(old.model)),
		name(std::move(old.name)),
		scale(std::move(old.scale)),
		mass(std::move(old.mass)),
		friction(std::move(old.friction)),
		velDir(std::move(old.velDir)),
		speed(std::move(old.speed)),
		grav(std::move(old.grav)),
		gravpull(std::move(old.gravpull)),
		mworld(std::move(old.mworld)),
		curTexture(std::move(old.curTexture)),
		clPositionBuff(std::move(old.clPositionBuff)),
		pDir(std::move(old.pDir)),
		tbuffer(std::move(old.tbuffer)),
		cmatrix(std::move(old.cmatrix)),
		mPos(std::move(old.mPos))
		{}
		eResource(const eResource& old)
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
			mPos = old.mPos;
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

		//Replace with uniqueptr to prevent memory leak
		RStorage::bmResource* model;
		

		//All of this data could probably be stored better. Maybe a struct?
		float scale = 1;
		float mass = 1;
		float friction = 0;
		relposVect mPos;
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
		std::atomic<bool> updated;
		std::atomic<bool> Collision;
		std::mutex currentMtx;
		std::mutex PhysicsUpdate;


		//Instanced texture path and buffer.
		std::filesystem::path curTexture;
		Microsoft::WRL::ComPtr <ID3D12Resource> tbuffer;

		//View Matrix
		DirectX::XMFLOAT4X4 cmatrix;

		//Instanced buffer specific to object for physics calculations
		cl::Buffer clPositionBuff;
	};

	std::unique_ptr<std::vector<RStorage::eResource>> trackedObjects;
	
	RStorage::bmResource* lModel(UINT umID) noexcept;
 	RStorage::eResource* initObject(std::string textureName, int filebModelIndex = 0, float mScale = 1.0, float mMass = 0.0, float mFriction = 0.01, DirectX::XMFLOAT3 initPos = {0,0,0}, DirectX::XMFLOAT3 initRot = {0,0,0}, DirectX::XMFLOAT3 initVelDir = {0,0,0}, float initSpeed = 0);
	void CreateBuffers(std::vector<eResource>& bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,  UINT buffercount);
	void UpdBuffer(eResource& bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);
	ReadX3D* CheckLoaded(int umID);



private:
	std::vector<unmappedData*> Models;
	std::vector<std::filesystem::path> Textures;
	std::vector<uint8_t> defaultTexture;
	GErrors::CheckerToken chk;
};