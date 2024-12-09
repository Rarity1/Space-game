#pragma once
#include "EngineTime.h"
#include "Graphics.h"


class Physics {
public:
	Physics(EngineTime& timer, std::vector<RStorage::eResource>& trackedModels, const int& UpdateRate);
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
	struct THREADS {
		THREADS()
		{
			lock = move(std::unique_lock<std::mutex>(cMutex));
			thread[0] = move(std::thread([this] {exeWork(); }));
			thread[1] = move(std::thread([this] {checkWork(); }));
			rlock = move(std::unique_lock<std::mutex>(rMutex));
			alock = move(std::unique_lock<std::mutex>(aMutex));
		}


		~THREADS() {
			tRunning = false;
			rVariable.notify_all();
			aVariable.notify_all();
			cVariable.notify_all();
			wMutex.lock();
			tWork.resize(0);
			wMutex.unlock();
			thread[0].join();
			thread[1].join();
			lock.release();
			rlock.release();
			alock.release();
		}
		void tPushWork(std::function<void()> f) {
			wMutex.lock();
			tWork.emplace_back(f);
			wMutex.unlock();
			cVariable.notify_one();
		};
		void tPushWork(std::vector<std::function<void()>> f) {
			wMutex.lock();
			tWork.append_range(f);
			wMutex.unlock();
			cVariable.notify_one();
		};


		static void tEndWork(std::vector<THREADS>& Threads) {
			for (auto& t : Threads) {
				t.wMutex.lock();
				if (!t.worked && t.tWork.size() != 0) {
					t.aVariable.notify_one();
					t.wMutex.unlock();
					t.rVariable.wait(t.rlock);
					t.wMutex.lock();
					t.worked = false;
					t.wMutex.unlock();
				}
				else {
					t.worked = false;
					t.wMutex.unlock();
					
				}
			}
		}


		void exeWork() {
			while (tRunning) {
				wMutex.lock();
				if (tWork.size() == 0) {
					wMutex.unlock();

					cVariable.wait(lock);
				}
				else {
					wMutex.unlock();
				}
				wMutex.lock();
				std::for_each(tWork.begin(), tWork.end(), [](auto& w) {
					w();
				});				
				tWork.resize(0);
				worked = true;
				aVariable.notify_one();
				wMutex.unlock();
			}
		};
		void checkWork() {
			while (tRunning) {
				aVariable.wait(alock);
				wMutex.lock();
				if (!worked && tWork.size() == 0 || worked && tWork.size() == 0) {
					wMutex.unlock();
					rVariable.notify_all();
				}
				else if (tWork.size() != 0) {
					wMutex.unlock();
					recurCheck();
				}
				else {
					wMutex.unlock();
				}
					
			}
		};
		void recurCheck() {
			aVariable.wait(alock);
			wMutex.lock();
			if (!worked && tWork.size() == 0 || worked && tWork.size() == 0) {
				wMutex.unlock();
				rVariable.notify_all();
			}
			else if(tWork.size() != 0) {
				wMutex.unlock();
				recurCheck();
			}
			else {
				wMutex.unlock();
			}
		}

	private:
		bool worked = false;
		std::atomic<bool> tRunning = true;
		std::thread thread[2];
		std::condition_variable cVariable;
		std::condition_variable rVariable;
		std::condition_variable aVariable;
		std::unique_lock<std::mutex> lock;
		std::unique_lock<std::mutex> rlock;
		std::unique_lock<std::mutex> alock;
		std::mutex cMutex;
		std::mutex wMutex;
		std::mutex rMutex;
		std::mutex aMutex;
		std::vector<std::function<void()>> tWork;
	};

	std::vector<collstruct> CollModels;
	std::mutex cmMtx;
	std::vector<RStorage::eResource>& trackedModels;
	EngineTime& timer;
	const int& urate;
	float GConst = 0;
	void cGravity(RStorage::eResource* obj);
	int bIndex(std::vector<int> w, int bInd);
	void CalProportionalSpeed(DirectX::XMFLOAT4& VelDir1, DirectX::XMFLOAT4& VelDir2, float& VSpeed1, float& VSpeed2, float& Mass1, float& Mass2);
	void pSpecCollison();
	void pSpecReset();

	static void mMove(std::vector<RStorage::eResource>& trackedModels);
	std::vector<std::thread> collisionThreads;
	std::vector<std::thread> distanceThreads = {};
	std::vector<THREADS> pThreads;

	std::vector<cl::Device> devices;
	cl::Context context;
	cl::Program::Sources sources;
	cl::Program program;
	cl::CommandQueue queue;
	std::mutex QueueMTX;
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
		DirectX::XMFLOAT3 Position{ 0,0,0 };
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
		std::vector<WORKDATA> WData;
		std::vector<int> Indices;
	};
	 WORKINDI ProcCollide(RStorage::eResource& obj, RStorage::eResource& obj2, cl::CommandQueue& tQueue, DirectX::XMFLOAT3& objpos, DirectX::XMFLOAT3& obj2pos, DirectX::XMFLOAT4& dir, float& dist);
	std::thread lastPhyxThread;
	std::mutex phyxBusy;
	std::atomic<bool> Updated;

};