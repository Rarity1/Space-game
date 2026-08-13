#pragma once
#include "EngineTime.h"
#include <cassert>
#include <cstdint>
#include <functional>
#include <numeric>
#include <thread>
#include <condition_variable>


//Rewrite using futures
class THREADS {
	std::unique_ptr<THREADS> SubThreads;
	uint8_t DepthIndex = 0;

	struct werk {
    werk():uWorkMTX(std::mutex()),Worked(true){};
		//The ID of the thread this work is a grandchild of. Should be 0 unless this is a subchild of current Threads instance.
		//std::vector<UINT> orderedParents;
		std::mutex uWorkMTX;
		std::function<void()> Function;
		std::atomic<bool> Worked = true;
	};
	class THREAD {
    friend class THREADS;
    uint8_t GetFree();

		uint8_t tPushWork(std::function<void()> f);
		void exeWork();
		void checkWork(uint8_t uWid);
		std::thread::id tThreadID;
		std::atomic<uint8_t> lWaiting;
    std::atomic<bool> tRunning = true;
		EngineTime eTime;
		std::unique_lock<std::mutex> workLock;
		std::thread thread;
		std::condition_variable cVariable;
		std::condition_variable aVariable;
		std::condition_variable bVariable;
		std::mutex tBusy;
		uint8_t Counter=0;
		std::mutex cMut;
		//uWid.
		std::array<uint8_t, 256> Queue;
		std::atomic<uint8_t> qSize = 0;

		//Local work storage
		std::array<werk, 256> tWork;
      class IDAllocator {
    std::vector<uint8_t> FreeIDs;
    std::unordered_map<uint8_t, bool> UsedIDs;
    std::mutex IDMutex;

  public:
    IDAllocator(){
      IDMutex.lock();
      FreeIDs.resize(256);
      std::iota(FreeIDs.begin(), FreeIDs.end(), 0);
      std::reverse(FreeIDs.begin(), FreeIDs.end());
      IDMutex.unlock();
    }
    ~IDAllocator() = default;
    uint8_t AllocID() {
      IDMutex.lock();
      uint8_t Result = FreeIDs.back();
      FreeIDs.pop_back();
      UsedIDs[Result] = true;
      IDMutex.unlock();
      return Result;
    }
    void FreeID(uint8_t &ID) {
      IDMutex.lock();
      if(UsedIDs[ID]){
        UsedIDs[ID] = false;
        FreeIDs.push_back(ID);
      }
      IDMutex.unlock();
    }
    uint16_t FreeCount() {
      IDMutex.lock();
      uint16_t result = FreeIDs.size();
      IDMutex.unlock();
      return result;
    }
  };
  IDAllocator IDs;

public:
  THREAD();
  ~THREAD();
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
  WRef commonPush(std::function<void()>& f);
	WRef gPushWork(std::function<void()> f);
	WRef gPushWork(std::vector<std::function<void()>> f);
	void gEndWork(WRef wref);
	void gEndWork(std::vector<WRef>& wref);
  private:
  WRef PushToSubThread(std::function<void()>& f);

};