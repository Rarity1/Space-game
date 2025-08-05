#include "Threads.h"


//Sorting child processes sucks. Just use more instances if needing to multithread subfunctions 
THREADS::THREADS(int cCount) {
	tCount = cCount;
	mParent = std::this_thread::get_id();
	for (UINT t = 0; t < tCount; t++) {
		Threads[t] = new THREAD();
		threadIDMap[Threads[t]->thread.get_id()] = Threads[t];
	}
};

THREADS::~THREADS() {
	for (auto t = 0; t < tCount; t++) delete Threads[t];
}

THREADS::WRef THREADS::gPushWork(std::function<void()> f)
{
	WRef Result{ 0 };
	uint8_t least = 0;
	uint8_t tInd = 0;
	bool valid = false;
	uint8_t twCount(0);
	for (uint8_t i = 0; i < tCount; i++) {
		twCount = Threads[i]->qSize.load();
		twCount += Threads[i]->lWaiting.load();
		if (twCount < least || !valid) {

			valid = true;
			least = twCount;
			tInd = i;
		}
	}

	_ASSERT(valid);
	Threads[tInd]->cMut.lock();
	Result.uWid = Threads[tInd]->tPushWork(f);
	Threads[tInd]->cMut.unlock();
	Result.Worker = Threads[tInd];
	Threads[tInd]->cVariable.notify_one();


	return Result;
}


//Pushes batch of work onto single thread
THREADS::WRef THREADS::gPushWork(std::vector<std::function<void()>>& f)
{

	std::function<void()> recur([&f]() {auto vect = std::move(f); auto iter = vect.begin(); recurBatch(vect, iter); });

	WRef Result{ 0 };
	uint8_t least = 0;
	uint8_t tInd = 0;
	bool valid = false;
	auto thredid = std::this_thread::get_id();
	uint8_t twCount = 0;
	for (uint8_t i = 0; i < tCount; i++) {
		twCount = Threads[i]->qSize.load();
		twCount += Threads[i]->lWaiting.load();
		if (twCount < least || !valid) {

			valid = true;
			least = twCount;
			tInd = i;
		}
	}
	Threads[tInd]->cMut.lock();
	Result.uWid = Threads[tInd]->tPushWork(recur);
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
uint8_t THREADS::THREAD::tPushWork(std::function<void()> &f) {

	//cMut.lock();
	uint8_t result(Counter);
	tWork[result].uWorkMTX.lock();
	_ASSERT(tWork[result].Worked);
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
		auto& funct = *i;
		funct();
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

		for (auto& i : localQ) {
			if (lWorkCount > 0) {
				tWork[i].Function();
				tWork[i].uWorkMTX.lock();
				tWork[i].Worked = true;
				tWork[i].uWorkMTX.unlock();
				--lWorkCount;
			}
			else {
				break;
			}
		}
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
			_ASSERT(!(lWaiting.load() == 0));
			waitCounter = true;
		}
		return tWork[uWid].Worked || !tRunning.load();
	});
	lock.unlock();
	if (waitCounter) {
		--lWaiting;
	}

}
