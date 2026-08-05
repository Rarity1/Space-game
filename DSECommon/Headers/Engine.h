#pragma once
#include "RStorage.h"
#include "ePhysics.h"
#include "Graphics.h"
#include "EngineTime.h"
#include "Threads.h"
#include "ObjectTracking.h"

class Input;

class DLL Engine {
	friend class App;
  friend class Window;
public:
	Engine(Input& InputHandler, WRect &WindowRect, HWND &hWnd);
	~Engine();
	const int updaterate = 60;
	void iLoad();
	bool Update();
	double engineTimeTaken = 0;
	EngineTime Clock;
  std::unique_ptr<THREADS> tMain;
  std::unique_ptr<RStorage> storage;
  std::unique_ptr<Tracker> pTracker;
  Tracker& tracker;	
	std::unique_ptr<Physics> phyx;
  std::unique_ptr<Graphics> pGfx;
  Graphics& rGfx;
	Object* plModel;
	struct Event {
		unsigned short wPriority;
		std::function<void()> wFunc;
	};
	void enQueueExternCommands();
private:
	UINT cTrackedInstance;
	THREADS::WRef eWref;
	std::atomic<bool> eRun;

	EngineTime updateClock;
	EngineTime ucontrolClock;

	//Queues a function onto the event queue without running the function. Larger priority number = lower priority. 
	void queueCommand(std::function<void()> Function, unsigned short Priority = 0);
	std::vector<std::function<void()>>& getCQueue();
	void enQueueEngineCommands();
	int eventBusSync();
	bool pauseLoop = false;
	void EngineLoop();
	FLOAT3 rWorld(FLOAT3 pos1);
	FLOAT3 dWorld(FLOAT3 pos1);
	FLOAT3 cnWorld(FLOAT3 pos1);
	void mAniUpdate();
  //Update active instances
	void updateInstances();
	void uPhysics();

	std::mutex evBusLock;


	FLOAT4 cWorld;
	FLOAT4 nWorld;
	std::vector<std::function<void()>> wFunctions;
	Input& InputHndlr;
	std::mutex usingThread;
	double lastD = 0.0;
	std::atomic<short int> queueCount;
	std::list<Event> QueueList;

	std::thread LoopThread;
	std::condition_variable loopVariable;
	std::unique_lock<std::mutex> loopLock;
	std::mutex loopMutex;




};