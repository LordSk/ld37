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
	lsk_DSparseArray<CBodyComponent> _comp_CBodyComponent;
	Ref<CBodyComponent> make_CBodyComponent();
	lsk_DSparseArray<CHealth> _comp_CHealth;
	Ref<CHealth> make_CHealth();
	lsk_DSparseArray<CTarget> _comp_CTarget;
	Ref<CTarget> make_CTarget();

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
	lsk_DSparseArray<ASkeleton> _entity_ASkeleton;
	Ref<ASkeleton> spawn_ASkeleton();
	void destroy_ASkeleton(ASkeleton& ent);
	inline void destroy_ASkeleton(Ref<ASkeleton>& ref) {
		destroy_ASkeleton(ref.get());
		ref.clear();
	}
	lsk_DSparseArray<ASkeletonBigShield> _entity_ASkeletonBigShield;
	Ref<ASkeletonBigShield> spawn_ASkeletonBigShield();
	void destroy_ASkeletonBigShield(ASkeletonBigShield& ent);
	inline void destroy_ASkeletonBigShield(Ref<ASkeletonBigShield>& ref) {
		destroy_ASkeletonBigShield(ref.get());
		ref.clear();
	}
	lsk_DSparseArray<ADragon> _entity_ADragon;
	Ref<ADragon> spawn_ADragon();
	void destroy_ADragon(ADragon& ent);
	inline void destroy_ADragon(Ref<ADragon>& ref) {
		destroy_ADragon(ref.get());
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
