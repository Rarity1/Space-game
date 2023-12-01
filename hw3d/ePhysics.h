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
		//Enum model name add time.
		UINT umID = 0;
		std::string name = "";
		bool isWorld = false;
		float mass = 1;
		Graphics::rpVect loadedModel;
		struct tmCollide {
			eResource* ptModel;
			float distance;
			std::mutex mtx;
			std::atomic<bool> Collision = false;
			XMFLOAT4 direction{ 0,0,0,0 };
		};
		std::vector<tmCollide*> tmDist;
		std::mutex currentMtx;
		std::vector<ReadX3D::pCollision*> currentCollision;
		float speed;
		XMFLOAT4 grav{ 0,0,0,0 };
		float gravpull;
		XMFLOAT4 velDir;
		float scale = 1;
		eResource* mworld = nullptr;
		float friction = 0;
		std::atomic<bool> updated = false;
	};
	Physics(mThreadTime* timer, std::vector<Physics::eResource*>& trackedModels, int* UpdateRate);
	~Physics();
	void Update();
	float fDistance(XMFLOAT3* pos1, XMFLOAT3* pos2);
	DirectX::XMFLOAT3 AddXMFLOAT3(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b);
	std::atomic<bool> upDist = false;
private:
	std::vector<Physics::eResource*>& trackedModels;
	mThreadTime* timer;
	int* urate;
	float GConst = 0;
	void cGravity(eResource* obj);
	void trackDist(Physics::eResource* tModel);
	void pSpecCollison(eResource* obj);
	void RayCastColl(ReadX3D::pCollision* pos, XMFLOAT3* apos, Physics::eResource* obj, XMFLOAT4* dir);
	bool triCollide(ReadX3D::pCollision* tri, ReadX3D::pCollision* tri2, XMFLOAT3* tripos, XMFLOAT3* tri2pos);
	void mMove(Physics::eResource* mUpdate);
	std::vector<std::thread> collisionThreads;
};