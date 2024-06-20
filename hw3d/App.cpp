#include "App.h"

App::App()
	:
	wnd(1000, 750, "Game Window")
{
};

int App::Go() {
	wnd.Eng().iLoad();
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
	wnd.Eng().timer.mtx.lock();
	wnd.Eng().timer.time = timer.Peek()/1000;
	wnd.Eng().timer.mtx.unlock();
	timer.Mark();
	wnd.Eng().Update();
	wnd.Gfx().RenderFrame();

}
