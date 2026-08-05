#pragma once
#include "ModelData.h"
#include "RStorage.h"
#include "Threads.h"
#include <CL/opencl.hpp>


class Object;
class Tracker;
class Physics {
public:
	Physics(const int& UpdateRate, class Tracker& Tracker);
	~Physics();
	void Update();
	
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
	std::unique_ptr<THREADS> tMain;
  Tracker& Tracker;
	THREADS::WRef lastWref;
	unsigned int coreCount = 0;
	static float fDistance(FLOAT3& pos1, FLOAT3& pos2);
	static FLOAT4 fDirection(FLOAT3& pos1, FLOAT3& pos2);
	cl_ulong clLocalMemSize;
	struct collstruct {
		Object* obj = nullptr;
		Object* obj2 = nullptr;
		bool operator==(const collstruct& r) const
		{
			return (obj == r.obj && obj2 == r.obj2) || (obj == r.obj2 && obj2 == r.obj);
		}
	};

	std::vector<collstruct> CollModels;
	std::mutex cmMtx;
	EngineTime Clock;
	const int& urate;
	float GConst = 0;
	void cGravity(Object* obj);
	int bIndex(std::vector<int> w, int bInd);
	void CalProportionalSpeed(FLOAT4& VelDir1, FLOAT4& VelDir2, float& VSpeed1, float& VSpeed2, float& Mass1, float& Mass2);
	//Main Collision function
	void pCollison(umID instanceID);
	void pSpecReset();
  bool QueueCLBuffer(Object& obj);

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
		cl_uint index[2]{ 0,0 };
	};

	struct WORKINDI {
		cl_int wWorkCount = 0;
		cl_int tWorkCount = 0;
		FLOAT3 Position{ 0,0,0 };
		std::vector<cl_int> Indices{};
	};
	 WORKINDI ProcCollide(Object& obj, Object& obj2, FLOAT3& objpos, FLOAT3& obj2pos, FLOAT4& dir, float& dist);
  std::function<void()> CheckVertexDirection(std::vector<int> &Result,
    ModelData &objudat, std::vector<std::atomic<bool>> &IndexChecked,
    uint32_t &localWorkData, 
    SphereCollider &CollSp,
    std::vector<std::array<ModelData::Vertex, 3>> &Vertices);


  std::mutex phyxBusy;
	std::atomic<bool> Updated;
;

};