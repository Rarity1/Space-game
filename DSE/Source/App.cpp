#include "App.h"

App::App()
	:
	wnd(1000, 750, "Game Window")
{
};

int App::Go() {
	wnd.Eng().iLoad();
	while (Alive.load()) {
		//Process messages each cycle
		if (wnd.ProcessMessages() == WM_QUIT) {
			Alive.store(false);
			return 0;
		}
		App::DoFrame();
	}
	return 0;
}
//Misnomer This is a cycle. 
void App::DoFrame() {
	auto delta = wnd.Eng().Clock.Mark();
	wnd.Eng().Update();
	updaterate += delta;
	//This is a tick
	if (updaterate >= 1.0) {
		wnd.SetTitle(std::to_string(wnd.Eng().phyx->ticker.cGet()));
		wnd.Eng().phyx->ticker.reset();
		updaterate = 0.0;
	}
	//Technically this is a frame
	wnd.Gfx().RenderFrame();

}
