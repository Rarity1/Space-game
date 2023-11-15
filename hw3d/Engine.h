#pragma once
#include "Graphics.h"
class Engine {
public:
	struct pCollision {
		XMFLOAT3 pos;
		float radius;
	};
	struct eResource {
		//Enum model name add time.
		UINT umID = 0;
		std::string name = "";
		bool isWorld = false;
		float mass = 1;
		Graphics::rpVect loadedModel;
		std::vector<pCollision> collision;
		XMFLOAT3 speed;
		RStorage::aRotation velDir;
		float scale = 1;
		eResource* mworld = nullptr;
	};
	Engine(Graphics* gfx, Keyboard* kbd);
	~Engine();
	void iLoad();
	void Update(float frametime);
	bool engInit = true;
	std::vector<eResource*> trackedModels;
private:
	struct Movement {
		float forward;
		float backward;
		float left;
		float right;
	};
	void UCampos();
	XMFLOAT3 rWorld(XMFLOAT3 pos1);
	XMFLOAT3 dWorld(XMFLOAT3 pos1);
	XMFLOAT3 cnWorld(XMFLOAT3 pos1);
	void OnKeyDown(unsigned char key);
	void OnKeyUp(unsigned char key);
	void cMPosUpdate();
	void UControls();
	float fDistance(XMFLOAT3 pos1, XMFLOAT3 pos2);
	void cGravity(eResource* obj);
	void cPlayermodel();
	Graphics* pGfx;
	float timer;
	float time2;
	XMFLOAT4 cWorld;
	XMFLOAT4 nWorld;
	struct KeysPressed
	{
		bool w = false;
		bool a = false;
		bool s = false;
		bool d = false;

		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
	};

	KeysPressed m_keysPressed;
	Keyboard* kbd;
	float GConst;


};