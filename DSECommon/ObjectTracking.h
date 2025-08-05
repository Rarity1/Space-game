#pragma once
#include "CWin.h"
#include "RStorage.h"




class DLL Object {
public:
	enum Type {
		DEFAULT,
		WORLD,
		SATELLITE,
		PROJECTILE,
		PLAYER
	};
	//Object* Parent;
	//std::vector<Object*> Child;
	uint64_t UOID = -1;

	struct relposVect {
		relposVect() :
			position(new DirectX::XMFLOAT3(0, 0, 0)) {
		};
		relposVect(DirectX::XMFLOAT3 initPos) :
			position(new DirectX::XMFLOAT3(initPos)) {
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
			return *this;
		}

		DirectX::XMFLOAT3* position;
		DirectX::XMFLOAT3 lastposition = { 0,0,0 };
		DirectX::XMFLOAT4 rotation{ 0,0,0,1 };
		std::mutex posMtx;
	};
	Object(std::string name, RStorage::bmResource* model, float mScale, float mMass, float mFriction, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 initRot, DirectX::XMFLOAT3 initVelDir, float initSpeed) :
		name(name),
		model(model),
		scale(mScale),
		mass(mMass),
		friction(mFriction),
		velDir(),
		speed(),
		mPos(relposVect(initPos))
	{
		ptrcbvData = std::addressof(cbvData);
	};
	~Object() {
	
		if (cbvwriteBuffer != nullptr) {
			cbvwriteBuffer->Unmap(0, nullptr);

		}
	};
	Object(Object&& old) noexcept :
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
		mPos(std::move(old.mPos)),
		cbvwriteBuffer(std::move(old.cbvwriteBuffer)),
		cbvData(std::move(old.cbvData))
	{
		ptrcbvData = std::addressof(cbvData);
	}
	Object& operator = (Object&& old) noexcept
	{
		model = std::move(old.model);
		name = std::move(old.name);
		scale = std::move(old.scale);
		mass = std::move(old.mass);
		friction = std::move(old.friction);
		velDir = std::move(old.velDir);
		speed = std::move(old.speed);
		grav = std::move(old.grav);
		gravpull = std::move(old.gravpull);
		mworld = std::move(old.mworld);
		curTexture = std::move(old.curTexture);
		clPositionBuff = std::move(old.clPositionBuff);
		pDir = std::move(old.pDir);
		tbuffer = std::move(old.tbuffer);
		mPos = std::move(old.mPos);
		cbvwriteBuffer = std::move(old.cbvwriteBuffer);
		cbvData = (old.cbvData);
		ptrcbvData = std::addressof(cbvData);
		return *this;


	};

	Object(const Object& old)
	{
		/*
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
		vMatrix = old.vMatrix;
		mPos = old.mPos;
		cbvwriteBuffer = old.cbvwriteBuffer;
		cbvMatrix = old.cbvMatrix;
		ptrCbvMatrix = std::addressof(cbvMatrix);
		*/

	}
	bool operator==(const Object& comparison) {
		if (this != &comparison) return false;
		if (clPositionBuff != comparison.clPositionBuff)return false;
		if (mPos.position != comparison.mPos.position) return false;
		return true;
	}
	bool operator!=(const Object& comparison) {
		if (this == &comparison) return false;
		if (clPositionBuff == comparison.clPositionBuff)return false;
		if (mPos.position == comparison.mPos.position) return false;
		return true;
	}
	Object& operator = (const Object& old) {
		return *this;
		/*
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
		vMatrix = old.vMatrix;
		mPos = old.mPos;
		cbvwriteBuffer = old.cbvwriteBuffer;
		cbvMatrix = old.cbvMatrix;
		ptrCbvMatrix = std::addressof(cbvMatrix);
		 
		 */

		//return *this;
	};
	std::string name = "";


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
	Object* mworld = nullptr;


	std::vector<DirectX::XMFLOAT4> pDir;
	void Move(DirectX::XMFLOAT4 Dir, float Dist = 1);
	//Returns previous rotation matrix
	DirectX::XMFLOAT4X4 Rotate(DirectX::XMFLOAT4X4& RotationMatrix);
	void CollReset();
	bool CollCheck();

	//Returns last position before update
	DirectX::XMFLOAT3 UpdatePosition();
	std::atomic<bool> updated;
	std::atomic<bool> Collision;
	std::mutex PhysicsUpdate;

	bool loadedModel = false;
	//Replace with shared ptr to prevent memory leak. Shared because models can be used more than once
	RStorage::bmResource* model;


	//Instanced texture path and buffer.
	std::filesystem::path curTexture;
	Microsoft::WRL::ComPtr<ID3D12Resource> tbuffer;

	struct CBVData {
		DirectX::XMFLOAT4X4 cbvMatrix;
		UINT Texture = 0;
	};
	CBVData cbvData;
	//View Matrix
	DirectX::XMFLOAT4X4 vMatrix;
	std::mutex viewMtx;

	CBVData* ptrcbvData;


	//upload view matrix.
	Microsoft::WRL::ComPtr<ID3D12Resource> cbvwriteBuffer;
	//Instanced buffer specific to object for physics calculations
	cl::Buffer clPositionBuff;
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGpuHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle;
};

class DLL Tracker {
	friend class Engine;
private:
	RStorage& storage;
	std::atomic<uint64_t> lastUOIDused = 0;
	//UMID, tracked objs using model
	std::map<uint32_t, std::list<Object*>> objCounter;
	std::vector<std::pair<uint64_t, bool>> UOIDlist;
	std::unordered_map<uint16_t, std::list<Object>> objectInstances;
	std::unordered_map<uint64_t, Object*> mapUOID;
public:
	Tracker(RStorage& tstorage) :
		storage(tstorage)
	{
	};
	~Tracker() = default;
	UINT getModelID(uint64_t oID);
	std::list<Object>& initInstance(uint16_t instanceID);
	std::list<Object>& getInstance(uint16_t instanceID);
	Object* initObject(std::list<Object>* oInstance, std::string textureName, UINT filebModelIndex = 0, float mScale = 1.0, float mMass = 0.0, float mFriction = 0.01, DirectX::XMFLOAT3 initPos = { 0,0,0 }, DirectX::XMFLOAT3 initRot = { 0,0,0 }, DirectX::XMFLOAT3 initVelDir = { 0,0,0 }, float initSpeed = 0);
	void unloadObject(uint64_t obj);
};