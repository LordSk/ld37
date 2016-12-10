#include "ld37.h"
#include "ordinator.h"
#include <lsk/lsk_console.h>
#include <engine/timer.h>
#include <engine/audio.h>

#define SKELETON_AGGRO_RANGE 180.f

enum: i32 {
	BODYGROUP_PLAYER = 0,
	BODYGROUP_SKELETON,
	BODYGROUP_BOSS,
};

Actor::Actor()
{
	transform = Ord.make_Transform();
	transform->position.z = 1;
	bodyComp = Ord.make_CBodyComponent();
	bodyComp->transform = transform;
}

void Actor::setPos(lsk_Vec2 pos)
{
	bodyComp->body->setPos(pos);
	transform->position.x = pos.x;
	transform->position.y = pos.y;
}


void DamageFieldManager::init()
{
	fields.init(32);
}

void DamageFieldManager::destroy()
{
	fields.destroy();
}

void DamageFieldManager::update(f64 delta)
{
	for(i32 i = 0; i < fields.count(); ++i) {
		fields[i].lifetime -= delta;
		if(fields[i].lifetime <= 0) {
			fields.remove(i);
		}
	}
}

void damageFieldCreate(const lsk_Vec2& pos, const lsk_Vec2& size, DamageGroup dmgGroup, f64 lifetime,
					   const lsk_Vec2& sourcePos)
{
	DamageField field;
	field.box.min = pos;
	field.box.max = pos + size;
	field.dmgGroup = dmgGroup;
	field.lifetime = lifetime;
	field.sourcePos = sourcePos;
	DamageFieldManager::get().fields.push(field);
}

void CHealth::takeDamage(const DamageField& source)
{
	if(dmgCooldown <= 0.0) {
		--health;
		dmgCooldown = dmgCooldownMax;
		lastDamageTime = Timers.getTime();
		lastSource = source;
	}
}

void CHealth::update(f64 delta)
{
	dmgCooldown -= delta;

	for(const auto& field: DamageFieldManager::get().fields) {
		if(field.dmgGroup != dmgGroup && intersectTest(body->box, field.box, nullptr)) {
			takeDamage(field);
		}
	}
}

APlayer::APlayer()
{
	healthComp = Ord.make_CHealth();
	healthComp->dmgGroup = DamageGroup::PLAYER;
	sprite = Ord.make_Sprite();
	sprite->transform = transform;
	sprite->materialName = H("explorer_still_right.material");
	sprite->size = {16, 38};
}

void APlayer::beginPlay()
{
	bodyComp->init({16, 38}, BODYGROUP_PLAYER);
	healthComp->body = bodyComp->body;
}

void APlayer::update(f64 delta)
{
	Actor::update(delta);

	// knockback from damage
	bool stunned = false;
	if(healthComp->lastDamageTime != 0 && Timers.getTime() - healthComp->lastDamageTime < 0.15) {
		f32 dir = 1;
		if(healthComp->lastSource.sourcePos.x - transform->position.x > 0) {
			dir = -1;
		}

		bodyComp->body->vel.x = dir * 200.f;
		bodyComp->body->vel.y = -100.f;
		stunned = true;
	}

	if(!stunned) {
		f32 xSpeed = 75.f;
		f32 jumpSpeed = 220.f;

		if(input.x == 1) {
			bodyComp->body->vel.x = xSpeed;
			sprite->materialName = H("explorer_still_right.material");
		}
		else if(input.x == -1) {
			bodyComp->body->vel.x = -xSpeed;
			sprite->materialName = H("explorer_still_left.material");
		}
		else {
			bodyComp->body->vel.x = 0;
		}

		if(input.jump && !prevInput.jump) {
			bool canJump = false;
			if(isGrounded()) {
				canJump = true;
				doubleJumps = 1;
			}
			else if(doubleJumps > 0) {
				--doubleJumps;
				canJump = true;
			}

			if(canJump) {
				bodyComp->body->vel.y = -jumpSpeed;
			}
		}
	}

	prevInput = input;
}

bool APlayer::isGrounded() const
{
	return bodyComp->body->intersecting && bodyComp->body->vel.y >= 0 && bodyComp->body->y_locked;
}

ASkeleton::ASkeleton()
{
	bodySize = {14, 34};
	target = Ord.make_CTarget();
	healthComp = Ord.make_CHealth();
	healthComp->dmgGroup = DamageGroup::ENEMY;
	attackRange = 18;

	nextRandomGruntCD = nextRandomGruntCD_min +
			lsk_randf() * (nextRandomGruntCD_max - nextRandomGruntCD_min);

	sndGruntNameHashes.push(H("snd_skeleton_grunt1.ogg"));
	sndGruntNameHashes.push(H("snd_skeleton_grunt2.ogg"));

	sndAttackNameHashes.push(H("snd_skeleton_attack1.ogg"));
	sndAttackNameHashes.push(H("snd_skeleton_attack2.ogg"));
	sndAttackNameHashes.push(H("snd_skeleton_attack3.ogg"));
}

void ASkeleton::beginPlay()
{
	bodyComp->init(bodySize, BODYGROUP_SKELETON);
	healthComp->body = bodyComp->body;
}

void ASkeleton::update(f64 delta)
{
	Actor::update(delta);

	input = {};
	attackAnimCooldown -= delta;
	nextRandomGruntCD -= delta;

	if(nextRandomGruntCD <= 0.0 && attackAnimCooldown <= 0) {
		nextRandomGruntCD = nextRandomGruntCD_min +
				lsk_randf() * (nextRandomGruntCD_max - nextRandomGruntCD_min);
		AudioGet.play(sndGruntNameHashes[lsk_rand() % sndGruntNameHashes.count()]);
	}

	// attack !
	f32 targetXDelta = target->pos.x - transform->position.x;
	if(lsk_abs(targetXDelta) < SKELETON_AGGRO_RANGE) {
		input.x = lsk_sign(targetXDelta);
		if(lsk_abs(targetXDelta) < attackRange) {
			if(dir == input.x) {
				input.x = 0;
			}
			input.attack = 1;
		}
	}

	bool canAdvance = dir == input.x;
	if(input.x != 0 && input.x != dir) {
		turnCooldown -= delta;
		if(turnCooldown < 0.0) {
			canAdvance = true;
		}
	}

	if(attackAnimCooldown > 0) {
		canAdvance = false;
	}

	if(input.x == 1) {
		if(canAdvance) {
			bodyComp->body->vel.x = xSpeed;
			dir = 1;
			turnCooldown = turnCooldownMax;
		}
	}
	else if(input.x == -1) {
		if(canAdvance) {
			bodyComp->body->vel.x = -xSpeed;
			dir = -1;
			turnCooldown = turnCooldownMax;
		}
	}
	else {
		bodyComp->body->vel.x = 0;
	}

	// actual attack
	if(attackAnimCooldown > 0 && attackAnimCooldown < attackTime) {
		lsk_printf("SMACK!");
		attack();
		AudioGet.play(sndAttackNameHashes[lsk_rand() % sndAttackNameHashes.count()]);
		attackTime = 0;
	}

	// attack animation
	if(input.attack && attackAnimCooldown <= 0.0) {
		attackAnimCooldown = attackAnimCooldownMax;
		attackTime = attackTimeMax;
		// play attack animation
	}
}

void ASkeleton::attack()
{
	lsk_Vec2 pos = {transform->position.x, transform->position.y};
	f32 xOffset = 14.f;
	if(dir == -1) xOffset = -20.f;
	lsk_Vec2 fieldPos = pos;
	fieldPos.x += xOffset;

	damageFieldCreate(fieldPos, {20, 20}, DamageGroup::ENEMY, 0.2, pos);
}

ASkeletonBigShield::ASkeletonBigShield()
{
	bodySize = {20, 40};
	healthComp->maxHealth = 4;
	healthComp->health = 4;

	xSpeed = 40.f;
	attackAnimCooldownMax = 2.0;
	turnCooldownMax = 1.0;
	attackRange = 30;
	attackTimeMax = 1.0;

	sndGruntNameHashes.clear();
	sndGruntNameHashes.push(H("snd_skeleton_big_grunt1.ogg"));
	sndGruntNameHashes.push(H("snd_skeleton_big_grunt2.ogg"));
	sndGruntNameHashes.push(H("snd_skeleton_big_grunt3.ogg"));

	sndAttackNameHashes.clear();
	sndAttackNameHashes.push(H("snd_skeleton_big_attack1.ogg"));
	sndAttackNameHashes.push(H("snd_skeleton_big_attack2.ogg"));
	sndAttackNameHashes.push(H("snd_skeleton_big_attack3.ogg"));
}

void ASkeletonBigShield::attack()
{
	lsk_Vec2 pos = {transform->position.x, transform->position.y};
	f32 xOffset = bodySize.x;
	if(dir == -1) xOffset = -20.f;
	lsk_Vec2 fieldPos = pos;
	fieldPos.x += xOffset;

	damageFieldCreate(fieldPos, {20, 40}, DamageGroup::ENEMY, 0.2, pos);
}

bool LD37_Window::postInit()
{
	Ord.init();
	Physics.init();
	DamageFieldManager::get().init();

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

	for(const auto& layer: gamemap.tileLayers) {
		if(H(layer.name.c_str()) == H("foreground")) {
			for(i32 y = 0; y < layer.height; ++y) {
				i32 chain_startX = -1;
				i32 chain_endX = -1;
				for(i32 x = 0; x < layer.width; ++x) {
					if(layer.data[y * layer.width + x] != 0) {
						if(chain_startX == -1) chain_startX = x;
					}
					else if(chain_startX != -1) {
						chain_endX = x;
						f32 sizeX = (chain_endX - chain_startX) * 14;
						auto body = Physics.bodiesStatic.push(BodyRectAligned(sizeX, 14));
						body->setPos({chain_startX * 14.f, y * 14.f});
						chain_startX = -1;
						chain_endX = -1;
					}
				}

				if(chain_startX != -1) {
					chain_endX = layer.width;
					f32 sizeX = (chain_endX - chain_startX) * 14;
					auto body = Physics.bodiesStatic.push(BodyRectAligned(sizeX, 14));
					body->setPos({chain_startX * 14.f, y * 14.f});
				}
			}

			break;
		}
	}

	player = Ord.spawn_APlayer();
	player->beginPlay();

	for(const auto& layer: gamemap.objectLayers) {
		for(const auto& obj: layer.objects) {
			if(H(obj.type.c_str()) == H("player_spawn")) {
				player->setPos({(f32)obj.x, (f32)obj.y});
			}
			else if(H(obj.type.c_str()) == H("skeleton_spawn")) {
				auto skeleton = Ord.spawn_ASkeletonBigShield();
				skeleton->beginPlay();
				skeleton->setPos({(f32)obj.x, (f32)obj.y});
			}
		}
	}

	return true;
}

void LD37_Window::preExit()
{
	Ord.destroy();
	DamageFieldManager::get().destroy();
	Physics.destroy();
	assets.deinit();
}

void LD37_Window::update(f64 delta)
{
	Physics.update(delta);
	IGameWindow::update(delta);
	DamageFieldManager::get().update(delta);
	Ord.update(delta);

	Renderer.viewSetPos(player->bodyComp->body->box.min.x - 50, 0);

	gamemap.draw();

#ifdef CONF_DEBUG
	if(debugCollisions) {
		for(const auto& dynBody: Physics.bodiesDynamic) {
			Renderer.queueSprite(H("body_dynamic.material"), 1000, dynBody.box.min,
								 dynBody.box.max - dynBody.box.min);
		}
		for(const auto& statBody: Physics.bodiesStatic) {
			Renderer.queueSprite(H("body_static.material"), 1000, statBody.box.min,
								 statBody.box.max - statBody.box.min);
		}
		for(const auto& field: DamageFieldManager::get().fields) {
			Renderer.queueSprite(H("body_static.material"), 1000, field.box.min,
								 field.box.max - field.box.min);
		}
	}
#endif

	for(auto& comp: Ord._comp_CTarget) {
		comp.pos.x = player->transform->position.x;
		comp.pos.y = player->transform->position.y;
	}
}

void LD37_Window::render()
{
	glClear(GL_COLOR_BUFFER_BIT);
	IGameWindow::render();
}

bool LD37_Window::handleEvent(SDL_Event event)
{
	if(!IGameWindow::handleEvent(event)) {
		return false;
	}

	if(event.type == SDL_KEYDOWN) {
		if(event.key.keysym.sym == SDLK_d) {
			player->input.x = 1;
			return true;
		}
		if(event.key.keysym.sym == SDLK_q ||
		   event.key.keysym.sym == SDLK_a) {
			player->input.x = -1;
			return true;
		}
		if(event.key.keysym.sym == SDLK_SPACE) {
			player->input.jump = 1;
			return true;
		}
	}

	if(event.type == SDL_KEYUP) {
		if(event.key.keysym.sym == SDLK_d) {
			if(player->input.x == 1) {
				player->input.x = 0;
			}
			return true;
		}
		if(event.key.keysym.sym == SDLK_q ||
		   event.key.keysym.sym == SDLK_a) {
			if(player->input.x == -1) {
				player->input.x = 0;
			}
			return true;
		}
		if(event.key.keysym.sym == SDLK_SPACE) {
			player->input.jump = 0;
			return true;
		}

		if(event.key.keysym.sym == SDLK_c) {
			debugCollisions ^= 1;
			return true;
		}
	}

	return true;
}

#ifdef _WIN32
i32 CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
i32 main()
#endif
{
	lsk_printf("Ludum Dare 37");
	AllocDefault_set(&GMalloc);

	i32 sdlInit = SDL_Init(SDL_INIT_VIDEO);
	if(sdlInit == -1) {
		lsk_errf("Error: %s", SDL_GetError());
		return 1;
	}
	defer(SDL_Quit());

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

void CBodyComponent::init(const lsk_Vec2& size, i32 bodyGroup)
{
	body = Physics.bodiesDynamic.push(BodyRectAligned(size.x, size.y));
	body->group = bodyGroup;
}

void CBodyComponent::update(f64 delta)
{
	assert(body.valid());
	transform->position.x = body->box.min.x;
	transform->position.y = body->box.min.y;
}

void CBodyComponent::endPlay()
{
	assert(body.valid());
	Physics.bodiesDynamic.remove(body);
}
