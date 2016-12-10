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

	void setPos(lsk_Vec2 pos);
	virtual void update(f64 delta) override;
	void endPlay() override;
};

struct Input
{
	i32 x = 0;
	i32 jump = 0;
	i32 attack = 0;
};

struct ENTITY APlayer: Actor
{
	Input prevInput;
	Input input;
	i32 doubleJumps = 0;

	i32 maxHealth = 2;
	i32 health = maxHealth;

	APlayer();

	void beginPlay() override;
	void update(f64 delta) override;

	bool canJump() const;
	bool isGrounded() const;
	void takeDamage();
};

struct COMPONENT CTarget
{
	lsk_Vec2 pos;

	void update(f64 delta) {}
	void endPlay() {}
};

struct ENTITY ASkeleton: Actor
{
	Ref<CTarget> target;
	Input input;
	f64 attackCooldown = 0;

	ASkeleton();

	void beginPlay() override;
	void update(f64 delta) override;
};


struct LD37_Window: IGameWindow
{
	Archive assets;
	TiledMap gamemap;
	Ref<APlayer> player;

	bool postInit() override;
	void preExit() override;
	void update(f64 delta) override;
	void render() override;
	bool handleEvent(SDL_Event event) override;
};
