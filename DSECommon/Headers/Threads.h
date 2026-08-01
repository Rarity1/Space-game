#pragma once
#include "CWin.h"
#include <functional>
#include <thread>
#include <condition_variable>
#include <map>
#include "EngineTime.h"

//Rewrite using futures
class DLL THREADS {
private:
	std::unique_ptr<THREADS> SubThreads;
	uint8_t DepthIndex = 0;

	struct werk {
		//The ID of the thread this work is a grandchild of. Should be 0 unless this is a subchild of current Threads instance.
		//std::vector<UINT> orderedParents;
		std::mutex uWorkMTX;
		std::function<void()> Function;
		bool Worked = true;
	};
	struct THREAD {
		friend class THREADS;
		THREAD();
		~THREAD();

		//std::mutex wCountMTX;
		//std::atomic<uint8_t> wCount = 0;
	private:
		inline uint8_t tPushWork(std::function<void()> f);
		void exeWork();
		void checkWork(unsigned int uWid);
		std::thread::id tThreadID;
		std::atomic<uint8_t> lWaiting;
		std::unique_ptr<EngineTime> eTime;
		std::unique_lock<std::mutex> workLock;
		std::atomic<bool> tRunning = true;
		std::thread thread;
		std::condition_variable cVariable;
		std::condition_variable aVariable;
		std::condition_variable bVariable;
		std::array<uint8_t, 256> localQ;
		std::mutex tBusy;
		uint8_t wIndex = 0;
		uint8_t Counter;
		std::mutex cMut;
		//uWid.
		std::vector<uint8_t> Queue;
		std::atomic<uint8_t> qSize = 0;
		uint8_t lWorkCount = 0;

		//Local work storage
		std::array<werk, 256> tWork;

	};

	std::array<THREAD*, 256> Threads;
	UINT tCount = 0;
	std::mutex lWorkMTX;
	static void recurBatch(std::vector<std::function<void()>>& f, std::vector<std::function<void()>>::iterator& i);

public:
	struct WRef {
		uint8_t uWid = 0;
		uint8_t DepthIndex = 0;
		THREADS::THREAD* Worker = nullptr;
	};
	THREADS(int cCount, uint8_t Depth = 0);
	~THREADS();

	std::map<std::thread::id, THREAD*> threadIDMap;

	inline WRef gPushWork(std::function<void()> f);
	inline WRef gPushWork(std::function<void()>& f);

	WRef gPushWork(std::vector<std::function<void()>>& f);
	void gEndWork(WRef wref);
	void gEndWork(std::vector<WRef>& wref);
};