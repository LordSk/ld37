#pragma once
#include <lsk/lsk_array.h>
#include <lsk/lsk_math.h>

struct BodyRectAligned
{
	lsk_AABB2 box;
	lsk_Vec2 vel = {};
	u16 x_locked = false;
	u16 y_locked = false;
	i32 group = 0;
	void* pUserData = nullptr;

	inline void setPos(const lsk_Vec2& pos) {
		lsk_Vec2 size = box.max - box.min;
		box.min = pos;
		box.max = pos + size;
	}

	BodyRectAligned(f32 sizeX, f32 sizeY) {
		box.min	= {0, 0};
		box.max = {sizeX, sizeY};
	}
};

struct CollisionInfo
{
	BodyRectAligned* pBodyA;
	BodyRectAligned* pBodyB;
	lsk_Vec2 pushVec;
	u8 boxB_static = false;
};

bool intersectTest(const lsk_AABB2& A, const lsk_AABB2& B, lsk_Vec2* out_pPushVec);

struct PhysicsManager
{
	SINGLETON_IMP(PhysicsManager)

	lsk_Vec2 gravity = {0, 10.f};
	lsk_DSparseArray<BodyRectAligned> bodiesDynamic;
	lsk_DSparseArray<BodyRectAligned> bodiesStatic;

	void init();
	void destroy();

	void update(f64 delta);
};

#define Physics PhysicsManager::get()
