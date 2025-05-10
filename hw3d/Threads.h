#pragma once
#include "EngineTime.h"
#include "CWin.h"

//Rewrite using futures
class THREADS {
private:
	struct werk {
		bool Worked = true;
		std::function<void()> Function;
		std::mutex uWorkMTX;
	};
	struct THREAD {
		THREAD();
		~THREAD();
		uint8_t tPushWork(std::function<void()>& f);

		void exeWork();

		std::atomic<unsigned int> lWaiting = 0;
		void checkWork(unsigned int uWid);
		std::mutex wCountMTX;
		std::vector<uint8_t> Queue;
		std::atomic<uint8_t> wCount = 0;
	private:
		std::unique_ptr<EngineTime> eTime;

		std::atomic<bool> tRunning = true;
		std::thread thread;
		std::condition_variable cVariable;
		std::condition_variable aVariable;
		std::condition_variable bVariable;

		std::mutex wMutex;
		std::mutex aMutex;
		std::mutex bMutex;
		uint8_t Counter = 0;
		std::mutex cMut;


		//Local work Queue
		std::array<werk, 256> tWork;
		//std::mutex tWorkMTX;

	};
	std::array<THREAD*, 256> Threads;
	UINT tCount = 0;
	std::mutex lWorkMTX;
	static void recurBatch(std::vector<std::function<void()>> f, unsigned int i = 0);

	//Global work map
	//std::array<werk[256], 256> lWorked;
public:
	THREADS(int cCount);
	~THREADS();

	struct WRef {
		uint8_t uWid;
		THREADS::THREAD* Worker;
	};
	WRef gPushWork(std::function<void()>& f);
	WRef gPushWork(std::vector<std::function<void()>> f);
	void gEndWork(WRef wref);

};