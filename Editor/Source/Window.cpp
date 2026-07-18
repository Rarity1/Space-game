#include "Window.h"
#include <windows.h>



static Window * Context;
class StaticFunc{
	friend class Window;
	static LRESULT Function(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
		return Context->HandleMsg(hWnd, msg, wParam, lParam);
	};
};

Window::Window(uint16_t w, uint16_t h, const char *name, HINSTANCE hInstance)
    : width(w), height(h) {

  std::unique_lock loc(winWait);

  auto Name = "WEEEE";
  // calculate window size based on desired client region size
  WindowRect.wr = RECT{0, 0, width, height};
  pGfx = std::make_unique<Graphics>(WindowRect);
  // create window & get hWnd
  Context = this;
  WNDCLASSEX wc = {
      .cbSize = sizeof(wc),
      .style = CS_HREDRAW | CS_VREDRAW,
      .lpfnWndProc = StaticFunc::Function,
      .cbClsExtra = 0,
      .cbWndExtra = 0,
      .hInstance = hInstance,
      .hIcon = static_cast<HICON>(LoadImage(
          hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, 0)),
      .hCursor = nullptr,
      .hbrBackground = CreateSolidBrush(0),
      .lpszMenuName = nullptr,
      .lpszClassName = Name,
      .hIconSm = static_cast<HICON>(LoadImage(
          hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0))};
  assert(RegisterClassEx(&wc));
  auto WindowStyle = WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION | WS_VISIBLE;
  AdjustWindowRect(&WindowRect.wr, WindowStyle, FALSE);
  hWnd = CreateWindow(Name, "Game Window", WindowStyle, CW_USEDEFAULT, CW_USEDEFAULT,
                   WindowRect.wr.right - WindowRect.wr.left,
                   WindowRect.wr.bottom - WindowRect.wr.top, nullptr, nullptr,
                   GetModuleHandle(NULL), this);


  (ShowWindow(hWnd, SW_SHOWDEFAULT));
  UpdateWindow(hWnd);
  pGfx->LoadPipeline(hWnd);
  sEng = std::make_unique<Engine>(*pGfx, kbd, clock);

  // newly created windows start off as hidden


  loc.unlock();
  std::unique_lock exelock(upLock);
  sEng->iLoad();
  std::thread UpdateThread([this](){while(Alive.load())sEng->Update();});
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
	pGfx.reset();
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
		kbd.ClearState();
		break;

	case WM_SIZING:
	case WM_SIZE:
		WindowRect.Mtx.lock();
		GetClientRect(hWnd, &WindowRect.wr);
		WindowRect.Mtx.unlock();
		pGfx->updateResolution.store(true);

		break;
	//Keyboard Messages
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (!(lParam & 0x40000000) || kbd.AutorepeatIsEnabled())
		{
			kbd.OnKeyPressed(static_cast<unsigned char>(wParam));
		}
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		kbd.OnKeyReleased(wParam);
		break;
	case WM_CHAR:
		kbd.OnChar(wParam);
		break;
		//End Keyboard messages
		//Mouse Messages
	case WM_MOUSEMOVE:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		//In client region -> log move, and log enter + capture mouse (if not previously captured?
		if (pt.x >= 0 && pt.x < width && pt.y >= 0 && pt.y < height)
		{
			mouse.OnMouseMove(pt.x, pt.y);
			if (!mouse.IsInWindow())
			{
				SetCapture(hWnd);
				mouse.OnMouseEnter();
			}
		}
		// Not in client -> log move / maintain capture if button down
		else {
			if (wParam & (MK_LBUTTON | MK_RBUTTON))
			{
				mouse.OnMouseMove(pt.x, pt.y);
			}
			//button up -> release capture / log event for leaving
			else {
				ReleaseCapture();
				mouse.OnMouseLeave();
			}
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnLeftPressed(pt.x, pt.y);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnRightPressed(pt.x, pt.y);
		break;
	}
	case WM_LBUTTONUP:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnLeftReleased(pt.x, pt.y);
		break;
	}
	case WM_RBUTTONUP:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnRightReleased(pt.x, pt.y);
		break;
	}
	case WM_MOUSEWHEEL:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		mouse.OnWheelDelta(pt.x, pt.y, delta);
		break;
		//End Mouse Messaging
	}
	}
	#ifndef IMGUI_DISABLE
	pGfx->iGui->ImGuiProcHndl(hWnd, msg, wParam, lParam);
	#endif
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

