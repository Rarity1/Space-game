#pragma once

#include <DSMouse.h>
#include <DSKeyboard.h>
#include <EngineTime.h>
#include <Graphics.h>
#include <GraphicsErrors.h>
#include <Engine.h>
#include <Exceptions.h>
#include <winnt.h>


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
	std::optional<WPARAM> ProcessMessages();
private:
	std::mutex upLock;
	std::mutex downLock;
	bool waitbl = false;
	std::condition_variable windowTimer;
	std::condition_variable appTimer;
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	Keyboard kbd;
	EngineTime clock;
	Mouse mouse;
	std::mutex winWait;
	Graphics::thRect WindowRect;
	uint16_t width;
	uint16_t height;
	std::unique_ptr<Graphics> pGfx;
	std::unique_ptr<Engine> sEng;
	std::thread windowThread;
	HWND hWnd;
};

