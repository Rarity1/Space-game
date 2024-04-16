#include "App.h"

App::App()
	:
	wnd(1000, 750, "Game Window")
{
};

int App::Go() {
	wnd.Eng().iLoad();
	while (wnd.ProcessMessages() != WM_QUIT) {
		wnd.Eng().DoStuff();
		App::DoFrame();
	}
	return 0;
}

void App::DoFrame() {
	wnd.Eng().timer.mtx.lock();
	wnd.Eng().timer.time += timer.Peek()/1000;
	wnd.Eng().timer.mtx.unlock();
	timer.Mark();
	wnd.Gfx().RenderFrame();

}
