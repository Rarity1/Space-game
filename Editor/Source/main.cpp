
#include "Window.h"
#include "Exceptions.h"
#if defined(_WIN32)
#include "CWin.h"


//Main Class

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow){
	try {
		Window APP(1920, 1080, "Game Window", hInstance);
		return 0;
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
#else

int main(int argc, char* argv[])
{
try{
	return App{}.Go();
}catch(const Exceptions& e){
		MessageBoxA(nullptr, e.what(), "Standard Exception", MB_OK | MB_ICONEXCLAMATION);
}catch (...){
		MessageBoxA(nullptr, "No details available", "Unknown Exception", MB_OK | MB_ICONEXCLAMATION);
}
return -1;
}
#endif // _WIN32



