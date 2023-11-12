#pragma once
#include "Graphics.h"
class Engine {
public:
	struct eResource {
		//Enum model name add time.
		UINT umID = 0;
		std::string name = "";
		RStorage::bmResource* loadedModel;
		float mass = 0;
		bool isWorld = false;
		XMFLOAT3 initPos = { 0,0,0 };
		float scale = 1;
		eResource* mworld = nullptr;
	};
	Engine(Graphics* gfx, Keyboard* kbd);
	~Engine();
	void iLoad();
	void Update();
	bool engInit = true;
	std::vector<eResource*> trackedModels;
private:
	void UCampos();
	XMFLOAT3 rWorld(XMFLOAT3 pos1);
	XMFLOAT3 dWorld(XMFLOAT3 pos1);
	XMFLOAT3 cnWorld(XMFLOAT3 pos1);
	void OnKeyDown(unsigned char key);
	void OnKeyUp(unsigned char key);
	
	void Unload();
	void Load();
	Graphics* pGfx;
	float timer;
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


};