#pragma once
#include "Window.h"
#include "Editor.h"

class App {
public:
	std::atomic<bool> Alive = true;
	App();
	//Master frame/ message loop
	int Go();
private:
	void DoFrame();
	double delta = 0;
	Window wnd;
	THREADS::WRef graphicsWref;
	std::atomic<bool> ignKey;
	double updaterate = 0;
};