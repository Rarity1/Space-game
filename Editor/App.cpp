#include "App.h"

App::App()
	:
	wnd(1280, 720, "Game Window")
{
};

int App::Go() {
	wnd.sEng->iLoad();
	while (Alive.load()) {
		//Process messages each frame
		if (wnd.ProcessMessages() == WM_QUIT) {
			//Add Graceful shutdown here
			Alive.store(false);
			break;
		}
		App::DoFrame();
	}
	return 0;
}

void App::DoFrame() {

	wnd.sEng->tMain->gEndWork(graphicsWref);
	if (wnd.sEng->Update()) {
		delta += wnd.sEng->engineTimeTaken;
	}
	if (delta >= 1) {
		wnd.SetTitle(std::to_string(wnd.sEng->phyx->ticker.cGet()));
		wnd.sEng->phyx->ticker.reset();
		delta = wnd.sEng->engineTimeTaken;
	}
	graphicsWref = wnd.sEng->tMain->gPushWork([this] {	wnd.pGfx->RenderFrame(wnd.sEng->tracker->getInstance(1));});
}
