#include "window.h"
#include "renderer.h"
#include "texture.h"
#include "audio.h"
#include "timer.h"

bool IGameWindow::init(const GameWindowConfig& config)
{
	_config = config;
	_running = true;
	_lastFrameBeginTp = timeNow();
	_updateDt = 1.0 / config.updateFrequency;
	_accumulator = 0;

	if(config.maxFps > 0) {
		_maxFrameTime = 1.0 / (f64)config.maxFps;
	}

	assert(_updateDt >= _maxFrameTime);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, config.openGL_profile);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, config.openGL_majorVersion);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, config.openGL_minorVersion);

	_pWindow = SDL_CreateWindow(
		config.title.c_str(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		config.windowWidth,
		config.windowHeight,
		SDL_WINDOW_OPENGL | config.windowFlags
	);

	if(!_pWindow) {
		lsk_errf("Error: can't create SDL2 window (%s)",  SDL_GetError());
		return false;
	}

	_glContext = SDL_GL_CreateContext(_pWindow);
	if(!_glContext) {
		lsk_errf("Error: can't create OpenGL 3.3 context (%s)",  SDL_GetError());
		return false;
	}

	SDL_GL_SetSwapInterval(config.vsync ? 1:0);

	if(gl3w_init()) {
		lsk_errf("Error: can't init gl3w");
		return false;
	}

	if(!gl3w_is_supported(config.openGL_majorVersion, config.openGL_minorVersion)) {
		lsk_errf("Error: OpenGL %d.%d isn't available on this system",
				 config.openGL_majorVersion, config.openGL_minorVersion);
		return false;
	}

	// OpenGL base state
	glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

	// culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, config.windowWidth, config.windowHeight);

	/*glGetIntegerv(GL_NV_MEMORY_DEDICATED, &_gpuMemTotal);
	glGetIntegerv(GL_NV_MEMORY_AVAILABLE, &_gpuMemAvailStart);*/

	Textures.init();

	if(!Renderer.init()) {
		lsk_errf("Error: failed to init Renderer");
		return false;
	}
	Renderer.viewResize(config.windowWidth, config.windowHeight);

	if(!AudioGet.init()) {
		lsk_errf("Error: failed to init Renderer");
		return false;
	}

	if(!postInit()) {
		return false;
	}

	return true;
}

void IGameWindow::exit()
{
	preExit();
	AudioGet.destroy();
	Renderer.destroy();
	Textures.destroy();

	if(_glContext) SDL_GL_DeleteContext(_glContext);
	if(_pWindow) SDL_DestroyWindow(_pWindow);
	lsk_succf("exit success");
}

bool IGameWindow::handleEvent(SDL_Event event)
{
	if(event.type == SDL_WINDOWEVENT) {
		if(event.window.event == SDL_WINDOWEVENT_CLOSE) {
			_running = false;
			return false;
		}
	}

	if(event.type == SDL_QUIT) {
		_running = false;
		return false;
	}

	// key down
	if(event.type == SDL_KEYDOWN) {
		if(event.key.keysym.sym == SDLK_ESCAPE) {
			_running = false;
			return false;
		}
	}

	return true;
}

void IGameWindow::update(f64 delta)
{
	AudioGet.update();
	Timers.udpate(delta);
}

void IGameWindow::render()
{
	Renderer.render();
}

void IGameWindow::run()
{
	if(!_running) {
		lsk_errf("Error: init window before running");
		return;
	}
	defer(exit(););

	SDL_Event event;
	while(_running) {
		while(SDL_PollEvent(&event)) {
			if(!handleEvent(event)) {
				break;
			}
		}

		f64 elapsed = timeDurSince(_lastFrameBeginTp);
		if(elapsed < _maxFrameTime) {
			lsk_sleep((_maxFrameTime - elapsed) * 1000);
			elapsed = _maxFrameTime;
		}
		else if(elapsed > _updateDt * 2) {
			elapsed = _updateDt * 2; // cap it
		}
		_lastFrameBeginTp = timeNow();
		_accumulator += elapsed;

		while(_accumulator > _updateDt) {
			_accumulator -= _updateDt;

			u32 windowFlags = SDL_GetWindowFlags(_pWindow);
			_windowActive = !(windowFlags&SDL_WINDOW_MINIMIZED) && windowFlags&SDL_WINDOW_SHOWN;

			if(_windowActive) {
				Renderer.beginFrame();
				update(_updateDt);
				Renderer.endFrame();
			}
		}

		if(_windowActive) {
			render();
			SDL_GL_SwapWindow(_pWindow);
		}

		// simple fps check
		++_fps;
		if(timeDurSince(_fpsDisplayTp) > 1.f) {
			lsk_printf("ft: %.5fms [%d]", 1000.f/_fps, _fps);
			_fpsDisplayTp = timeNow();
			_fps = 0;
		}
	}
}
