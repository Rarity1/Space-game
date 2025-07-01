#pragma once
#include "Window.h"
#include "EngineTime.h"
#include "Engine.h"

class App {
public:
	std::atomic<bool> Alive = true;
	App();
	//Master frame/ message loop
	int Go();
private:
	void DoFrame();
	Window wnd;
	std::atomic<bool> ignKey;
	double updaterate = 0;
};