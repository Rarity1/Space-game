#pragma once

#include <CWin.h>
#include <DSMouse.h>
#include <DSKeyboard.h>
#include <EngineTime.h>
#include <Exceptions.h>
#include <condition_variable>
#include <minwindef.h>
#include <winnt.h>
#include "Engine.h"

//Window Class
class Window {
	friend class APP;
	friend class StaticFunc;
public:
	Window(uint16_t width, uint16_t height, const char* name, HINSTANCE hInstance);
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	void SetTitle(const std::string& title);
private:
  	std::atomic<bool> Alive = true;
	MSG msg = tagMSG{ nullptr, WM_NULL };
	std::mutex upLock;
	std::mutex downLock;
	bool waitbl = false;
	std::condition_variable windowTimer;
	std::condition_variable appTimer;
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  std::function<LRESULT(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)> ImGuiHnd = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){return DefWindowProc(hWnd, msg, wParam, lParam);};
	Keyboard kbd;
	EngineTime clock;
	Mouse mouse;
	std::mutex winWait;
	thRect WindowRect;
	uint16_t width;
	uint16_t height;
	std::unique_ptr<Graphics> pGfx;
	std::unique_ptr<Engine> sEng;
	std::thread windowThread;
	HWND hWnd;
};

