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
		float gCollision = 1;
		float speed;
		XMFLOAT4 grav{0,0,0,0};
		XMFLOAT4 velDir;
		float scale = 1;
		eResource* mworld = nullptr;
		float friction = 0;
	};
	Engine(Graphics* gfx, Keyboard* kbd);
	~Engine();
	int updaterate = 60;
	void iLoad();
	void Update(float frametime);
	bool engInit = true;
	std::vector<eResource*> trackedModels;
private:
	struct Movement {
		float forward = 0.0;
		float backward = 0.0;
		float left = 0.0;
		float right = 0.0;
	};

	double cspin = 0;
	void UCampos();
	XMFLOAT3 rWorld(XMFLOAT3 pos1);
	XMFLOAT3 dWorld(XMFLOAT3 pos1);
	XMFLOAT3 cnWorld(XMFLOAT3 pos1);
	void OnKeyDown(unsigned char key);
	void OnKeyUp(unsigned char key);
	void cMPosUpdate(eResource* mUpdate, eResource* tempres);
	void mMove(eResource* mUpdate);
	void UControls();
	float fDistance(XMFLOAT3 pos1, XMFLOAT3 pos2);
	void cGravity(eResource* obj);
	void cPlayermodel();
	void procGenCollision(eResource* m, eResource* tempres);
	void pSpecCollison(eResource* obj1, eResource* obj2, float radialdist, float actualdist);
	Graphics* pGfx;
	float timer;
	float time2;
	eResource* plModel;
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