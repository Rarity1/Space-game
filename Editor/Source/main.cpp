#include "Window.h"
#include "App.h"
#include <string>
#include <sstream>



int gameMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow) {
	try {
		return App{}.Go();
	}
	catch (const Exceptions& e)
	{
		MessageBoxA(nullptr, e.what(), e.GetType(), MB_OK | MB_ICONEXCLAMATION);
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Standard Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	catch (...)
	{
		MessageBoxA(nullptr, "No details available", "Unknown Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	return -1;
}

//Main Class
#ifdef _WIN32
int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow) {
	return gameMain(hInstance,
		hPrevInstance,
		lpCmdLine,
		nCmdShow);
}
#else

int CALLBACK main(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	return gameMain(hInstance,
		hPrevInstance,
		lpCmdLine,
		nCmdShow);
}
#endif // _WIN32



