#pragma once
#include "Graphics.h"
#include <DirectXCollision.h>


class Physics {
public:
	struct mThreadTime {
		float time = 0;
		std::mutex mtx;
	};
	


	Physics(mThreadTime& timer, std::vector<RStorage::eResource*>& trackedModels, int& UpdateRate);
	~Physics();
	void Update();
	float fDistance(XMFLOAT3* pos1, XMFLOAT3* pos2);
	DirectX::XMFLOAT3 AddXMFLOAT3(DirectX::XMFLOAT3& a, DirectX::XMFLOAT3& b);
	std::atomic<bool> Retracker = true;
private:
	std::vector<std::atomic<bool>*> tmDist;
	void Retrack();
	template <typename T> int sgn(T val);
	std::vector<RStorage::eResource*>& trackedModels;
	mThreadTime& timer;
	int& urate;
	float GConst = 0;
	void cGravity(RStorage::eResource* obj);
	XMFLOAT4 fDirection(XMFLOAT3* pos1, XMFLOAT3* pos2);
	int bIndex(std::vector<int> w, int bInd);
	void CalProportionalSpeed(XMFLOAT4& VelDir1, XMFLOAT4& VelDir2, float& VSpeed1, float& VSpeed2, float& Mass1, float& Mass2);
	void ProcCollide(RStorage::eResource* obj, int tmindex);
	void pSpecCollison(RStorage::eResource* obj);
	void pSpecReset();
	void mMove(RStorage::eResource* mUpdate);
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