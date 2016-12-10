#pragma once
#include <lsk/lsk_array.h>
#include "C:\Prog\Projects\ld37\src\engine/base_entity.h"
#include "C:\Prog\Projects\ld37\src\ld37/ld37.h"


struct Ordinator
{
	lsk_DSparseArray<Transform> _comp_Transform;
	Ref<Transform> make_Transform();
	lsk_DSparseArray<Sprite> _comp_Sprite;
	Ref<Sprite> make_Sprite();

	lsk_DSparseArray<Actor> _entity_Actor;
	Ref<Actor> spawn_Actor();
	void destroy_Actor(Actor& ent);
	inline void destroy_Actor(Ref<Actor>& ref) {
		destroy_Actor(ref.get());
		ref.clear();
	}
	lsk_DSparseArray<APlayer> _entity_APlayer;
	Ref<APlayer> spawn_APlayer();
	void destroy_APlayer(APlayer& ent);
	inline void destroy_APlayer(Ref<APlayer>& ref) {
		destroy_APlayer(ref.get());
		ref.clear();
	}

	void init();
	void update(f64 delta);
	void destroy();
};

inline Ordinator& __getOrdinator()
{
	static Ordinator ord;
	return ord;
}

#define Ord __getOrdinator()
