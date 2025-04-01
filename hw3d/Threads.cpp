#include "Threads.h"



THREADS::THREADS(int cCount) {
	Threads.resize(cCount);
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
	int least = 0;
	int tInd = 0;
	for (auto i = 0; i < Threads.size(); i++) {
		int count = Threads[i]->Counter;
		if (count < least || least == 0 && count == 0) {
			least = count;
			tInd = i;
		}
	}
	Result.uWid = Threads[tInd]->tPushWork(f);
	Result.Worker = Threads[tInd];
	lWorked[Result.uWid] = false;
	//move this into private
	Result.Worker->tWork[Result.uWid].wCheck = &lWorked[Result.uWid];
	return Result;
}

void THREADS::gEndWork(WRef wref)
{
	if(wref.Worker != nullptr)
	wref.Worker->checkWork(wref.uWid);
}







THREADS::THREAD::THREAD()
{
	Queue.reserve(255);
	tWork.reserve(255);
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

unsigned int THREADS::THREAD::tPushWork(std::function<void()> &f) {
	//int result = std::stoi(std::to_string(abs((short int)this->thread.get_id()._Get_underlying_id())) + std::to_string(abs((short int)eTime.get()->TimeLook())));
	unsigned int result = std::stoul(std::to_string(this->thread.get_id()._Get_underlying_id()) + std::to_string(Counter.load()));
	Counter.store(Counter.load() + 1);
	tWork[result] = werk{nullptr, f};
	wCountMTX.lock();
	Queue.push_back(result);
	wCountMTX.unlock();
	cVariable.notify_all();
	return result;
};




//Fix random freezes
void THREADS::THREAD::exeWork() {
	while (tRunning) {
		std::unique_lock<std::mutex> lock(wMutex);
		cVariable.wait(lock, [this] {
			wCountMTX.lock();
			short Count = Queue.size();
			wCountMTX.unlock();
		return Count > 0 || !tRunning.load();
		});
		wCountMTX.lock();
		auto lWorkC = Queue;
		Queue.resize(0);
		wCountMTX.unlock();
		for (auto i : lWorkC) {
			tWork[i].Function();
			tWork[i].wCheck->store(true);
		}
		aVariable.notify_all();
	}
}

void THREADS::THREAD::checkWork(unsigned int uWid) {
	std::unique_lock<std::mutex> lock(aMutex);
	//auto time = std::chrono::duration <int, std::nano>(20000);
	//aVariable.wait_for(lock, time, [this, uWid] {return tWork[uWid].wCheck->load() || !tRunning.load(); });
	aVariable.wait(lock, [this, uWid] {return tWork[uWid].wCheck->load() || !tRunning.load(); });

}
