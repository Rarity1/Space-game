#pragma once
#include "ePhysics.h"
#include "EngineTime.h"
class Engine {
public:
	Engine(Graphics& gfx, Keyboard& kbd, EngineTime& clock);
	~Engine();
	const int updaterate = 60;
	void iLoad();
	void Update(double delta);
	bool engInit = true;
	EngineTime& Clock;
	std::vector<RStorage::eResource>& trackedModels;
	std::unique_ptr<Physics> phyx;


	struct KeysPressed
	{

		enum KEYS {
			W = 57,
			A = 41,
			S = 53,
			D = 44,
			K = 48,
			Q = 51,
			E = 45,
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
		Event(std::function<void()> Fnct, int prio = -1):
		Function(Fnct),
		priority(new int(prio))
		{
		}
		Event(const Event& e) :
			Function(e.Function),
			priority(new int(*e.priority))
		{
		};
		~Event() {
			delete priority;
		};
		bool operator==(const Event& r) const
		{
			return (*priority == *r.priority && Function.target_type() == r.Function.target_type());
		}
		virtual void evnt() {
			Function();
		};
	private:
		int* priority;
		std::function<void()> Function;
	};
	virtual void enQueueExternCommands();
private:
	std::thread EngThread;
	std::atomic<bool> eRun;
	struct Movement {
		float forward = 0.0;
		float backward = 0.0;
		float left = 0.0;
		float right = 0.0;
	};
	//Queues a function onto the event queue without running the function. Larger priority number = lower priority. 
	void queueCommand(std::function<void()> Function, int Priority = 0);
	void enQueueEngineCommands();
	double cspin = 0;
	void UCampos();
	void SetModelPosition(RStorage::eResource* model);
	DirectX::XMFLOAT3 rWorld(DirectX::XMFLOAT3 pos1);
	DirectX::XMFLOAT3 dWorld(DirectX::XMFLOAT3 pos1);
	DirectX::XMFLOAT3 cnWorld(DirectX::XMFLOAT3 pos1);
	void OnKeyDown(unsigned char key);
	void OnKeyUp(unsigned char key);
	void mAniUpdate();
	void UControls();
	void cPlayermodel();
	void RotateCam(float Pitch = 0, float Yaw = 0, float Roll = 0);
	//Fires all queued events/functions in order by priority
	int eventBusSync();
	std::mutex evBusLock;
	std::thread oldBusThread;
	Graphics& pGfx;
	RStorage::eResource* plModel;
	DirectX::XMFLOAT4 cWorld;
	DirectX::XMFLOAT4 nWorld;
	
	Keyboard& kbd;
	std::mutex usingThread;
	double lastD = 0.0;
	std::vector<Event*> eventQueue;
	std::atomic<short int> queueCount;
	std::mutex QueueLock;
	std::vector<std::thread> QueueThreads;
	std::mutex QueueTLock;
};