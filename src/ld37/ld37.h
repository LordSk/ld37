#pragma once
#include <engine/window.h>
#include <engine/archive.h>
#include <engine/tiledmap.h>
#include <engine/base_entity.h>
#include <engine/physics.h>

struct COMPONENT CBodyComponent
{
	Ref<Transform> transform;
	Ref<BodyRectAligned> body;

	void init(const lsk_Vec2& size, i32 bodyGroup);
	void update(f64 delta);
	void endPlay();
};

struct ENTITY Actor: IEntityBase
{
	Ref<Transform> transform;
	Ref<CBodyComponent> bodyComp;

	Actor();

	void setPos(lsk_Vec2 pos);
};

enum class DamageGroup: i32 {
	INVALID = 0,
	NEUTRAL,
	PLAYER,
	ENEMY
};

struct DamageField
{
	lsk_AABB2 box;
	lsk_Vec2 sourcePos;
	DamageGroup dmgGroup;
	f64 lifetime;
};

struct DamageFieldManager
{
	SINGLETON_IMP(DamageFieldManager)

	lsk_DArray<DamageField> fields;

	void init();
	void destroy();

	void update(f64 delta);
};

void damageFieldCreate(const lsk_Vec2& pos, const lsk_Vec2& size, DamageGroup dmgGroup,
					   const lsk_Vec2& sourcePos);

struct COMPONENT CHealth
{
	Ref<BodyRectAligned> body;
	i32 maxHealth = 2;
	i32 health = maxHealth;
	DamageGroup dmgGroup = DamageGroup::INVALID;
	f64 dmgCooldownMax = 0.3;
	f64 dmgCooldown = 0.0;
	f64 lastDamageTime = 0.0;
	DamageField lastSource;

	void takeDamage(const DamageField& source);
	void update(f64 delta);
	void endPlay() {}

	inline bool isDead() const {
		return health <= 0;
	}
};

struct Input
{
	i32 x = 0;
	i32 jump = 0;
	i32 attack = 0;
};

struct ENTITY APlayer: Actor
{
	Ref<Sprite> sprite;
	Ref<CHealth> healthComp;
	Input prevInput;
	Input input;
	i32 dir = 1;
	i32 doubleJumps = 0;

	f64 attackCD = 0;
	f64 attackCDMax = 0.5;
	struct MaterialAnimation* pPunchAnim = nullptr;

	APlayer();

	void beginPlay() override;
	void update(f64 delta) override;

	bool canJump() const;
	bool isGrounded() const;
	void attack();
};

struct COMPONENT CTarget
{
	lsk_Vec2 pos;

	void update(f64 delta) {}
	void endPlay() {}
};

struct ENTITY ASkeleton: Actor
{
	lsk_Vec2 bodySize;
	Ref<CTarget> target;
	Ref<CHealth> healthComp;
	Ref<Sprite> sprite;
	Input input;
	i32 dir = -1;

	f32 xSpeed = 50.f;
	f64 attackAnimCooldownMax = 1.5;
	f64 attackAnimCooldown = 0;
	f64 attackTime = 0;
	f64 attackTimeMax = 0.15;
	f32 attackRange = 18;
	f64 turnCooldownMax = 0.5;
	f64 turnCooldown = 0;

	f64 nextRandomGruntCD_min = 20.0;
	f64 nextRandomGruntCD_max = 40.0;
	f64 nextRandomGruntCD;
	lsk_Array<u32, 3> sndGruntNameHashes;
	lsk_Array<u32, 3> sndAttackNameHashes;

	f32 knockbackMultiplier = 1.f;
	f64 knockbackCD = 0;

	bool canAdvance = true;

	ASkeleton();

	void beginPlay() override;
	void update(f64 delta) override;
	virtual void attack();
	virtual void die();

	virtual void playIdle();
	virtual void playRun();
	virtual void playAttack();
};

struct ENTITY ASkeletonBigShield: ASkeleton
{
	ASkeletonBigShield();

	//void update(f64 delta) override;
	void attack() override;
	void die() override;
	void playIdle() override;
	void playRun() override;
	void playAttack() override;
};

struct ENTITY ADragon: IEntityBase
{
	const lsk_Array<lsk_Vec2, 16>* pDragonPath;
	f32 damageCD = 0;
	i32 curPathId = -1;
	lsk_Vec2 pos;

	struct Part {
		i32 alive = true;
		i32 pathId = 0;
		lsk_Vec2 pos = {};
		Ref<BodyRectAligned> body;
	};
	lsk_Array<Part, 8> parts;
	Ref<BodyRectAligned> headBody;


	ADragon();
	void update(f64 delta) override;
	void place(const lsk_Array<lsk_Vec2, 16>* pDragonPath_);
	void endPlay() override;
	bool isDead();
};

struct MaterialAnimation
{
	Shader_Textured::Material* pMat;
	f64 _time = 0;
	f32 frameTime;
	i32 paused = false;

	void update(f64 delta);
	inline void reset() { _time = 0; }
};

struct LD37_Window: IGameWindow
{
	Archive assets;
	TiledMap gamemap;
	Ref<APlayer> player;
	bool debugCollisions = true;
	lsk_DArray<MaterialAnimation> matAnims;

	MaterialAnimation* pPunchAnim;
	MaterialAnimation* pDeathAnim;
	MaterialAnimation* pWakeAnim;

	f32 camX;
	lsk_Vec2 playerSpawnPos;
	lsk_Vec2 lastPlayerPos;

	f64 lastStateChangeTime = 0;

	enum {
		GAMESTATE_PREGAME = 0,
		GAMESTATE_SPAWN,
		GAMESTATE_EXPLORE,
		GAMESTATE_CHALICE_SUMMON,
		GAMESTATE_BOSS,
		GAMESTATE_DEFEAT,
		GAMESTATE_VICTORY
	};

	i32 gamestate = GAMESTATE_PREGAME;

	lsk_Array<lsk_Vec2, 16> dragonPath;
	Ref<ADragon> dragon;

	bool postInit() override;
	void preExit() override;
	void update(f64 delta) override;
	void render() override;
	bool handleEvent(SDL_Event event) override;

	void start_preGame();
	void start_spawn();
	void start_explore();
	void start_chaliceSummon();
	void start_boss();
	void start_defeat();
	void start_victory();

	void update_preGame(f64 delta);
	void update_spawn(f64 delta);
	void update_explore(f64 delta);
	void update_chaliceSummon(f64 delta);
	void update_boss(f64 delta);
	void update_defeat(f64 delta);
	void update_victory(f64 delta);
};
