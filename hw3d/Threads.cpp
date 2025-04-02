#include "Threads.h"



THREADS::THREADS(int cCount) {
	Threads.resize(cCount);
	lWorked.reserve(65535 * cCount);
	for (auto& t : Threads) {
		t = new THREAD;
	} 
};

THREADS::~THREADS() {
	for (auto& t : Threads) delete t;
	Threads.resize(0);
}

THREADS::WRef THREADS::gPushWork(std::function<void()> f)
{
	WRef Result{ -1 };
	int least = -1;
	int tInd = -1;
	for (auto i = 0; i < Threads.size(); i++) {
		auto& count = Threads[i]->wCount;
		if (count.load() < least || (least == -1)) {
			least = count.load();
			tInd = i;
		}
	}
	Result.uWid = Threads[tInd]->tPushWork(f, lWorked);
	Result.Worker = Threads[tInd];
	return Result;
}

void THREADS::gEndWork(WRef wref)
{
	if(wref.Worker != nullptr)
	wref.Worker->checkWork(wref.uWid, lWorked);
}







THREADS::THREAD::THREAD()
{
	Queue.reserve(65535);
	tWork.reserve(65535);
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

unsigned int THREADS::THREAD::tPushWork(std::function<void()> &f, std::unordered_map<unsigned int, lWork>& lWorked) {
	//int result = std::stoi(std::to_string(abs((short int)this->thread.get_id()._Get_underlying_id())) + std::to_string(abs((short int)eTime.get()->TimeLook())));
	unsigned int result = std::stoul(std::to_string(this->thread.get_id()._Get_underlying_id()) + std::to_string(Counter.load()));
	Counter.store(Counter.load() + 1);
	lWorked[result].uWorkMTX.lock();
	lWorked[result].Worked.store(false);
	lWorked[result].uWorkMTX.unlock();

	tWork[result] = werk{&lWorked[result], f};
	wCountMTX.lock();
	Queue.push_back(result);
	wCount.store(wCount.load() + 1);
	wCountMTX.unlock();
	cVariable.notify_one();
	return result;
};




//Fix random freezes
void THREADS::THREAD::exeWork() {
	std::vector<unsigned int> lWorkC;
	lWorkC.reserve(255);
	while (tRunning) {
		auto time = std::chrono::duration <int, std::nano>(2000000);

		std::unique_lock<std::mutex> lock(wCountMTX);
		//cVariable.wait_for(lock, time, [this] {return Queue.size() > 0 || lWaiting.load() > 0;});
		cVariable.wait(lock, [this] {
			return Queue.size() > 0 || lWaiting.load() > 0 || !tRunning.load();
		});
		lWorkC = Queue;
		Queue.resize(0);
		lock.unlock();

		for (auto i : lWorkC) {
			tWork[i].Function();
			tWork[i].lWork->Worked.store(true);
		}
		wCount.store(wCount.load() - lWorkC.size());
		aVariable.notify_all();
	}
}

void THREADS::THREAD::checkWork(unsigned int uWid, std::unordered_map<unsigned int, lWork>& lWorked) {
	std::unique_lock<std::mutex> lock(lWorked[uWid].uWorkMTX);
	//auto time = std::chrono::duration <int, std::nano>(20000);
	//aVariable.wait_for(lock, time, [this, uWid] {return tWork[uWid].wCheck->load() || !tRunning.load(); });
	lWaiting.store(lWaiting.load() + 1);
	aVariable.wait(lock, [this, uWid] {
		cVariable.notify_one();
		return tWork[uWid].lWork->Worked.load() || !tRunning.load();
	});
	lWaiting.store(lWaiting.load() - 1);
	tWork.erase(uWid);
	lock.unlock();
	lWorked.erase(uWid);
}
