#pragma once
#include "EngineTime.h"
#include "CWin.h"

//Rewrite using futures
class THREADS {
private:
	struct lWork {
		std::atomic<bool> Worked;
		std::mutex uWorkMTX;
	};
	struct THREAD {
		THREAD();
		~THREAD();
		unsigned int tPushWork(std::function<void()>& f, std::unordered_map<unsigned int, lWork>& lWorked);
		void exeWork();
		void checkWork(unsigned int uWid, std::unordered_map<unsigned int, lWork>& lWorked);
		std::atomic<unsigned int> lWaiting = 0;

		struct werk {
			lWork* lWork;
			std::function<void()> Function;
		};
		std::atomic<unsigned short> wCount;
		std::atomic<unsigned short> Counter;
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


		//Local work Queue
		std::vector<unsigned int> Queue;
		std::mutex wCountMTX;
		std::unordered_map<unsigned int, werk> tWork;
	};
	std::vector<THREAD*> Threads;

	std::unordered_map<unsigned int, lWork> lWorked;

public:
	THREADS(int cCount);
	~THREADS();

	struct WRef {
		unsigned int uWid;
		THREADS::THREAD* Worker;
	};
	WRef gPushWork(std::function<void()> f);
	void gEndWork(WRef wref);

};