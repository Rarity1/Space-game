#include "Threads.h"
#include "Profiler.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>


//Add more depth when doing recursive functions. Depth is amount of thread recursion - 1
THREADS::THREADS(int cCount):
IDMapPtr(std::make_unique<std::unordered_map<std::thread::id, THREAD*>>()), MasterthreadIDMap(*IDMapPtr),
IDMutexPtr(std::make_unique<std::mutex>()), IDMutex(*IDMutexPtr) {
	tCount = cCount;
	for (int t = 0; t < tCount; t++) {
		Threads.emplace_back(std::make_unique<THREAD>());
		threadIDMap.insert({Threads[t]->thread.get_id(), Threads[t].get()});
	}
  MasterthreadIDMap.insert(threadIDMap.begin(), threadIDMap.end());
};

THREADS::THREADS(int cCount,
                 std::unordered_map<std::thread::id, THREAD *> &PIDMap,
                 std::unordered_map<std::thread::id, THREAD *> &MasterIDMap,
                 std::mutex &Mutex, uint8_t DepthID)
    : MasterthreadIDMap(MasterIDMap), IDMutex(Mutex) {
  tCount = cCount;
  DepthIndex = DepthID;
  for (int t = 0; t < tCount; t++) {
    Threads.emplace_back(std::make_unique<THREAD>());
    threadIDMap.insert({Threads[t]->thread.get_id(), Threads[t].get()});
  }
  IDMutex.lock();
  MasterIDMap.insert(threadIDMap.begin(), threadIDMap.end());
  PIDMap.insert(threadIDMap.begin(), threadIDMap.end());
  IDMutex.unlock();
};

THREADS::~THREADS() {
  threadIDMap.clear();
  Threads.clear();

}


 THREADS::WRef THREADS::gPushWork(std::function<void()> f)
{
  return PushToSubThread(f);
}

//Pushes batch of work onto single thread
 THREADS::WRef THREADS::gPushWork(std::vector<std::function<void()>> f)
{
	std::function<void()> recur([f]() {auto iter = f.begin(); recurBatch(f, iter); });
  return PushToSubThread(recur);
}

THREADS::WRef THREADS::commonPush(std::function<void()> &f) {
  WRef Result{0, DepthIndex};
  uint16_t twCount=0;
  uint16_t least = 0;
  uint8_t tInd = 0;
  bool valid = false;
  for (uint8_t i = 0; i < tCount; i++) {
    twCount = Threads[i]->qSize.load() + Threads[i]->lWaiting.load() + (256 - Threads[i]->IDs.FreeCount());
    if (twCount < least || !valid) {
      valid = true;
      least = twCount;
      tInd = i;
    }
  }
  if(least >= 255){
    SubThreadsCreationLock.lock();
      if (SubThreads) {
        SubThreadsCreationLock.unlock();
        return SubThreads->PushToSubThread(f);
      } else {
        SubThreads = std::make_unique<THREADS>(
            tCount, SubThreadIDMap, MasterthreadIDMap, IDMutex, DepthIndex + 1);
                  SubThreadsCreationLock.unlock();
        return SubThreads->PushToSubThread(f);
      }

  }
  Result.uWid = Threads[tInd]->tPushWork(std::move(f));
  Result.Worker = Threads[tInd].get();
  Threads[tInd]->cVariable.notify_one();
  return Result;
}

THREADS::WRef THREADS::PushToSubThread(std::function<void()> &f) {

  IDMutex.lock();
  bool containID = MasterthreadIDMap.contains(std::this_thread::get_id());
  IDMutex.unlock();
  if (!containID) {
    return commonPush(f);
  } else {
    IDMutex.lock();
    bool contain = SubThreadIDMap.contains(std::this_thread::get_id()) ||
                   threadIDMap.contains(std::this_thread::get_id());
    IDMutex.unlock();
    if (contain) {
      SubThreadsCreationLock.lock();
      if (SubThreads.get()) {
        SubThreadsCreationLock.unlock();
        return SubThreads->PushToSubThread(f);
      } else {
        SubThreads = std::make_unique<THREADS>(
            tCount, SubThreadIDMap, MasterthreadIDMap, IDMutex, DepthIndex + 1);
                  SubThreadsCreationLock.unlock();
        return SubThreads->PushToSubThread(f);
      }
    } else {
      return commonPush(f);
    }
  }
}

void THREADS::gEndWork(WRef wref)
{
	if(wref.Worker != nullptr)
	wref.Worker->checkWork(wref.uWid);
}

void THREADS::gEndWork(std::vector<WRef>& wrefv)
{
	for (auto& wref : wrefv) {
		if (wref.Worker != nullptr)
			wref.Worker->checkWork(wref.uWid);
	}
  wrefv.resize(0);
}







THREADS::THREAD::THREAD()
{
	tRunning.store(true);
	thread = std::thread([this] {exeWork(); });
	tThreadID = thread.get_id();
	lWaiting.store(0);
	Counter = 0;
}

THREADS::THREAD::~THREAD()
{
  for(auto& w : tWork){
    w.Function = [](){};
  }
	tRunning.store(false);
	cVariable.notify_all();				
	aVariable.notify_all();
	thread.join();
}

uint8_t THREADS::THREAD::GetFree(){
  uint8_t result(Counter);
  Counter++;
  if(tWork[result].Worked.load()){
    return result;
  }else{
    return GetFree();
  }
}

//Currently ONLY for concurrent work. Do not push parent/child work. Erratic behavior expected if you do. Add checking for queue overflow. eg Give instructions to work distribution that queue is full of unfinished work
uint8_t THREADS::THREAD::tPushWork(std::function<void()> f) {
	uint8_t result(IDs.AllocID());
  assert(tWork[result].Worked.load());
  tWork[result].uWorkMTX.lock();
  tWork[result].Function = std::move(f);
  tWork[result].Worked.store(false);
  tWork[result].uWorkMTX.unlock();
  cMut.lock();
	Queue[qSize.load()] = (result);
  ++qSize;
	cMut.unlock();
  cVariable.notify_one();
	return result;
};

void THREADS::recurBatch(const std::vector<std::function<void()>>& f, std::vector<std::function<void()>>::const_iterator& i)
{
	if (i != f.end())
	{
		{
			const auto & func = *i;
			func();
		}
		recurBatch(f, ++i);
	}
}


void THREADS::THREAD::exeWork() {

	workLock = std::unique_lock<std::mutex>(cMut);
	workLock.unlock();
  uint8_t lWorkCount = 0;
  std::array<uint8_t, 256> localQ;
	while (tRunning) {
		//Prep work
		workLock.lock();
		cVariable.wait(workLock, [this] {return qSize.load() > 0 || lWaiting.load() > 0 || !tRunning.load();});
		lWorkCount = qSize.load();
    memcpy(&localQ, Queue.data(), sizeof(uint8_t)*qSize.load());
    qSize.store(0);
    workLock.unlock();
		//Run work
		//auto time = std::chrono::duration <int, std::nano>(2000);
		//bVariable.wait_for(tBuss, time, [this] {return lWorkCount > 0 || lWaiting.load() > 0 || !tRunning.load(); });
		for (int i = lWorkCount-1; i >= 0; i--) {
      tWork[localQ[i]].Function();
      tWork[localQ[i]].uWorkMTX.lock();
      tWork[localQ[i]].Function = []{};
			tWork[localQ[i]].Worked.store(true);
      tWork[localQ[i]].uWorkMTX.unlock();
      aVariable.notify_all();
    }
		//lWorkCount = 0;
	}
}

void THREADS::THREAD::checkWork(uint8_t uWid) {
	bool waitCounter = false;
	std::unique_lock<std::mutex> lock(tWork[uWid].uWorkMTX);
	aVariable.wait(lock, [this, uWid, &waitCounter] {
		if (!waitCounter || !tRunning.load()) {
      ++lWaiting;
			assert(!(lWaiting.load() == 0));
			waitCounter = true;
		}
    cVariable.notify_one();
		return tWork[uWid].Worked.load() || !tRunning.load();
	});
  lock.unlock();
  IDs.FreeID(uWid);
	if (waitCounter) {
		--lWaiting;
	}

}
