#include "Threads.h"
#include <cassert>


//Add more depth when doing recursive functions. Depth is amount of thread recursion - 1
THREADS::THREADS(int cCount, uint8_t Depth) {
	tCount = cCount;
	DepthIndex = Depth;
	for (UINT t = 0; t < tCount; t++) {
		Threads[t] = new THREAD();
		threadIDMap[Threads[t]->thread.get_id()] = Threads[t];
	}
	if (Depth > 0) {
		SubThreads = std::make_unique<THREADS>(cCount, Depth - 1);
		threadIDMap.insert(SubThreads->threadIDMap.begin(), SubThreads->threadIDMap.end());
	}
};

THREADS::~THREADS() {
	for (auto t = 0; t < tCount; t++) delete Threads[t];
}

THREADS::WRef THREADS::gPushWork(std::function<void()> f)
{
	WRef Result{ 0, DepthIndex };
	uint8_t least = 0;
	uint8_t tInd = 0;
	bool valid = false;
	uint8_t twCount(0);
	if (threadIDMap.find(std::this_thread::get_id()) == threadIDMap.end()) {
		for (uint8_t i = 0; i < tCount; i++) {
			twCount = Threads[i]->qSize.load() + Threads[i]->lWaiting.load();
			if (twCount < least || !valid) {
				valid = true;
				least = twCount;
				tInd = i;
			}

			

		}
	}
	else {
		if (DepthIndex > 0) {
			return SubThreads->gPushWork(std::move(f));
		}
		else {
      assert(false);
			//If this code runs just add more depth
			/*
			for (uint8_t i = 0; i < tCount; i++) {
				bool test = threadIDMap[std::this_thread::get_id()] != Threads[i];
				twCount = Threads[i]->qSize.load() + Threads[i]->lWaiting.load();
				if ((twCount < least && test) || (!valid && test)) {
					valid = true;
					least = twCount;
					tInd = i;
				}
			}
			*/
		}
	}

	//Process depth needed if this throws
	assert(valid);
	Threads[tInd]->cMut.lock();
	Result.uWid = Threads[tInd]->tPushWork(std::move(f));
	Threads[tInd]->cMut.unlock();
	Result.Worker = Threads[tInd];
	Threads[tInd]->cVariable.notify_one();


	return Result;
}

inline THREADS::WRef THREADS::gPushWork(std::function<void()>& f)
{
	WRef Result{ 0, DepthIndex };
	uint8_t least = 0;
	uint8_t tInd = 0;
	bool valid = false;
	uint8_t twCount(0);
	if (threadIDMap.find(std::this_thread::get_id()) == threadIDMap.end()) {
		for (uint8_t i = 0; i < tCount; i++) {
			twCount = Threads[i]->qSize.load() + Threads[i]->lWaiting.load();
			if (twCount < least || !valid) {
				valid = true;
				least = twCount;
				tInd = i;
			}



		}
	}
	else {
		if (DepthIndex > 0) {
			return SubThreads->gPushWork(std::move(f));
		}
		else {
			//If this code runs just add more depth
			for (uint8_t i = 0; i < tCount; i++) {
				bool test = threadIDMap[std::this_thread::get_id()] != Threads[i];
				twCount = Threads[i]->qSize.load() + Threads[i]->lWaiting.load();
				if ((twCount < least && test) || (!valid && test)) {
					valid = true;
					least = twCount;
					tInd = i;
				}
			}
		}
	}

	assert(valid);
	Threads[tInd]->cMut.lock();
	Result.uWid = Threads[tInd]->tPushWork(std::move(f));
	Threads[tInd]->cMut.unlock();
	Result.Worker = Threads[tInd];
	Threads[tInd]->cVariable.notify_one();


	return Result;
}


//Pushes batch of work onto single thread
inline THREADS::WRef THREADS::gPushWork(std::vector<std::function<void()>>& f)
{

	std::function<void()> recur([&f]() {auto vect = std::move(f); auto iter = vect.begin(); recurBatch(vect, iter); });

	WRef Result{ 0, DepthIndex };
	uint8_t least = 0;
	uint8_t tInd = 0;
	bool valid = false;
	uint8_t twCount(0);
	if (threadIDMap.find(std::this_thread::get_id()) == threadIDMap.end()) {
		for (uint8_t i = 0; i < tCount; i++) {
			twCount = Threads[i]->qSize.load() + Threads[i]->lWaiting.load();
			if (twCount < least || !valid) {
				valid = true;
				least = twCount;
				tInd = i;
			}



		}
	}
	else {
		if (DepthIndex > 0) {
			return SubThreads->gPushWork(std::move(recur));
		}
		else {
			//If this code runs just add more depth
			for (uint8_t i = 0; i < tCount; i++) {
				bool test = threadIDMap[std::this_thread::get_id()] != Threads[i];
				twCount = Threads[i]->qSize.load() + Threads[i]->lWaiting.load();
				if ((twCount < least && test) || (!valid && test)) {
					valid = true;
					least = twCount;
					tInd = i;
				}
			}
		}
	}

	assert(valid);
	Threads[tInd]->cMut.lock();
	Result.uWid = Threads[tInd]->tPushWork(std::move(recur));
	Threads[tInd]->cMut.unlock();
	Result.Worker = Threads[tInd];
	Threads[tInd]->cVariable.notify_one();


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
	eTime = std::make_unique<EngineTime>();
	tRunning.store(true);
	thread = move(std::thread([this] {exeWork(); }));
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

	//cMut.lock();
	uint8_t result(Counter);
	tWork[result].uWorkMTX.lock();
	assert(tWork[result].Worked);
	tWork[result].Function = std::move(f);
	tWork[result].Worked = false;
	tWork[result].uWorkMTX.unlock();
	Counter++;
	Queue.emplace_back(result);
	//cMut.unlock();

	return result;
};

void THREADS::recurBatch(std::vector<std::function<void()>>& f, std::vector<std::function<void()>>::iterator& i)
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

		auto time = std::chrono::duration <int, std::nano>(2000);
		

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
