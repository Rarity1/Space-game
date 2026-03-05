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
			position(std::make_unique<DirectX::XMFLOAT3>(0, 0, 0)) {
		};
		relposVect(DirectX::XMFLOAT3 initPos) :
			position(std::make_unique<DirectX::XMFLOAT3>(initPos)) {
		};
		~relposVect() = default;
		relposVect(relposVect&& old) noexcept {
			position = std::move(old.position);
			lastposition = std::move(old.lastposition);
			rotation = std::move(old.rotation);
		};
		relposVect(const relposVect& old) {
			position = std::make_unique<DirectX::XMFLOAT3>(*old.position);
			lastposition = old.lastposition;
			rotation = old.rotation;
		};
		relposVect& operator=(const relposVect& old) {
			*position = *old.position;
			lastposition = old.lastposition;
			rotation = old.rotation;
			return *this;
		}

		std::unique_ptr<DirectX::XMFLOAT3> position;
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
		mPos(relposVect(initPos))
	{
	};
	~Object() = default;
	bool operator==(const Object& comparison) {
		if (UOID != comparison.UOID) return false;
		return true;
	}
	bool operator!=(const Object& comparison) {
		if (UOID == comparison.UOID) return false;
		return true;
	}

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
	void CollReset();
	bool CollCheck();


	//Returns last position before update
	DirectX::XMFLOAT3 UpdatePosition();
	std::atomic<bool> updated;
	std::atomic<bool> Collision;
	std::mutex PhysicsUpdate;

	bool loadedModel = false;

	//This pointer is handled by rstorage
	RStorage::bmResource* model;


	UINT CBVIndex = 0;

	//Instanced buffer specific to object for physics calculations
	cl::Buffer clPositionBuff;
	

};

class DLL Tracker {
	friend class Engine;
public: 
	struct CBVData {
		__declspec(align(16)) DirectX::XMFLOAT4X4 cbvMatrix;
		UINT Texture = 0;
		//do not use
		//UINT Padding[3];
	};
	struct InstanceStruc {
		//umID to list of objects using that model
		std::unordered_map<UINT, std::list<Object*>> tmodelLinkedObjects{};
		std::unordered_map<UINT, CBVData*> instancedCBVData;
		Tracker* pTracker = nullptr;
		UINT Count;
	};
private:
	std::shared_ptr<RStorage> storage;
	std::atomic<uint64_t> lastUOIDused = 0;


	//UOID to ptr storing that tracked object
	std::unordered_map<uint16_t, InstanceStruc> objectInstances;
	std::unordered_map<UINT, std::unique_ptr<Object>> mapUOID;
public:
	class IDAllocator {
		uint64_t cIDCount = 0;
		std::vector<uint64_t> FreeIDs;
		std::unordered_map<uint64_t, bool> UsedIDs;
		std::mutex IDMutex;
	public:
		IDAllocator(uint64_t IDCount = 1200) :
			cIDCount(IDCount)
		{
			IDMutex.lock();
			cIDCount = cIDCount == 0 ? 1 : cIDCount;
			FreeIDs.reserve(cIDCount);
			FreeIDs.resize(cIDCount);
			std::iota(FreeIDs.begin(), FreeIDs.end(), 0);
			std::reverse(FreeIDs.begin(), FreeIDs.end());
			IDMutex.unlock();
		}
		~IDAllocator() = default;
		uint64_t AllocID() {
			IDMutex.lock();
			uint64_t Result = FreeIDs.back();
			FreeIDs.pop_back();
			if (FreeIDs.size() <= cIDCount * 0.5) {
				std::vector<uint64_t> extendIDs(cIDCount);
				std::iota(extendIDs.begin(), extendIDs.end(), cIDCount);
				std::reverse(extendIDs.begin(), extendIDs.end());
				extendIDs.append_range(FreeIDs);
				cIDCount = (cIDCount + cIDCount);
				FreeIDs = std::move(extendIDs);
			}
			UsedIDs[Result] = true;
			IDMutex.unlock();
			return Result;
		}
		void FreeID(uint64_t& ID) {
			IDMutex.lock();
			if (UsedIDs.find(ID) != UsedIDs.end()) {
				UsedIDs.erase(ID);
				FreeIDs.push_back(ID);
			}
			IDMutex.unlock();
		}
	};
	Tracker() :
		storage(std::make_shared<RStorage>()),
		idTracker(std::make_unique<Tracker::IDAllocator>())
	{
	};
	~Tracker() {
	};
	void lModel(Object& obj, UINT umID) noexcept;

	std::unique_ptr<IDAllocator> idTracker;
	//Returns ID of current model given UOID(Unique Object ID)
	RStorage::bmResource* GetModel(UINT umID);
	InstanceStruc& initInstance(uint16_t instanceID);
	InstanceStruc& getInstance(uint16_t instanceID);
	Object* initObject(InstanceStruc& oInstance, std::string textureName, UINT filebModelIndex = 0, float mScale = 1.0, float mMass = 0.0, float mFriction = 0.01, DirectX::XMFLOAT3 initPos = { 0,0,0 }, DirectX::XMFLOAT3 initRot = { 0,0,0 }, DirectX::XMFLOAT3 initVelDir = { 0,0,0 }, float initSpeed = 0);
	void unloadObject(uint64_t obj);
};