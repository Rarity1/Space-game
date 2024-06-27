#pragma once
#include "Graphics.h"


class Physics {
public:
	struct mThreadTime {
		float time = 0;
		std::mutex mtx;
	};
	


	Physics(mThreadTime& timer, std::vector<RStorage::eResource*>& trackedModels, int& UpdateRate);
	~Physics();
	void Update();
	float fDistance(DirectX::XMFLOAT3* pos1, DirectX::XMFLOAT3* pos2);
	DirectX::XMFLOAT3 AddXMFLOAT3(DirectX::XMFLOAT3& a, DirectX::XMFLOAT3& b);
	std::atomic<bool> Retracker = true;
private:
	cl_ulong clLocalMemSize;
	std::vector<std::atomic<bool>*> tmDist;
	void Retrack();
	template <typename T> int sgn(T val);
	std::vector<RStorage::eResource*>& trackedModels;
	mThreadTime& timer;
	int& urate;
	float GConst = 0;
	void cGravity(RStorage::eResource* obj);
	DirectX::XMFLOAT4 fDirection(DirectX::XMFLOAT3* pos1, DirectX::XMFLOAT3* pos2);
	int bIndex(std::vector<int> w, int bInd);
	void CalProportionalSpeed(DirectX::XMFLOAT4& VelDir1, DirectX::XMFLOAT4& VelDir2, float& VSpeed1, float& VSpeed2, float& Mass1, float& Mass2);
	std::vector<void *> ProcCollide(RStorage::eResource* obj, int tmindex, cl::CommandQueue& tQueue, cl::Buffer*& ReturnBuff, cl::Buffer*& WorkBuff, cl::Buffer*& IndBuff);
	void pSpecCollison(RStorage::eResource* obj);
	void pSpecReset();

	void mMove(RStorage::eResource* mUpdate);
	std::vector<std::thread> collisionThreads;
	std::vector<std::thread> distanceThreads = {};

	struct RETURNDATA {
		bool coll;
		int index1[3];
		int index2[3];
		DirectX::XMFLOAT4 dir[2];
		float dist[2];
	};
	struct float3 {
		float x;
		float y;
		float z;
	};
	struct WORKDATA {
		int bIndex[2];
		DirectX::XMFLOAT3 Position{0,0,0};
		int ICount[2];
		int offset[2];
	};

	bool VectThreadCheck(std::vector<int>& BoolV);

	//void OpenWorkUpload(Physics::WORKDATA& WData, OffsetC& offset, RStorage::eResource*& obj, RStorage::eResource*& obj2);

	//int WorkDataSize = 40;
	std::vector<cl::Device> devices;
	cl::Context context;
	cl::Program::Sources sources;
	cl::Program program;
	cl::CommandQueue queue;
	std::mutex QueueMTX;
};