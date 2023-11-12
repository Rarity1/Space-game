#include "App.h"

App::App()
	:
	wnd(1000, 750, "Game Window")
{
};

int App::Go() {
	while (wnd.ProcessMessages() != WM_QUIT) {
		App::DoFrame();
	}
	return 0;
}

void App::DoFrame() {
	if (timer.Peek() > 16) {
		wnd.Eng().Update();
		timer.Mark();
	}
	if(!wnd.Eng().engInit)
	wnd.Gfx().RenderFrame();
	else {
		wnd.Eng().iLoad();
	}
}
