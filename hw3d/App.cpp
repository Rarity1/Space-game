#include "App.h"

App::App()
	:
	wnd(1000, 750, "Game Window")
{};

int App::Go() {
	wnd.Gfx().OnInit();
	
	while (wnd.ProcessMessages() != WM_QUIT) {
		App::DoFrame();
	}
	return 0;
}

void App::DoFrame() {
		wnd.Gfx().OnUpdate();
		wnd.Gfx().RenderFrame();
}
