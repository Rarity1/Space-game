#pragma once
#include "Graphics.h"


class Physics {
public:
	struct mThreadTime {
		float time = 0;
		std::mutex mtx;
	};
	


	Physics(mThreadTime& timer, std::vector<RStorage::eResource>& trackedModels, int& UpdateRate);
	~Physics();
	void Update();
	float fDistance(DirectX::XMFLOAT3* pos1, DirectX::XMFLOAT3* pos2);
	std::atomic<bool> Retracker = true;
private:
	cl_ulong clLocalMemSize;
	struct collstruct {
		RStorage::eResource* obj = nullptr;
		RStorage::eResource* obj2 = nullptr;
		bool operator==(const collstruct& r) const
		{
			return (obj == r.obj && obj2 == r.obj2) || (obj == r.obj2 && obj2 == r.obj);
		}
	};

	std::vector<collstruct> CollModels;
	std::mutex cmMtx;
	void Retrack();
	std::vector<RStorage::eResource>& trackedModels;
	mThreadTime& timer;
	int& urate;
	float GConst = 0;
	void cGravity(RStorage::eResource* obj);
	DirectX::XMFLOAT4 fDirection(DirectX::XMFLOAT3* pos1, DirectX::XMFLOAT3* pos2);
	int bIndex(std::vector<int> w, int bInd);
	void CalProportionalSpeed(DirectX::XMFLOAT4& VelDir1, DirectX::XMFLOAT4& VelDir2, float& VSpeed1, float& VSpeed2, float& Mass1, float& Mass2);
	void pSpecCollison(RStorage::eResource& obj);
	void pSpecReset();

	void mMove(RStorage::eResource& mUpdate);
	std::vector<std::thread> collisionThreads;
	std::vector<std::thread> distanceThreads = {};

	struct RETURNDATA {
		bool coll;
		int index1[3];
		int index2[3];
		DirectX::XMFLOAT3 dir[2];
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
		int wWorkCount = 0;
		int tWorkCount = 0;
		int tOffset = 0;
	};
	struct UpVertNorm {
		DirectX::XMFLOAT3 Vert;
		DirectX::XMFLOAT3 Norm;
	};
	struct INTINDEX {
		int Index[3];
	};

	struct WORKINDI {
		std::vector<WORKDATA>* WData = nullptr;
		std::vector<int>* Indices = nullptr;
	};
	WORKINDI ProcCollide(RStorage::eResource& obj, RStorage::eResource& obj2, cl::CommandQueue& tQueue, cl::Buffer*& ReturnBuff, cl::Buffer*& WorkBuff, cl::Buffer*& IndBuff, DirectX::XMFLOAT3& objpos, DirectX::XMFLOAT3& obj2pos, DirectX::XMFLOAT4& dir, float& dist);

	std::vector<cl::Device> devices;
	cl::Context context;
	cl::Program::Sources sources;
	cl::Program program;
	cl::CommandQueue queue;
	std::mutex QueueMTX;
};