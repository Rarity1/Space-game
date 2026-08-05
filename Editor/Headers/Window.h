#pragma once

#include "InputHandler.h"
#include "CommonStructs.h"
#include <condition_variable>

#if defined(_WIN32)
#include "CWin.h"
#endif
class Window;
class Graphics;
class Engine;
static Window* Context;
//Window Class
class Window {
	friend class APP;
public:
	Window(uint16_t width, uint16_t height, const char* name, HINSTANCE hInstance);
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	void SetTitle(const std::string& title);
private:
  
  static LRESULT StaticHandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
		return Context->HandleMsg(hWnd, msg, wParam, lParam);
	};

  std::atomic<bool> Alive = true;
	MSG msg = tagMSG{ nullptr, WM_NULL };
	std::mutex upLock;
	std::mutex downLock;
	bool waitbl = false;
	std::condition_variable windowTimer;
	std::condition_variable appTimer;
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  std::function<LRESULT(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)> ImGuiHnd = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){return DefWindowProc(hWnd, msg, wParam, lParam);};

  Input InputHndlr;
	std::mutex winWait;
	WRect WindowRect;
	uint16_t width;
	uint16_t height;
	Graphics* pGfx;
	std::unique_ptr<Engine> sEng;
	std::thread windowThread;
	HWND hWnd;
};

