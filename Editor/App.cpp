#include "App.h"

App::App()
{
	wnd = std::make_unique<Window>(1280, 720, "Game Window", Alive);
};

int App::Go() {
	{
		std::unique_lock loc(wnd->winWait);
		wnd->winReady.wait(loc, [this] {return Alive.load(); });
	}
	wnd->sEng->iLoad();
	while (Alive.load()) {
		wnd->Update();
		App::DoFrame();
	}
	return 0;
}

void App::DoFrame() {
	wnd->sEng->Update();
	wnd->sEng->RenderI();
}
