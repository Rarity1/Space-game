#pragma once
#include "RStorage.h"
#include "ePhysics.h"
#include "Graphics.h"
#include "EngineTime.h"
#include "Threads.h"
#include "DSKeyboard.h"
#include "ObjectTracking.h"

class DLL Engine {
	friend class App;
  friend class Window;
public:
	Engine(Keyboard& kbd, EngineTime& clock, thRect &WindowRect, HWND &hWnd);
	~Engine();
	const int updaterate = 60;
	void iLoad();
	bool Update();
	double engineTimeTaken = 0;
	EngineTime& Clock;
  std::unique_ptr<THREADS> tMain;
  std::unique_ptr<RStorage> storage;
  std::unique_ptr<Tracker> pTracker;
  Tracker& tracker;	
	std::unique_ptr<Physics> phyx;
  std::unique_ptr<Graphics> pGfx;
  Graphics& rGfx;
	Object* plModel;

	struct KeysPressed
	{
		//Rename to actions
		enum KEYS {
			J,
			W,
			A,
			S,
			D,
			K,
			Q,
			E,
			left,
			right,
			up,
			down
		};

		bool Find(KEYS key) {
			return keyMap[key];
		}
		void Set(KEYS key, bool isPressed = true) {
			if (isPressed) {
				keyBuffer[key] = isPressed;
			}
			keyMap[key] = isPressed;
		}
		bool FindBuffered(KEYS key) {
			auto result = keyBuffer[key];
			keyBuffer[key] = keyMap[key];
			return result;
		}

	private:
		std::map<KEYS, bool> keyMap = {
	{J, false},
	{W, false},
	{A, false},
	{S, false},
	{D, false},
	{K, false},
	{Q, false},
	{E, false},
	{left, false},
	{right, false},
	{up, false},
	{down, false}
		};
		std::map<KEYS, bool> keyBuffer = keyMap;

	};
	KeysPressed m_keysPressed;
	struct Event {
		unsigned short wPriority;
		std::function<void()> wFunc;
	};
	virtual void enQueueExternCommands();
private:
	UINT cTrackedInstance;
	THREADS::WRef eWref;
	std::atomic<bool> eRun;
	struct Movement {
		float forward = 0.0;
		float backward = 0.0;
		float left = 0.0;
		float right = 0.0;
		bool movestop = false;
	};
	EngineTime updateClock;
	EngineTime ucontrolClock;

	//Queues a function onto the event queue without running the function. Larger priority number = lower priority. 
	void queueCommand(std::function<void()> Function, unsigned short Priority = 0);
	std::vector<std::function<void()>>& getCQueue();
	void enQueueEngineCommands();
	int eventBusSync();
	bool pauseLoop = false;
	void EngineLoop();

	DirectX::XMFLOAT3 rWorld(DirectX::XMFLOAT3 pos1);
	DirectX::XMFLOAT3 dWorld(DirectX::XMFLOAT3 pos1);
	DirectX::XMFLOAT3 cnWorld(DirectX::XMFLOAT3 pos1);
	void OnKeyDown(unsigned char key);
	void OnKeyUp(unsigned char key);
	void UControls();
	void mAniUpdate();
  //Update active instances
	void updateInstances();
	void uPhysics();

	std::mutex evBusLock;


	DirectX::XMFLOAT4 cWorld;
	DirectX::XMFLOAT4 nWorld;
	EngineTime inputDelay;
	std::vector<std::function<void()>> wFunctions;
	Keyboard& kbd;
	std::mutex usingThread;
	double lastD = 0.0;
	std::atomic<short int> queueCount;
	std::list<Event> QueueList;

	std::thread LoopThread;
	std::condition_variable loopVariable;
	std::unique_lock<std::mutex> loopLock;
	std::mutex loopMutex;




};