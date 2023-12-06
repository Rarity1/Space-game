#include "Window.h"



Window::WindowClass Window::WindowClass::wndClass;

Window::WindowClass::WindowClass():
	hInst(GetModuleHandle(nullptr))
{
	WNDCLASSEX wc = { 
		.cbSize = sizeof (wc),
		.style= CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc= HandleMsgSetup,
		.cbClsExtra= 0,
		.cbWndExtra= 0,
		.hInstance= hInst,
		.hIcon = static_cast<HICON>(LoadImage(
		hInst, MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON, 32, 32, 0
	)),
		.hCursor= nullptr,
		.hbrBackground = CreateSolidBrush(0),
		.lpszMenuName = nullptr,
		.lpszClassName = GetName(),
		.hIconSm = static_cast<HICON>(LoadImage(
		hInst, MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON, 16, 16, 0
	))
	};
	
	RegisterClassEx(&wc) >> chk;
}

Window::WindowClass::~WindowClass()
{
	UnregisterClassA( wndClassName, GetInstance() );
}

const char* Window::WindowClass::GetName() noexcept
{
	return wndClassName;
}

HMODULE Window::WindowClass::GetInstance() noexcept
{
	return wndClass.hInst;
}

Window::Window(int width, int height, const char* name)
	:
	width(width),
	height(height)
{
	
	// calculate window size based on desired client region size
	RECT wr = { 0, 0, (LONG)width, (LONG)height };
	AdjustWindowRect(&wr, WS_OVERLAPPED | 
		WS_CAPTION | 
		WS_SYSMENU | 
		WS_THICKFRAME | 
		WS_MINIMIZEBOX | 
		WS_MAXIMIZEBOX, FALSE);
	// create window & get hWnd
	hWnd = CreateWindow(
		WindowClass::GetName(), name,
		WS_OVERLAPPED |
		WS_CAPTION |
		WS_SYSMENU |
		WS_THICKFRAME |
		WS_MINIMIZEBOX |
		WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,wr.right - wr.left,wr.bottom - wr.top,
		nullptr, nullptr, WindowClass::GetInstance(), this
	);
	// newly created windows start off as hidden

	pGfx = std::make_shared<Graphics>(&hWnd, height, width);
	sEng = std::make_unique<Engine>(pGfx.get(), &kbd);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	//Create graphics object
	
}

Window::~Window()
{
	DestroyWindow(hWnd);
}

void Window::SetTitle(const std::string& title)
{
	if (SetWindowTextA(hWnd, title.c_str()) == 0)
	{
		throw CHWND_LAST_EXCEPT();
	}
}

std::optional<WPARAM> Window::ProcessMessages() {
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		//Translate Message will post auxilliary WM_CHAR messages from key msgs
		if (msg.message == WM_QUIT)
		{
			return msg.message;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return {};
}

Graphics& Window::Gfx()
{
	return *pGfx;
}

Engine& Window::Eng()
{
	return *sEng;
}

LRESULT CALLBACK Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//Use create parameter passed in from CreateWindow() to store the class pointer for the window.
	if (msg == WM_NCCREATE)
	{
		//Extract PTR to window class from creation data
		const CREATESTRUCT* const pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
		//Set the WinAPI-managed user data to store ptr to window class
		SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd) );
		//Set message proc to normal (non-setup) handler now that setup is finished
		SetWindowLongPtr( hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMsgThunk));
		//Forward message to window class handler
		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}
}
LRESULT CALLBACK Window::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//Retrieve ptr to window class
	Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	//Forward message to window class handler
	return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}
LRESULT Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_KILLFOCUS:
		kbd.ClearState();
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
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

//Window Exception
std::string Window::Exception::TranslateErrorCode(HRESULT hr) noexcept
{
	char* pMsgBuf = nullptr;
	// windows will allocate memory for err string and make our pointer point to it
	const DWORD nMsgLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&pMsgBuf), 0, nullptr
	);
	// 0 string length returned indicates a failure
	if (nMsgLen == 0)
	{
		return "Unidentified error code";
	}
	// copy error string from windows-allocated buffer to std::string
	std::string errorString = pMsgBuf;
	// free windows buffer
	LocalFree(pMsgBuf);
	return errorString;
}
Window::HrException::HrException(HRESULT hr, int line, const char* file) noexcept
	:
	Exception(line, file),
	hr( hr )
{}
const char* Window::HrException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << '\n'
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode() << '\n'
		<< "[Description] " << GetErrorDescription() << '\n'
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}
const char* Window::HrException::GetType() const noexcept
{
	return "Demo Window Exception";
}
HRESULT Window::HrException::GetErrorCode() const noexcept
{
	return hr;
}
std::string Window::HrException::GetErrorDescription() const noexcept
{
	return Exception::TranslateErrorCode(hr);
}
