#pragma once
#include <engine/window.h>
#include <engine/archive.h>
#include <engine/tiledmap.h>
#include <engine/base_entity.h>
#include <engine/physics.h>

struct ENTITY Actor: IEntityBase
{
	Ref<Transform> transform;
	Ref<BodyRectAligned> body;
	Ref<Sprite> body_debugSprite;

	Actor();

	void initBody(lsk_Vec2 pos, lsk_Vec2 size);
	void update(f64 delta) override;
	void endPlay() override;
};




struct LD37_Window: IGameWindow
{
	Archive assets;
	TiledMap gamemap;

	bool postInit() override;
	void preExit() override;
	void update(f64 delta) override;
	void render() override;
};
