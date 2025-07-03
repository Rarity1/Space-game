#pragma once
#include "RStorage.h"
#include "Threads.h"
#include <CL/opencl.hpp>


class DLL Physics {
public:
	Physics(EngineTime& timer, std::vector<RStorage::eResource>& trackedObjects, const int& UpdateRate);
	~Physics();
	void Update();
	//Call if loaded models/tracked models changes
	void trackM();
	struct tpsCounter {
	public:
		tpsCounter() {
			count = 0;
		}
		void incCount(short int ammount = 1) {
			count += ammount;
		}
		void reset() {
			count = 0;
		}
		short int cGet() {
			return count;
		}
	private:
		std::atomic<short int> count;
	};
	tpsCounter ticker;
private:
	std::unique_ptr<THREADS> thrds;
	std::unique_ptr<THREADS> childthrds;
	std::unique_ptr<THREADS> subchildthrds;

	THREADS::WRef lastWref;
	unsigned int coreCount = 0;
	static float fDistance(DirectX::XMFLOAT3& pos1, DirectX::XMFLOAT3& pos2);
	static DirectX::XMFLOAT4 fDirection(DirectX::XMFLOAT3& pos1, DirectX::XMFLOAT3& pos2);
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
	std::vector<RStorage::eResource>& trackedObjects;
	EngineTime& timer;
	const int& urate;
	float GConst = 0;
	void cGravity(RStorage::eResource* obj);
	int bIndex(std::vector<int> w, int bInd);
	void CalProportionalSpeed(DirectX::XMFLOAT4& VelDir1, DirectX::XMFLOAT4& VelDir2, float& VSpeed1, float& VSpeed2, float& Mass1, float& Mass2);
	void pSpecCollison();
	void pSpecReset();

	std::vector<std::thread> collisionThreads;
	std::vector<std::thread> distanceThreads = {};
	std::vector<cl::Device> devices;
	cl::Context context;
	cl::Program FullColl;
	cl::Program Sphere;
	cl::CommandQueue queue;
	std::mutex QueueMTX;
	std::mutex DebugMTX;
	struct RETURNDATA {
		cl_float dist[2]{0.0,0.0};
		cl_int index[2]{ 0,0 };
	};

	struct WORKINDI {
		cl_int wWorkCount = 0;
		cl_int tWorkCount = 0;
		DirectX::XMFLOAT3 Position{ 0,0,0 };
		std::vector<cl_int> Indices{};
	};
	 WORKINDI ProcCollide(RStorage::eResource& obj, RStorage::eResource& obj2, DirectX::XMFLOAT3& objpos, DirectX::XMFLOAT3& obj2pos, DirectX::XMFLOAT4& dir, float& dist);
	std::mutex phyxBusy;
	std::atomic<bool> Updated;
;

};