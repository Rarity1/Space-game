#include "App.h"

App::App()
	:
	wnd(1280, 720, "Game Window")
{
};

int App::Go() {
	while (Alive.load()) {
		//Process messages each frame
		if (wnd.ProcessMessages() == WM_QUIT) {
			Alive.store(false);
			return 0;
		}
		App::DoFrame();
	}
	return 0;
}

void App::DoFrame() {
	if (launch) {
		wnd.Eng().iLoad();
		launch = false;
	}
	auto delta = wnd.Eng().Clock.Mark();
	wnd.Eng().Update(delta);
	updaterate += delta;
	if (updaterate >= 1.0) {
		wnd.SetTitle(std::to_string(wnd.Eng().phyx->ticker.cGet()));
		wnd.Eng().phyx->ticker.reset();
		updaterate = 0.0;
	}
	wnd.Gfx().RenderFrame();

}
