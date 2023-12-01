#pragma once
#include "Window.h"
#include "EngineTime.h"
#include "Engine.h"

class App {
public:
	App();
	//Master frame/ message loop
	int Go();
	void DoFrame();
	
private:
	std::thread EngThread;
	Window wnd;
	EngineTime timer;
};