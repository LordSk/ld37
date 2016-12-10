#pragma once
#include "meta.h"
#include <lsk/lsk_math.h>
#include <lsk/lsk_array.h>

struct IEntityBase
{
	u32 _entityId = 0;
	bool _markedAsDestroy = false;

	inline void destroy() {
		_markedAsDestroy = true;
	}

	virtual void beginPlay() {}
	virtual void update(f64 delta) {}
	virtual void endPlay() {}

protected:
	IEntityBase() {}
};

struct COMPONENT Transform
{
	lsk_Vec3 position;
	lsk_Vec3 scale = {1, 1, 1};
	lsk_Quat rotation;

	void beginPlay() {}
	void update(f32 delta) {}
	void endPlay() {}
};

struct COMPONENT Sprite
{
	Ref<Transform> transform;
	u32 materialName = 0;
	lsk_Vec2 localPos = {};
	lsk_Vec2 origin = {};
	lsk_Vec2 size = {1, 1};

	void beginPlay() {}
	void update(f64 delta);
	void endPlay() {}
};
