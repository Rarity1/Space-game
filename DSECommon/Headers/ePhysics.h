#pragma once
#include "ModelData.h"
#include "ObjectTracking.h"
#include "RStorage.h"
#include "Threads.h"
#include <CL/opencl.hpp>
#include <cstddef>
#include <cstdint>


class Object;
class PhysicsObject;
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
	cl_ulong clLocalMemSize;
  public:
	struct CollFlag {
    CollFlag() = default;
    CollFlag(UOID obj, UOID obj2):obj(obj),obj2(obj2){};
    CollFlag(const CollFlag& old) = default;
    CollFlag(CollFlag&& old) = default;
    UOID obj;
    UOID obj2;
    size_t operator()(const CollFlag& old)const{
      return std::hash<uint64_t>()(obj) ^ std::hash<uint64_t>()(obj2); 
    };
    inline bool operator==(const CollFlag &r) const {
      return (obj == r.obj && obj2 == r.obj2) ||
             (obj == r.obj2 && obj2 == r.obj);
    }
	};
  private:
  Exceptions::CheckerToken chk;
  std::unordered_map<CollFlag, std::atomic<bool>, CollFlag> CollModels;
	std::mutex cmMtx;
	EngineTime Clock;
	const int& urate;
	float GConst = 0;
	void cGravity(PhysicsObject* obj);
	int bIndex(std::vector<int> w, int bInd);
	void CalProportionalSpeed(FLOAT4& VelDir1, FLOAT4& VelDir2, float& VSpeed1, float& VSpeed2, float& Mass1, float& Mass2);
	//Main Collision function
	void pCollison();
  void ClearCollisionQueue();
  bool QueueCLBuffer(PhysicsObject* obj);

	std::vector<std::thread> collisionThreads;
	std::vector<std::thread> distanceThreads = {};
	std::vector<cl::Device> devices;
	cl::Context context;
	cl::Program FullColl;
	cl::Program Sphere;
	cl::CommandQueue UploadQueue;
	std::mutex QueueMTX;
	std::mutex DebugMTX;
	struct RETURNDATA {
		cl_float dist[2]{0.0,0.0};
		cl_uint index[2]{ 0,0 };
	};

	struct WORKINDI {
		cl_uint wWorkCount = 0;
		cl_uint tWorkCount = 0;
		FLOAT3 Position{ 0,0,0 };
		std::vector<cl_uint> Indices{};
	};
	 WORKINDI ProcCollide(PhysicsObject& obj, PhysicsObject& obj2);
  std::function<void()> CheckVertexDirection(std::vector<uint32_t> &Result,
    ModelData &objudat, std::vector<std::atomic<bool>> &IndexChecked,
    uint32_t &localWorkData, 
    SphereCollider &CollSp,
    std::vector<std::array<Vertex, 3>> &Vertices);


  std::mutex phyxBusy;
	std::atomic<bool> Updated;
};

