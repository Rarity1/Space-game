#include "Window.h"
#include "Engine.h"
#include "DLLGui.h"
#include <cassert>
#include <functional>



Window::Window(uint16_t w, uint16_t h, const char *name, HINSTANCE hInstance)
    : width(w), height(h) {

  std::unique_lock loc(winWait);
  Context = this;
  // calculate window size based on desired client region size
  WindowRect.wr = WRect::WindowRect{0, 0, width, height};
  // create window & get hWnd
  WNDCLASSEX wc = {
      .cbSize = sizeof(wc),
      .style = CS_HREDRAW | CS_VREDRAW,
      .lpfnWndProc = StaticHandleMsg,
      .cbClsExtra = 0,
      .cbWndExtra = 0,
      .hInstance = hInstance,
      .hIcon = static_cast<HICON>(LoadImage(
          hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, 0)),
      .hCursor = nullptr,
      .hbrBackground = CreateSolidBrush(0),
      .lpszMenuName = nullptr,
      .lpszClassName = name,
      .hIconSm = static_cast<HICON>(LoadImage(
          hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0))};
  assert(RegisterClassEx(&wc));
  auto WindowStyle = WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION | WS_VISIBLE;
  AdjustWindowRect((LPRECT)&WindowRect.wr, WindowStyle, FALSE);
  hWnd = CreateWindow(name, name, WindowStyle, CW_USEDEFAULT, CW_USEDEFAULT,
                   WindowRect.wr.right - WindowRect.wr.left,
                   WindowRect.wr.bottom - WindowRect.wr.top, nullptr, nullptr,
                   GetModuleHandle(NULL), this);
   GetClientRect(hWnd, (LPRECT)&WindowRect.wr);

  ShowWindow(hWnd, SW_SHOWDEFAULT);
  UpdateWindow(hWnd);

  sEng = std::make_unique<Engine>(InputHndlr, WindowRect, hWnd);
  pGfx = sEng->pGfx.get();

  #ifndef IMGUI_DISABLE
  ImGuiHnd = std::bind(&imguid::ImGuiProcHndl, pGfx->iGui.get(), std::placeholders::_1,
     std::placeholders::_2,
      std::placeholders::_3,
      std::placeholders::_4);
  // newly created windows start off as hidden
  #endif
  loc.unlock();
  std::unique_lock exelock(upLock);
  sEng->iLoad();
  std::thread UpdateThread([this]() {
    while (Alive.load())
      sEng->Update();
  });
  while (GetMessage(&msg, NULL, 0, 0)) {
    // Translate Message will post auxilliary WM_CHAR messages from key msgs
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  Alive.store(false);

  // Move Message processing to own thread
  UpdateThread.join();
  exelock.unlock();
}

Window::~Window()
{
	//windowThread.join();
  ImGuiHnd = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){return DefWindowProc( hWnd,  msg,  wParam,  lParam);};
	sEng.reset();
	DestroyWindow(hWnd);
}

void Window::SetTitle(const std::string& title)
{
	assert(SetWindowTextA(hWnd, title.c_str()) == 0);
}


LRESULT Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg) {
	case WM_NCCREATE:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_CREATE:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_NCCALCSIZE:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_KILLFOCUS:
		InputHndlr.keyboard.ClearState();
		break;

	case WM_SIZING:
	case WM_SIZE:
		WindowRect.Mtx.lock();
		//GetClientRect(hWnd, (LPRECT)&WindowRect.wr);
    //WindowRect.Updated.store(true);
		WindowRect.Mtx.unlock();
    

		break;
	//Keyboard Messages
  using EType = Input::Event::Type;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		InputHndlr.keyboard.UpdateKey(EType::Press, wParam);
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		InputHndlr.keyboard.UpdateKey(EType::Release, wParam);
		break;
	case WM_CHAR:
		InputHndlr.keyboard.UpdateKey(EType::Character, wParam);
		break;
		//End Keyboard messages
		//Mouse Messages
	
  /*
    case WM_MOUSEMOVE:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		//In client region -> log move, and log enter + capture mouse (if not previously captured?
		if (pt.x >= 0 && pt.x < width && pt.y >= 0 && pt.y < height)
		{
			InputHndlr.mouse.OnMouseMove(pt.x, pt.y);
			if (!InputHndlr.mouse.IsInWindow())
			{
				SetCapture(hWnd);
				InputHndlr.mouse.OnMouseEnter();
			}
		}
		// Not in client -> log move / maintain capture if button down
		else {
			if (wParam & (MK_LBUTTON | MK_RBUTTON))
			{
				InputHndlr.mouse.OnMouseMove(pt.x, pt.y);
			}
			//button up -> release capture / log event for leaving
			else {
				ReleaseCapture();
				InputHndlr.mouse.OnMouseLeave();
			}
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		InputHndlr.mouse.OnLeftPressed(pt.x, pt.y);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		InputHndlr.mouse.OnRightPressed(pt.x, pt.y);
		break;
	}
	case WM_LBUTTONUP:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		InputHndlr.mouse.OnLeftReleased(pt.x, pt.y);
		break;
	}
	case WM_RBUTTONUP:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		InputHndlr.mouse.OnRightReleased(pt.x, pt.y);
		break;
	}
	case WM_MOUSEWHEEL:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		InputHndlr.mouse.OnWheelDelta(pt.x, pt.y, delta);
		break;
		//End Mouse Messaging
	}
  */

	}
	#ifndef IMGUI_DISABLE
  ImGuiHnd(hWnd, msg, wParam, lParam);
	#endif
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

