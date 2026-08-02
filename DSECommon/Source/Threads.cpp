#include "Threads.h"
#include <cassert>
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

THREADS::WRef THREADS::PushToSubThread(std::function<void()> &f) {

  WRef Result{0, DepthIndex};
  uint8_t least = 0;
  uint8_t tInd = 0;
  bool valid = false;
  uint8_t twCount(0);
  IDMutex.lock();
  bool containID = MasterthreadIDMap.contains(std::this_thread::get_id());
  IDMutex.unlock();
  if (!containID) {
    for (uint8_t i = 0; i < tCount; i++) {
      twCount = Threads[i]->qSize.load() + Threads[i]->lWaiting.load();
      if (twCount < least || !valid) {
        valid = true;
        least = twCount;
        tInd = i;
      }
    }
    assert(valid);
    Result.uWid = Threads[tInd]->tPushWork(std::move(f));
    Result.Worker = Threads[tInd].get();
    Threads[tInd]->cVariable.notify_one();
  } else {
    IDMutex.lock();
    bool contain = SubThreadIDMap.contains(std::this_thread::get_id()) ||
                   threadIDMap.contains(std::this_thread::get_id());
    IDMutex.unlock();
    if (contain) {
      SubThreadsCreationLock.lock();
      if (SubThreads.get()) {
        Result = SubThreads->PushToSubThread(f);
      } else {
        SubThreads = std::make_unique<THREADS>(
            tCount, SubThreadIDMap, MasterthreadIDMap, IDMutex, DepthIndex + 1);
        Result = SubThreads->gPushWork(f);
      }
      SubThreadsCreationLock.unlock();
    } else {
      for (uint8_t i = 0; i < tCount; i++) {
        twCount = Threads[i]->qSize.load() + Threads[i]->lWaiting.load();
        if (twCount < least || !valid) {
          valid = true;
          least = twCount;
          tInd = i;
        }
      }
      assert(valid);
      Result.uWid = Threads[tInd]->tPushWork(std::move(f));
      Result.Worker = Threads[tInd].get();
      Threads[tInd]->cVariable.notify_one();
    }
  }

  return Result;
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
}







THREADS::THREAD::THREAD()
{
	Queue.reserve(256);
	tRunning.store(true);
	thread = std::thread([this] {exeWork(); });
	tThreadID = thread.get_id();
	lWaiting.store(0);
	Counter = 0;
}

THREADS::THREAD::~THREAD()
{
	tRunning.store(false);
	cVariable.notify_all();				
	aVariable.notify_all();
	thread.join();
}


//Currently ONLY for concurrent work. Do not push parent/child work. Erratic behavior expected if you do. Add checking for queue overflow. eg Give instructions to work distribution that queue is full of unfinished work
uint8_t THREADS::THREAD::tPushWork(std::function<void()> f) {

	cMut.lock();
	uint8_t result(Counter);
	tWork[result].uWorkMTX.lock();
	assert(tWork[result].Worked);
	tWork[result].Function = std::move(f);
	tWork[result].Worked = false;
	tWork[result].uWorkMTX.unlock();
	Counter++;
	Queue.emplace_back(result);
	cMut.unlock();

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
	while (tRunning) {
		//Prep work
		workLock.lock();
		cVariable.wait(workLock, [this] {return Queue.size() > 0 || lWaiting.load() > 0 || !tRunning.load();});
		lWorkCount = Queue.size();
		qSize.store(lWorkCount);
		memcpy(&localQ, Queue.data(), sizeof(uint8_t)*Queue.size());
		Queue.resize(0);
		workLock.unlock();

		//Run work
		//auto time = std::chrono::duration <int, std::nano>(2000);
		//bVariable.wait_for(tBuss, time, [this] {return lWorkCount > 0 || lWaiting.load() > 0 || !tRunning.load(); });
		for (auto i = lWorkCount-1; i >= 0; i--) {
			tWork[localQ[i]].Function();
			tWork[localQ[i]].uWorkMTX.lock();
			tWork[localQ[i]].Worked = true;
			tWork[localQ[i]].uWorkMTX.unlock();
		}

		lWorkCount = 0;
		qSize.store(0);

		aVariable.notify_all();
	}
}

void THREADS::THREAD::checkWork(unsigned int uWid) {
	bool waitCounter = false;
	std::unique_lock<std::mutex> lock(tWork[uWid].uWorkMTX);
	aVariable.wait(lock, [this, uWid, &waitCounter] {
		cVariable.notify_one();
		if (!waitCounter || !tRunning.load()) {
			++lWaiting;
			assert(!(lWaiting.load() == 0));
			waitCounter = true;
		}
		return tWork[uWid].Worked || !tRunning.load();
	});
	lock.unlock();
	if (waitCounter) {
		--lWaiting;
	}

}
