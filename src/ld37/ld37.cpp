#include "ld37.h"
#include "ordinator.h"
#include <lsk/lsk_console.h>
#include <engine/timer.h>
#include <engine/audio.h>
#include <time.h>

#define SKELETON_AGGRO_RANGE 220.f
#define GLOBAL_VOLUME 0.5

#define EXPLORER_IDLE_SIZEX 14
#define EXPLORER_RUNNING_SIZEX 26
#define EXPLORER_PUNCH_SIZEX 24

#define SKELETON_IDLE_SIZEX 20
#define SKELETON_RUNNING_SIZEX 33
#define SKELETON_ATTACK_SIZEX 45

#define SKELETON_BIG_IDLE_SIZEX 27
#define SKELETON_RUNNING_SIZEX 27
#define SKELETON_ATTACK_SIZEX 45

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

void damageFieldCreate(const lsk_Vec2& pos, const lsk_Vec2& size, DamageGroup dmgGroup,
					   const lsk_Vec2& sourcePos)
{
	DamageField field;
	field.box.min = pos;
	field.box.max = pos + size;
	field.dmgGroup = dmgGroup;
	field.lifetime = 0.1;
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

		if(source.dmgGroup == DamageGroup::ENEMY) {
			AudioGet.play(H("snd_enemy_hit.ogg"));
		}
		else {
			AudioGet.play(H("snd_punch_hit.ogg"));
		}
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
	sprite->materialName = H("explorer_idle.material");
	sprite->size = {14, 38};
}

void APlayer::beginPlay()
{
	bodyComp->init({14, 38}, BODYGROUP_PLAYER);
	healthComp->body = bodyComp->body;
}

void APlayer::update(f64 delta)
{
	Actor::update(delta);

	attackCD -= delta;

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
			sprite->materialName = H("explorer_running.material");
			sprite->localPos.x = -8;
			sprite->size.x = EXPLORER_RUNNING_SIZEX;
			dir = 1;
		}
		else if(input.x == -1) {
			bodyComp->body->vel.x = -xSpeed;
			sprite->materialName = H("explorer_running.material");
			sprite->localPos.x = EXPLORER_RUNNING_SIZEX - 4;
			sprite->size.x = -EXPLORER_RUNNING_SIZEX;
			dir = -1;
		}
		else {
			bodyComp->body->vel.x = 0;
			sprite->materialName = H("explorer_idle.material");
			if(dir == 1) {
				sprite->localPos.x = 0;
				sprite->size.x = EXPLORER_IDLE_SIZEX;
			}
			else {
				sprite->localPos.x = EXPLORER_IDLE_SIZEX;
				sprite->size.x = -EXPLORER_IDLE_SIZEX;
			}
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

		if(input.attack && attackCD <= 0.0) {
			attack();
			pPunchAnim->reset();
			attackCD = attackCDMax;
		}
	}

	if(attackCD > attackCDMax - 0.25) {
		sprite->materialName = H("explorer_punch.material");
		if(dir == 1) {
			sprite->localPos.x = 0;
			sprite->size.x = EXPLORER_PUNCH_SIZEX;
		}
		else {
			sprite->localPos.x = EXPLORER_PUNCH_SIZEX - 8;
			sprite->size.x = -EXPLORER_PUNCH_SIZEX;
		}
	}

	prevInput = input;
}

bool APlayer::isGrounded() const
{
	return bodyComp->body->intersecting && bodyComp->body->vel.y >= 0 && bodyComp->body->y_locked;
}

void APlayer::attack()
{
	lsk_Vec2 pos = {transform->position.x, transform->position.y};
	f32 xOffset = 4.f;
	if(dir == -1) xOffset = -20.f;
	lsk_Vec2 fieldPos = pos;
	fieldPos.x += xOffset;

	damageFieldCreate(fieldPos, {30, 30}, DamageGroup::PLAYER, pos);
}

ASkeleton::ASkeleton()
{
	bodySize = {14, 37};
	target = Ord.make_CTarget();
	healthComp = Ord.make_CHealth();
	healthComp->dmgGroup = DamageGroup::ENEMY;
	sprite = Ord.make_Sprite();
	sprite->transform = transform;
	sprite->size = {20, 37};
	sprite->materialName = H("skeleton_idle.material");

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

	if(healthComp->isDead()) {
		die();
		destroy();
		return;
	}

	input = {};
	attackAnimCooldown -= delta;
	nextRandomGruntCD -= delta;
	knockbackCD -= delta;

	// knockback from damage
	if(healthComp->lastDamageTime != 0 && Timers.getTime() - healthComp->lastDamageTime < 0.15 &&
	   knockbackCD <= 0.0) {
		knockbackCD = 0.5;

		f32 dir = 1;
		if(healthComp->lastSource.sourcePos.x - transform->position.x > 0) {
			dir = -1;
		}

		bodyComp->body->vel.x = dir * 180.f * knockbackMultiplier;
		bodyComp->body->vel.y = -100.f * knockbackMultiplier;
	}

	if(knockbackCD > 0) {
		bodyComp->body->vel.x *= 0.9;
		if(lsk_abs(bodyComp->body->vel.x) < 10.f) {
			bodyComp->body->vel.x = 0;
		}
		return;
	}

	f32 targetXDelta = target->pos.x - transform->position.x;

	if(lsk_abs(targetXDelta) < 400.f) {
		nextRandomGruntCD -= delta;
		if(nextRandomGruntCD <= 0.0 && attackAnimCooldown <= 0) {
			nextRandomGruntCD = nextRandomGruntCD_min +
					lsk_randf() * (nextRandomGruntCD_max - nextRandomGruntCD_min);
			AudioGet.play(sndGruntNameHashes[lsk_rand() % sndGruntNameHashes.count()],
					0.3f + lsk_randf() * 0.3f);
		}
	}

	// attack !
	if(lsk_abs(targetXDelta) < SKELETON_AGGRO_RANGE) {
		input.x = lsk_sign(targetXDelta);
		if(lsk_abs(targetXDelta) < attackRange) {
			if(dir == input.x) {
				input.x = 0;
			}
			input.attack = 1;
		}
	}

	canAdvance = dir == input.x;
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
			playRun();
		}
		else {
			playIdle();
		}
	}
	else if(input.x == -1) {
		if(canAdvance) {
			bodyComp->body->vel.x = -xSpeed;
			dir = -1;
			turnCooldown = turnCooldownMax;
			playRun();
		}
		else {
			playIdle();
		}
	}
	else {
		bodyComp->body->vel.x = 0;
		playIdle();
	}

	// actual attack
	if(attackAnimCooldown > 0 && attackAnimCooldown < attackAnimCooldownMax - attackTime) {
		lsk_printf("SMACK!");
		attack();
		AudioGet.play(sndAttackNameHashes[lsk_rand() % sndAttackNameHashes.count()]);
		attackTime = 10000;
	}

	if(attackAnimCooldown > 0 && attackAnimCooldown > attackAnimCooldownMax - attackTimeMax) {
		playAttack();
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

	damageFieldCreate(fieldPos, {20, 20}, DamageGroup::ENEMY, pos);
}

void ASkeleton::die()
{
	constexpr u32 dieSounds[] = {
		H("snd_skeleton_die1.ogg"),
		H("snd_skeleton_die2.ogg"),
		H("snd_skeleton_die3.ogg")
	};
	AudioGet.play(dieSounds[lsk_rand() % 3], 0.5f + lsk_randf() * 0.3f);
}

void ASkeleton::playIdle()
{
	sprite->materialName = H("skeleton_idle.material");
	if(dir == 1) {
		sprite->localPos.x = 0;
		sprite->size.x = SKELETON_IDLE_SIZEX;
	}
	else {
		sprite->localPos.x = SKELETON_IDLE_SIZEX - 6;
		sprite->size.x = -SKELETON_IDLE_SIZEX;
	}
}

void ASkeleton::playRun()
{
	if(dir == 1) {
	sprite->materialName = H("skeleton_running.material");
	sprite->localPos.x = -9;
	sprite->size.x = SKELETON_RUNNING_SIZEX;
	}
	else {
		sprite->materialName = H("skeleton_running.material");
		sprite->localPos.x = SKELETON_RUNNING_SIZEX - 12;
		sprite->size.x = -SKELETON_RUNNING_SIZEX;
	}
}

void ASkeleton::playAttack()
{
	sprite->materialName = H("skeleton_attack.material");
	if(dir == 1) {
		sprite->localPos.x = 0;
		sprite->size.x = SKELETON_ATTACK_SIZEX;
	}
	else {
		sprite->localPos.x = SKELETON_ATTACK_SIZEX - 20;
		sprite->size.x = -SKELETON_ATTACK_SIZEX;
	}
}

ASkeletonBigShield::ASkeletonBigShield()
{
	bodySize = {20, 40};
	sprite->size.y = 40;
	healthComp->maxHealth = 4;
	healthComp->health = 4;

	xSpeed = 40.f;
	attackAnimCooldownMax = 2.0;
	turnCooldownMax = 1.0;
	attackRange = 30;
	attackTimeMax = 0.2;

	knockbackMultiplier = 0.5;

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

	damageFieldCreate(fieldPos, {20, 40}, DamageGroup::ENEMY, pos);
}

void ASkeletonBigShield::die()
{
	constexpr u32 dieSounds[] = {
		H("snd_skeleton_big_die1.ogg"),
		H("snd_skeleton_big_die2.ogg"),
		H("snd_skeleton_big_die3.ogg")
	};
	AudioGet.play(dieSounds[lsk_rand() % 3], 0.5f + lsk_randf() * 0.3f);
}

void ASkeletonBigShield::playIdle()
{
	sprite->materialName = H("skeleton_big_idle.material");
	if(dir == 1) {
		sprite->localPos.x = 0;
		sprite->size.x = SKELETON_BIG_IDLE_SIZEX;
	}
	else {
		sprite->localPos.x = SKELETON_BIG_IDLE_SIZEX - 6;
		sprite->size.x = -SKELETON_BIG_IDLE_SIZEX;
	}
}

void ASkeletonBigShield::playRun()
{
	sprite->materialName = H("skeleton_big_running.material");
	if(dir == 1) {
		sprite->localPos.x = 0;
		sprite->size.x = SKELETON_RUNNING_SIZEX;
	}
	else {
		sprite->localPos.x = SKELETON_RUNNING_SIZEX - 6;
		sprite->size.x = -SKELETON_RUNNING_SIZEX;
	}
}

void MaterialAnimation::update(f64 delta)
{
	if(paused) return;
	_time += delta;
	i32 frameCount = 1.f / pMat->uvParams.z;
	i32 curFrame = (i32)(_time / frameTime) % frameCount;
	pMat->uvParams.x = curFrame;
}

bool LD37_Window::postInit()
{
	glDisable(GL_CULL_FACE);

	AudioGet._soloud.setGlobalVolume(GLOBAL_VOLUME);
	Ord.init();
	Physics.init();
	DamageFieldManager::get().init();

	Renderer.viewResize(320, 180);

	time_t t = time(0);
	lsk_randSetSeed(t);

	assets.init();
	if(!assets.open("../assets/assets.lsk_arch")) {
		return false;
	}

	assets.loadData();

	// material animations
	matAnims.init(80);

	MaterialAnimation anim;
	anim.pMat = &Renderer.materials.getTextured(H("explorer_idle.material"));
	anim.frameTime = 0.75f;
	matAnims.push(anim);

	anim.pMat = &Renderer.materials.getTextured(H("explorer_running.material"));
	anim.frameTime = 0.15f;
	matAnims.push(anim);

	anim.pMat = &Renderer.materials.getTextured(H("explorer_punch.material"));
	anim.frameTime = 0.125f;
	pPunchAnim = &matAnims.push(anim);

	anim.pMat = &Renderer.materials.getTextured(H("skeleton_idle.material"));
	anim.frameTime = 0.75f;
	matAnims.push(anim);

	anim.pMat = &Renderer.materials.getTextured(H("skeleton_running.material"));
	anim.frameTime = 0.15f;
	matAnims.push(anim);

	anim.pMat = &Renderer.materials.getTextured(H("explorer_death.material"));
	anim.frameTime = 0.5f;
	pDeathAnim = &matAnims.push(anim);

	anim.pMat = &Renderer.materials.getTextured(H("explorer_wake.material"));
	anim.frameTime = 0.5f;
	pWakeAnim = &matAnims.push(anim);

	anim.pMat = &Renderer.materials.getTextured(H("skeleton_attack.material"));
	anim.frameTime = 0.15f / 2.f;
	matAnims.push(anim);

	anim.pMat = &Renderer.materials.getTextured(H("skeleton_big_running.material"));
	anim.frameTime = 0.2f;
	matAnims.push(anim);

	// load tiledmap
	ArchiveFile& mapFile = assets.fileStrMap.geth(H("map1.json"))->get();
	if(!gamemap.load((const char*)mapFile.buffer.ptr)) {
		return false;
	}

	gamemap.initForDrawing();

	// map collision
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

	for(const auto& layer: gamemap.objectLayers) {
		for(const auto& obj: layer.objects) {
			if(H(obj.type.c_str()) == H("player_spawn")) {
				playerSpawnPos = {(f32)obj.x, (f32)obj.y};
			}
		}
	}

	//start_preGame();
	start_spawn();

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
	for(auto& anim: matAnims) {
		anim.update(delta);
	}

	Physics.update(delta);
	IGameWindow::update(delta);
	DamageFieldManager::get().update(delta);
	Ord.update(delta);

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

	switch(gamestate) {
		case GAMESTATE_PREGAME: update_preGame(delta); break;
		case GAMESTATE_SPAWN: update_spawn(delta); break;
		case GAMESTATE_EXPLORE: update_explore(delta); break;
		case GAMESTATE_CHALICE_SUMMON: update_chaliceSummon(delta); break;
		case GAMESTATE_BOSS: update_boss(delta); break;
		case GAMESTATE_DEFEAT: update_defeat(delta); break;
		case GAMESTATE_VICTORY: update_victory(delta); break;
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

	if(gamestate == GAMESTATE_EXPLORE || gamestate == GAMESTATE_BOSS) {
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
			if(event.key.keysym.sym == SDLK_SPACE ||
			   event.key.keysym.sym == SDLK_w ||
			   event.key.keysym.sym == SDLK_z) {
				player->input.jump = 1;
				return true;
			}
			if(event.key.keysym.sym == SDLK_f) {
				player->input.attack = 1;
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
			if(event.key.keysym.sym == SDLK_SPACE ||
			   event.key.keysym.sym == SDLK_w ||
			   event.key.keysym.sym == SDLK_z) {
				player->input.jump = 0;
				return true;
			}
			if(event.key.keysym.sym == SDLK_f) {
				player->input.attack = 0;
				return true;
			}



			if(event.key.keysym.sym == SDLK_c) {
				debugCollisions ^= 1;
				return true;
			}
		}
	}

	return true;
}

void LD37_Window::start_preGame()
{
	gamestate = GAMESTATE_PREGAME;

	Timers.add(10.0, [&] {
		start_spawn();
	});
}

void LD37_Window::start_spawn()
{
	gamestate = GAMESTATE_SPAWN;

	Renderer.viewSetPos(0, 0);
	pWakeAnim->reset();
	pWakeAnim->paused = 0;

	Timers.add(1.4, [&] {
		pWakeAnim->paused = 1;
	});

	Timers.add(3, [&] {
		start_explore();
	});

	for(auto& skel: Ord._entity_ASkeleton) {
		skel.destroy();
	}

	for(auto& skel: Ord._entity_ASkeletonBigShield) {
		skel.destroy();
	}

	AudioGet._soloud.stopAll();
}

void LD37_Window::start_explore()
{
	gamestate = GAMESTATE_EXPLORE;

	player = Ord.spawn_APlayer();
	player->beginPlay();
	player->setPos(playerSpawnPos);
	player->healthComp->maxHealth = 4;
	player->healthComp->health = 4;
	player->pPunchAnim = pPunchAnim;

	// spawn skeletons
	for(const auto& layer: gamemap.objectLayers) {
		for(const auto& obj: layer.objects) {
			if(H(obj.type.c_str()) == H("skeleton_spawn")) {
				i32 r = lsk_rand()%2;
				if(r == 0) {
					auto skeleton = Ord.spawn_ASkeleton();
					skeleton->beginPlay();
					skeleton->setPos({(f32)obj.x, (f32)obj.y});
				}
				else {
					auto skeleton = Ord.spawn_ASkeletonBigShield();
					skeleton->beginPlay();
					skeleton->setPos({(f32)obj.x, (f32)obj.y});
				}
			}
		}
	}
}

void LD37_Window::start_chaliceSummon()
{
	gamestate = GAMESTATE_CHALICE_SUMMON;
	player->input = {};
	player->healthComp->health = player->healthComp->maxHealth;

	for(auto& skel: Ord._entity_ASkeleton) {
		skel.destroy();
	}

	for(auto& skel: Ord._entity_ASkeletonBigShield) {
		skel.destroy();
	}

	Timers.add(2.0, [&] {
		start_boss();
	});

	AudioGet._soloud.stopAll();
	AudioGet.play(H("snd_chalice_summon.ogg"));
}

void LD37_Window::start_boss()
{
	gamestate = GAMESTATE_BOSS;
}

void LD37_Window::start_defeat()
{
	gamestate = GAMESTATE_DEFEAT;
	lastPlayerPos = {
		player->transform->position.x,
		player->transform->position.y
	};
	player->destroy();

	pDeathAnim->paused = 0;
	pDeathAnim->reset();

	Timers.add(1.2, [&] {
		pDeathAnim->paused = 1;
	});

	Timers.add(4.0, [&] {
		start_spawn();
	});

	lastStateChangeTime = Timers.getTime();

	AudioGet.play(H("snd_death.ogg"));
}

void LD37_Window::start_victory()
{
	gamestate = GAMESTATE_VICTORY;
}

void LD37_Window::update_preGame(f64 delta)
{
	Renderer.queueSprite(H("story.material"), 1000, {0, 0}, {320, 180});
}

void LD37_Window::update_spawn(f64 delta)
{
	Renderer.queueSprite(H("explorer_wake.material"), 1000, {playerSpawnPos.x, playerSpawnPos.y-1},
		{38, 38});
	Renderer.queueSprite(H("text_dream.material"), 1000, {160-70, 180-28}, {140, 28});
}

void LD37_Window::update_explore(f64 delta)
{
	if(player->healthComp->isDead()) {
		lastPlayerPos = {
			player->transform->position.x,
			player->transform->position.y
		};
		player->destroy();
		start_defeat();
		return;
	}

	camX = lsk_clamp(player->bodyComp->body->box.min.x - 120.f, 0.f, gamemap.width*14.f - 320);
	Renderer.viewSetPos(camX, 0);

	for(auto& comp: Ord._comp_CTarget) {
		comp.pos.x = player->transform->position.x;
		comp.pos.y = player->transform->position.y;
	}

	i32 h = 0;
	for(; h < player->healthComp->health; ++h) {
		Renderer.queueSprite(H("heart_full.material"), 100, {camX + 4.f + 14.f * h, 4}, {14, 14});
	}

	for(;h < player->healthComp->maxHealth; ++h) {
		Renderer.queueSprite(H("heart_empty.material"), 100, {camX + 4.f + 14.f * h, 4}, {14, 14});
	}

	if(player->transform->position.x > (182*14.f)) {
		start_chaliceSummon();
	}
}

void LD37_Window::update_chaliceSummon(f64 delta)
{
	Renderer.queueSprite(H("text_chalice.material"), 1000, {camX + 160-70, 180-28}, {140, 28});
}

void LD37_Window::update_boss(f64 delta)
{
	if(player->healthComp->isDead()) {
		start_defeat();
		return;
	}

	camX = lsk_clamp(player->bodyComp->body->box.min.x - 120.f, 0.f, gamemap.width*14.f - 320);
	Renderer.viewSetPos(camX, 0);

	i32 h = 0;
	for(; h < player->healthComp->health; ++h) {
		Renderer.queueSprite(H("heart_full.material"), 100, {camX + 4.f + 14.f * h, 4}, {14, 14});
	}

	for(;h < player->healthComp->maxHealth; ++h) {
		Renderer.queueSprite(H("heart_empty.material"), 100, {camX + 4.f + 14.f * h, 4}, {14, 14});
	}
}

void LD37_Window::update_defeat(f64 delta)
{
	if((Timers.getTime() - lastStateChangeTime) > 2.5) {
		Renderer.queueSprite(H("black.material"), 1000, {camX, 0}, {320, 180});
	}
	else {
		Renderer.queueSprite(H("explorer_death.material"), 1000, {lastPlayerPos.x, lastPlayerPos.y-1},
			{38, 38});
	}
}

void LD37_Window::update_victory(f64 delta)
{

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
