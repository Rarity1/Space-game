#pragma once
#include "ePhysics.h"
class Engine {
public:
	Engine(Graphics* gfx, Keyboard* kbd);
	~Engine();
	int updaterate = 60;
	void iLoad();
	void Update();
	Physics::mThreadTime timer;
	bool engInit = true;
	std::vector<RStorage::eResource>& trackedModels;
private:
	double culmtime;
	std::thread EngThread;
	std::atomic<bool> eRun;
	struct Movement {
		float forward = 0.0;
		float backward = 0.0;
		float left = 0.0;
		float right = 0.0;
	};
	
	std::unique_ptr<Physics> phyx;
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
	Graphics* pGfx;

	RStorage::eResource* plModel;
	DirectX::XMFLOAT4 cWorld;
	DirectX::XMFLOAT4 nWorld;
	int timetorot;
	struct KeysPressed
	{
		bool w = false;
		bool a = false;
		bool s = false;
		bool d = false;
		bool k = false;
		bool q = false;
		bool e = false;
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
	};

	KeysPressed m_keysPressed;
	Keyboard* kbd;


};