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
	};
	~Object() {};
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
		clPositionBuff(std::move(old.clPositionBuff)),
		pDir(std::move(old.pDir)),
		mPos(std::move(old.mPos))

	{
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
		clPositionBuff = std::move(old.clPositionBuff);
		pDir = std::move(old.pDir);
		mPos = std::move(old.mPos);
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



	 struct CBVData {
		 __declspec(align(16)) DirectX::XMFLOAT4X4 cbvMatrix;
		UINT Texture = 0;
		//do not use
		//UINT Padding[3];
	};
	//View Matrix
	DirectX::XMFLOAT4X4 vMatrix;
	std::mutex viewMtx;


	//upload view matrix.
	//Microsoft::WRL::ComPtr<ID3D12Resource> cbvwriteBuffer;
	//Instanced buffer specific to object for physics calculations
	cl::Buffer clPositionBuff;
	

};

class DLL Tracker {
	friend class Engine;
public: 
	struct InstanceStruc {
		//umID to list of objects using that model
		std::unordered_map<UINT, std::list<Object*>> tmodelLinkedObjects{};
		std::unordered_map<UINT, Object::CBVData*> instancedCBVData;
		Tracker* pTracker = nullptr;
		UINT Count;
	};
private:
	RStorage& storage;
	std::atomic<uint64_t> lastUOIDused = 0;
	//UMID, tracked objs using model

	//UOID to ptr storing that tracked object
	std::unordered_map<uint16_t, InstanceStruc> objectInstances;
	std::unordered_map<UINT, std::unique_ptr<Object>> mapUOID;
public:
	Tracker(RStorage& tstorage) :
		storage(tstorage)
	{
	};
	~Tracker() {
	};
	//Returns ID of current model given UOID(Unique Object ID)
	UINT getModelID(UINT UOID);
	RStorage::bmResource* GetModel(UINT umID);
	InstanceStruc& initInstance(uint16_t instanceID);
	InstanceStruc& getInstance(uint16_t instanceID);
	Object* initObject(InstanceStruc& oInstance, std::string textureName, UINT filebModelIndex = 0, float mScale = 1.0, float mMass = 0.0, float mFriction = 0.01, DirectX::XMFLOAT3 initPos = { 0,0,0 }, DirectX::XMFLOAT3 initRot = { 0,0,0 }, DirectX::XMFLOAT3 initVelDir = { 0,0,0 }, float initSpeed = 0);
	void unloadObject(uint64_t obj);
};