#pragma once
#include "EngineTime.h"
#include "CWin.h"

//Rewrite using futures
class THREADS {
private:
	struct THREAD {
		THREAD();
		~THREAD();
		unsigned int tPushWork(std::function<void()>& f);
		void tEndWork();
		void exeWork();
		void queueCheck();
		void checkWork(unsigned int uWid);
		void recurCheck();

		struct werk {
			std::atomic<bool>* wCheck;
			std::function<void()> Function;
		};

		std::atomic<unsigned short> Counter;
		std::unordered_map<unsigned int, werk> tWork;
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
	};
	std::vector<THREAD*> Threads;
	std::unordered_map<unsigned int, std::atomic<bool>> lWorked;

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