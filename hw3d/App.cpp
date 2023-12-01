#include "App.h"

App::App()
	:
	wnd(1000, 750, "Game Window")
{
};

int App::Go() {
	wnd.Eng().iLoad();
	while (wnd.ProcessMessages() != WM_QUIT) {
		App::DoFrame();
	}
	return 0;
}

void App::DoFrame() {
	EngThread = wnd.Eng().Update(timer.Peek());
	timer.Mark();
	wnd.Gfx().OnUpdate();
	wnd.Gfx().RenderFrame();
	EngThread.join();
}
