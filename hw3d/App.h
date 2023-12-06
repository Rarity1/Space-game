#pragma once
#include "Window.h"
#include "EngineTime.h"
#include "Engine.h"

class App {
public:
	App();
	//Master frame/ message loop
	int Go();
private:
	void DoFrame();
	Window wnd;
	EngineTime timer;
	std::atomic<bool> ignKey;
};