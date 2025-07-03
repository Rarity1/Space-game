#pragma once


#include <sstream>
#include "DLLImports.h"
#include "../ImGui/imgui.h"



//Window Class
class Window {
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
	Window(int width, int height, const char* name);
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	void SetTitle(const std::string& title);
	static std::optional<WPARAM> ProcessMessages();
	Graphics& Gfx();
	Engine& Eng();

private:
	static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	Keyboard kbd;
	EngineTime clock;
	Mouse mouse;
private:
	int width;
	int height;
	HWND hWnd;
	std::shared_ptr<Graphics> pGfx;
	std::shared_ptr<imguid> iGui;
	std::unique_ptr<Engine> sEng;
};


#define CHWND_EXCEPT( hr ) Window::HrException((hr), __LINE__,__FILE__)
#define CHWND_LAST_EXCEPT() Window::HrException( GetLastError(),__LINE__,__FILE__)

