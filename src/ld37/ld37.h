#pragma once
#include <engine/window.h>
#include <engine/archive.h>
#include <engine/tiledmap.h>

struct LD37_Window: IGameWindow
{
	Archive assets;
	TiledMap gamemap;

	bool postInit() override;
	void preExit() override;
	void update(f64 delta) override;
	void render() override;
};
