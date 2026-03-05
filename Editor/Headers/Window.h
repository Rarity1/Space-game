#pragma once


#include <sstream>
#include "DLLImports.h"
#include "imgui.h"



//Window Class
class Window {
	friend class App;
public:
	class Exception : public Exceptions {
		using Exceptions::Exceptions;
	public:
		static std::string TranslateErrorCode(HRESULT hr) noexcept;
	};
	class HrException : public Exception
	{
	public:
		HrException(HRESULT hr, int line, const char* file) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorDescription() const noexcept;
	private:
		HRESULT hr;

	};
private:

	class WindowClass
	{
	public:
		static const char* GetName() noexcept;
		static HMODULE GetInstance() noexcept;
	private:

		WindowClass();
		~WindowClass();
		WindowClass(const WindowClass&) = delete;
		WindowClass& operator=(const WindowClass&) = delete;
		static constexpr const char* wndClassName = "Engine Window";
		static WindowClass wndClass;
		HMODULE hInst;
	};
public:
	Window(uint16_t width, uint16_t height, const char* name, std::atomic<bool>& Alive);
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	void SetTitle(const std::string& title);
	static std::optional<WPARAM> ProcessMessages();
	void Update();
	void exeWinLoop(std::atomic<bool>& Alive);

private:
	std::mutex upLock;
	std::mutex downLock;
	bool waitbl = false;
	std::condition_variable windowTimer;
	std::condition_variable appTimer;

	static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	Keyboard kbd;
	EngineTime clock;
	Mouse mouse;
	std::condition_variable winReady;
	std::mutex winWait;
private:
	Graphics::thRect WindowRect;
	uint16_t width;
	uint16_t height;
	HWND hWnd;
	std::unique_ptr<Graphics> pGfx;
	std::unique_ptr<Engine> sEng;
	std::thread windowThread;
};


#define CHWND_EXCEPT( hr ) Window::HrException((hr), __LINE__,__FILE__)
#define CHWND_LAST_EXCEPT() Window::HrException( GetLastError(),__LINE__,__FILE__)

