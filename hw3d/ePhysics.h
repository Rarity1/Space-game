#pragma once
#include "Graphics.h"
#include <DirectXCollision.h>


class Physics {
public:
	struct mThreadTime {
		float time = 0;
		std::mutex mtx;
	};
	
	struct eResource {
		//Enum model name
		std::string name = "";
		RStorage::bmResource* model = nullptr;
		float scale = 1;
		float mass = 1;
		float friction = 0;
		struct relposVect {
			XMFLOAT3 position = { 0,0,0 };
			XMFLOAT4 rotation = { 0,0,0,0 };
			XMFLOAT4 orbit = { 0,0,0,0 };
			XMFLOAT3 lastposition = { 0,0,0 };
			XMFLOAT3 lastrotation = { 1,0,0 };
			XMFLOAT3 lastorbit = { 1,0,0 };
			std::mutex posMtx;
		};
		relposVect mPos;
		float speed = 0;
		XMFLOAT4 grav{ 0,0,0,0 };
		float gravpull = 0;
		XMFLOAT4 velDir{0,0,0,0};
		XMFLOAT4 pDir{0,0,0,0};
		eResource* mworld = nullptr;
		bool isWorld = false;
		RStorage::pChange which = RStorage::INIT;
		struct tmCollide {
			eResource* ptModel;
			float distance;
			std::mutex mtx;
			std::atomic<bool> Collision = false;
			XMFLOAT4 direction{ 0,0,0,0 };
			XMFLOAT4 veldir{ 0,0,0,0 };
		};
		std::vector<tmCollide*> tmDist;
		std::mutex currentMtx;
		cl::Buffer clBuff;
		cl::Buffer clPositionBuff;
		cl::Buffer clBoneBuff;
		cl::Buffer clCollIndBuff;
		std::vector<ReadX3D::pCollision*> currentCollision;
		std::atomic<bool> updated = false;
		std::atomic<bool> Collision = false;
		std::mutex QueueMTX;
	};
	Physics(mThreadTime* timer, std::vector<Physics::eResource*>& trackedModels, int* UpdateRate);
	~Physics();
	void Update();
	float fDistance(XMFLOAT3* pos1, XMFLOAT3* pos2);
	DirectX::XMFLOAT3 AddXMFLOAT3(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b);
	std::atomic<bool> upDist = false;
	std::atomic<bool> Retracker = true;
private:
	
	void Retrack();
	template <typename T> int sgn(T val);
	std::vector<Physics::eResource*>& trackedModels;
	mThreadTime* timer;
	int* urate;
	float GConst = 0;
	void cGravity(eResource* obj);
	XMFLOAT4 fDirection(XMFLOAT3* pos1, XMFLOAT3* pos2);
	int bIndex(std::vector<int> w, int bInd);
	void trackDist(Physics::eResource* tModel);
	void uCone(Physics::eResource* obj, eResource::tmCollide* tmdist);
	bool coneCheck(XMFLOAT3 postc, float height, float radius, XMFLOAT4 dir);
	void ProcCollide(Physics::eResource* obj, eResource::tmCollide* tmdist);
	void pSpecCollison(eResource* obj);
	void pSpecReset(eResource* obj);
	std::vector<float> RayCastColl(std::vector<int>& index, std::vector<ReadX3D::pCollision>& cdata, XMFLOAT4& dir, std::vector<XMFLOAT3>& raypos);
	bool triCollide(ReadX3D::pCollision* tri, ReadX3D::pCollision* tri2, XMFLOAT4 dir, float dist);
	void mMove(Physics::eResource* mUpdate);
	std::vector<std::thread> collisionThreads;
	std::vector<std::thread> distanceThreads = {};

	struct RETURNDATA {
		bool coll;
		int index1[3];
		int index2[3];
		XMFLOAT4 dir[2];
		float dist[2];
	};
	struct WORKDATA {
		int bIndex[2];
		XMFLOAT3 Position[2];
	};

	struct OffsetC {
		int Offset[2];
		int ICount[2];
	};
	int WorkDataSize = 40;
	std::vector<cl::Device> devices;
	cl::Context context;
	cl::Program::Sources sources;
	cl::Program program;
	cl::CommandQueue queue;
	std::mutex QueueMTX;
	cl::Kernel collide;
};