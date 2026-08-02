#pragma once
#include "EngineTime.h"
#include <functional>
#include <thread>
#include <condition_variable>


//Rewrite using futures
class THREADS {
	std::unique_ptr<THREADS> SubThreads;
	uint8_t DepthIndex = 0;

	struct werk {
		//The ID of the thread this work is a grandchild of. Should be 0 unless this is a subchild of current Threads instance.
		//std::vector<UINT> orderedParents;
		std::mutex uWorkMTX;
		std::function<void()> Function;
		bool Worked = true;
	};
	class THREAD {
    public:
    THREAD();
    ~THREAD();
		uint8_t tPushWork(std::function<void()> f);
		void exeWork();
		void checkWork(unsigned int uWid);
		std::thread::id tThreadID;
		std::atomic<uint8_t> lWaiting;
    std::atomic<bool> tRunning = true;
		EngineTime eTime;
		std::unique_lock<std::mutex> workLock;
		std::thread thread;
		std::condition_variable cVariable;
		std::condition_variable aVariable;
		std::condition_variable bVariable;
		std::array<uint8_t, 256> localQ;
		std::mutex tBusy;
		uint8_t wIndex = 0;
		uint8_t Counter;
		std::mutex cMut;
		//uWid.
		std::vector<uint8_t> Queue;
		std::atomic<uint8_t> qSize = 0;
		uint8_t lWorkCount = 0;
		//Local work storage
		std::array<werk, 256> tWork;
	};
  std::vector<std::unique_ptr<THREAD>> Threads;
	int tCount = 0;
	std::mutex lWorkMTX;
	static void recurBatch(const std::vector<std::function<void()>>& f, std::vector<std::function<void()>>::const_iterator& i);
  std::unique_ptr<std::unordered_map<std::thread::id, THREAD*>> IDMapPtr;
  std::unordered_map<std::thread::id, THREAD*>& MasterthreadIDMap;
	std::unordered_map<std::thread::id, THREAD*> threadIDMap;
  std::unordered_map<std::thread::id, THREAD*> SubThreadIDMap;
  std::unique_ptr<std::mutex> IDMutexPtr;
  std::mutex& IDMutex;
  std::mutex SubThreadsCreationLock;
public:
	struct WRef {
		uint8_t uWid = 0;
		uint8_t DepthIndex = 0;
		THREADS::THREAD* Worker = nullptr;
	};
	THREADS(int cCount);
  THREADS(int cCount, std::unordered_map<std::thread::id, THREAD*>& PIDMap, 
    std::unordered_map<std::thread::id, THREAD*>& MasterIDMap,
     std::mutex& Mutex, uint8_t DepthID);
	~THREADS();

	WRef gPushWork(std::function<void()> f);
	WRef gPushWork(std::vector<std::function<void()>> f);
	void gEndWork(WRef wref);
	void gEndWork(std::vector<WRef>& wref);
  private:
  WRef PushToSubThread(std::function<void()>& f);
};