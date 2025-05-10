#include "Threads.h"



THREADS::THREADS(int cCount) {
	tCount = cCount;
	for (UINT t = 0; t < tCount; t++) {
		Threads[t] = new THREAD();
	} 
};

THREADS::~THREADS() {
	for (auto t = 0; t < tCount; t++) delete Threads[t];
}

THREADS::WRef THREADS::gPushWork(std::function<void()>& f)
{
	WRef Result{ 0 };
	int least = -1;
	int tInd = 0;
	for (auto i = 0; i < tCount; i++) {
		auto& count = Threads[i]->wCount;
		if (count.load() < least || (least == -1)) {
			least = count.load();
			tInd = i;
		}

	}
	Result.uWid = Threads[tInd]->tPushWork(f);
	
	Result.Worker = Threads[tInd];
	return Result;
}


//Pushes batch of work onto single thread
THREADS::WRef THREADS::gPushWork(std::vector<std::function<void()>> f)
{
	WRef Result{ -1 };
	int least = -1;
	int tInd = 0;
	for (auto i = 0; i < tCount; i++) {
		auto& count = Threads[i]->wCount;
		if (count.load() < least || (least == -1)) {
			least = count.load();
			tInd = i;
		}
	}
	auto recur = std::function<void()>([f]() {recurBatch(f); });

	Result.uWid = Threads[tInd]->tPushWork(recur);
	Result.Worker = Threads[tInd];
	return Result;
}

void THREADS::gEndWork(WRef wref)
{
	if(wref.Worker != nullptr)
	wref.Worker->checkWork(wref.uWid);
}







THREADS::THREAD::THREAD()
{
	Queue.reserve(256);
	eTime = std::make_unique<EngineTime>();
	tRunning.store(true);
	thread = move(std::thread([this] {exeWork(); }));
}

THREADS::THREAD::~THREAD()
{
	tRunning.store(false);

	cVariable.notify_all();				
	aVariable.notify_all();
	bVariable.notify_all();
	thread.join();
}


//Currently ONLY for concurrent work. Do not push parent/child work. Erratic behavior expected if you do. Add checking for queue overflow. eg Give instructions to work distribution that queue is full of unfinished work
uint8_t THREADS::THREAD::tPushWork(std::function<void()> &f) {

	cMut.lock();
	unsigned int result = Counter;
	Counter++;
	cMut.unlock();

	tWork[result].uWorkMTX.lock();
	tWork[result].Function = f;
	tWork[result].Worked = false;
	tWork[result].uWorkMTX.unlock();

	wCountMTX.lock();
	Queue.emplace_back(result);
	wCountMTX.unlock();

	cVariable.notify_one();
	return result;
};

//Wildly inefficient with memory. Find better way
void THREADS::recurBatch(std::vector<std::function<void()>> f, unsigned int i)
{
	if (i < f.size())
	{
		f[i]();
		auto next = i+1;
		recurBatch(f, next);
	}
}


//Fix random freezes
void THREADS::THREAD::exeWork() {
	uint8_t cWorkCount = 0;
	while (tRunning) {
		//auto time = std::chrono::duration <int, std::nano>(2000000);
		std::unique_lock<std::mutex> lock(wCountMTX);
		//cVariable.wait_for(lock, time, [this] {return Queue.size() > 0 || lWaiting.load() > 0;});
		cVariable.wait(lock, [this] {
			return Queue.size() > 0 || lWaiting.load() > 0 || !tRunning.load();
		});
		cWorkCount = Queue.size();
		auto localQ = Queue;
		wCount.store(cWorkCount);
		Queue.resize(0);
		lock.unlock();


		for (uint8_t i : localQ) {
			tWork[i].uWorkMTX.lock();
			tWork[i].Function();
			tWork[i].Worked = true;
			tWork[i].uWorkMTX.unlock();
		}
		wCount.store(wCount.load() - cWorkCount);


		aVariable.notify_all();
		
	}
}

void THREADS::THREAD::checkWork(unsigned int uWid) {
	std::unique_lock<std::mutex> lock(tWork[uWid].uWorkMTX);
	//auto time = std::chrono::duration <int, std::nano>(20000);
	//aVariable.wait_for(lock, time, [this, uWid] {return tWork[uWid].wCheck->load() || !tRunning.load(); });
	lWaiting.store(lWaiting.load() + 1);
	aVariable.wait(lock, [this, uWid] {
		cVariable.notify_one();
		return tWork[uWid].Worked || !tRunning.load();
	});
	lWaiting.store(lWaiting.load() - 1);
	lock.unlock();

}
