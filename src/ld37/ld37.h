#pragma once
#include <engine/window.h>

struct LD37_Window: IGameWindow
{



	bool postInit() override;
	void preExit() override;
	void update(f64 delta) override;
	void render() override;
};
