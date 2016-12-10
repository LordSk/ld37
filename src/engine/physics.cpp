#include "physics.h"

bool intersectTest(const lsk_AABB2& A, const lsk_AABB2& B, lsk_Vec2* out_pPushVec)
{
	// x axis ------------
	f32 d0 = B.max.x - A.min.x;
	f32 d1 = B.min.x - A.max.x;

	// no overlap
	if(d0 < 0.0f || d1 > 0.0f) {
		return false;
	}

	f32 depth = d0 < -d1 ? d0 : d1;
	lsk_Vec2 pvx = {depth, 0.f};

	// y axis ------------
	d0 = B.max.y - A.min.y;
	d1 = B.min.y - A.max.y;

	// no overlap
	if(d0 < 0.0f || d1 > 0.0f) {
		return false;
	}

	depth = d0 < -d1 ? d0 : d1;
	lsk_Vec2 pvy = {0.f, depth};

	if(out_pPushVec) {
		*out_pPushVec = lsk_lengthSq(pvx) < lsk_lengthSq(pvy) ? pvx : pvy;
	}

	return true;
}

void PhysicsManager::init()
{
	bodiesDynamic.init(32);
	bodiesStatic.init(32);
}

void PhysicsManager::destroy()
{
	bodiesDynamic.destroy();
	bodiesStatic.destroy();
}

void PhysicsManager::update(f64 delta)
{
	for(auto& body: bodiesDynamic) {
		body.vel += gravity;
		body.box.min += body.vel * delta;
		body.box.max += body.vel * delta;
	}

	lsk_DArray<CollisionInfo> collisions(64);
	bool resolveCollisions = true;
	for(i32 step = 0; step < 4 && resolveCollisions; ++step) {
		resolveCollisions = false;
		collisions.clear();

		const i32 dynamicCount = bodiesDynamic.count();
		const i32 staticCount = bodiesStatic.count();

		for(i32 i = 0; i < dynamicCount; ++i) {
			BodyRectAligned& bodyA = bodiesDynamic.data()[i];

			for(i32 j = 0; j < staticCount; ++j) {
				BodyRectAligned& bodyB = bodiesStatic.data()[j];
				lsk_Vec2 pushVec;

				if(intersectTest(bodyA.box, bodyB.box, &pushVec)) {
					CollisionInfo coll;
					coll.pBodyA = &bodyA;
					coll.pBodyB = &bodyB;
					coll.boxB_static = true;
					coll.pushVec = pushVec * 1.0001f; // add a little epsilon
					collisions.push(coll);

					resolveCollisions = true;
				}
			}

			for(i32 j = 0; j < dynamicCount; ++j) {
				if(i == j) continue;
				BodyRectAligned& bodyB = bodiesDynamic.data()[j];
				lsk_Vec2 pushVec;

				if(intersectTest(bodyA.box, bodyB.box, &pushVec)) {
					CollisionInfo coll;
					coll.pBodyA = &bodyA;
					coll.pBodyB = &bodyB;
					coll.boxB_static = false;
					coll.pushVec = pushVec * 1.0001f; // add a little epsilon
					collisions.push(coll);

					resolveCollisions = true;
				}
			}
		}

		for(auto& coll: collisions) {
			if(coll.boxB_static) {
				coll.pBodyA->box.min += coll.pushVec;
				coll.pBodyA->box.max += coll.pushVec;

				if(coll.pushVec.x != 0) {
					coll.pBodyA->x_locked = true;
					coll.pBodyA->vel.x = 0;
				}
				if(coll.pushVec.y != 0) {
					coll.pBodyA->y_locked = true;
					coll.pBodyA->vel.y = 0;
				}
			}
			else {
				if(!coll.pBodyA->x_locked) {
					coll.pBodyA->box.min.x += coll.pushVec.x / 2.f;
					coll.pBodyA->box.max.x += coll.pushVec.x / 2.f;
				}
				else {
					coll.pBodyB->box.min.x += -coll.pushVec.x;
					coll.pBodyB->box.max.x += -coll.pushVec.x;
				}

				if(!coll.pBodyA->y_locked) {
					coll.pBodyA->box.min.y += coll.pushVec.y / 2.f;
					coll.pBodyA->box.max.y += coll.pushVec.y / 2.f;
				}
				else {
					coll.pBodyB->box.min.y += -coll.pushVec.y;
					coll.pBodyB->box.max.y += -coll.pushVec.y;
				}

				if(!coll.pBodyB->x_locked) {
					coll.pBodyB->box.min.x += -coll.pushVec.x / 2.f;
					coll.pBodyB->box.max.x +=- coll.pushVec.x / 2.f;
				}
				if(!coll.pBodyB->y_locked) {
					coll.pBodyB->box.min.y += -coll.pushVec.y / 2.f;
					coll.pBodyB->box.max.y += -coll.pushVec.y / 2.f;
				}

				if(coll.pushVec.x != 0) {
					coll.pBodyA->vel.x = 0;
					coll.pBodyB->vel.x = 0;
				}
				if(coll.pushVec.y != 0) {
					coll.pBodyA->vel.y = 0;
					coll.pBodyB->vel.y = 0;
				}
			}
		}

		lsk_printf("step=%d", step);
	}
}
