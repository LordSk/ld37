#pragma once
#define SDL_MAIN_HANDLED // handle main on our own
#include <SDL2/SDL.h>
#include <external/gl3w.h>
#include <lsk/lsk_string.h>
#include <lsk/lsk_utils.h>

struct GameWindowConfig
{
	lsk_DStr256 title;
	SDL_GLprofile openGL_profile = SDL_GL_CONTEXT_PROFILE_CORE;
	i32 openGL_majorVersion = 3;
	i32 openGL_minorVersion = 3;
	u32 windowWidth = 1280;
	u32 windowHeight = 720;
	i32 windowFlags = 0; // SDL_WindowFlags, SDL_WINDOW_OPENGL is forced
	bool vsync = false;
	i32 maxFps = -1;
	f64 updateFrequency = 60.0;
};

struct IGameWindow
{
	GameWindowConfig _config;
	bool _running = false;
	bool _windowActive = true;
	SDL_Window* _pWindow = nullptr;
	SDL_GLContext _glContext = nullptr;
	timept _lastFrameBeginTp, _fpsDisplayTp;
	f64 _updateDt = 1;
	f64 _accumulator = 0;
	f64 _maxFrameTime = 0;
	i32 _fps = 0;

	virtual bool init(const GameWindowConfig& config);
	virtual void exit();
	virtual bool handleEvent(SDL_Event event);
	virtual void update(f64 delta);
	virtual void render();
	virtual void run();

	virtual bool postInit() { return true;}
	virtual void preExit() {}
};
