#include "ld37.h"
#include "ordinator.h"
#include <lsk/lsk_console.h>
#include <engine/physics.h>


bool LD37_Window::postInit()
{
	Ord.init();
	Physics.init();

	Renderer.viewResize(320, 180);

	assets.init();
	if(!assets.open("../assets/assets.lsk_arch")) {
		return false;
	}

	assets.loadData();

	ArchiveFile& mapFile = assets.fileStrMap.geth(H("map1.json"))->get();
	if(!gamemap.load((const char*)mapFile.buffer.ptr)) {
		return false;
	}

	gamemap.initForDrawing();

	return true;
}

void LD37_Window::preExit()
{
	assets.deinit();
	Physics.destroy();
	Ord.destroy();
}

void LD37_Window::update(f64 delta)
{
	IGameWindow::update(delta);
	Ord.update(delta);
	Physics.update(delta);

	gamemap.draw();
}

void LD37_Window::render()
{
	glClear(GL_COLOR_BUFFER_BIT);
	IGameWindow::render();
}

#ifdef _WIN32
i32 CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
i32 main()
#endif
{
	lsk_printf("Ludum Dare 37");

	AllocDefault_set(&GMalloc);

	LD37_Window window;
	GameWindowConfig config;
	config.windowWidth = 1280;
	config.windowHeight = 720;
	config.title = "Am I awake?";
	if(!window.init(config)) {
		return 1;
	}
	window.run();
	return 0;
}
