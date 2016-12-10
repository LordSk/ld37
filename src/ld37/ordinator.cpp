#include "ordinator.h"
Ref<Transform> Ordinator::make_Transform()
{
	return _comp_Transform.push(Transform());
}

Ref<Sprite> Ordinator::make_Sprite()
{
	return _comp_Sprite.push(Sprite());
}

Ref<CHealth> Ordinator::make_CHealth()
{
	return _comp_CHealth.push(CHealth());
}

Ref<CTarget> Ordinator::make_CTarget()
{
	return _comp_CTarget.push(CTarget());
}

Ref<Actor> Ordinator::spawn_Actor()
{
	return _entity_Actor.push(Actor());
}

void Ordinator::destroy_Actor(Actor& ent)
{
	ent.transform->endPlay();
	_comp_Transform.remove(ent.transform);
	ent.body_debugSprite->endPlay();
	_comp_Sprite.remove(ent.body_debugSprite);
	ent.endPlay();
	_entity_Actor.remove(ent);
}

Ref<APlayer> Ordinator::spawn_APlayer()
{
	return _entity_APlayer.push(APlayer());
}

void Ordinator::destroy_APlayer(APlayer& ent)
{
	ent.transform->endPlay();
	_comp_Transform.remove(ent.transform);
	ent.body_debugSprite->endPlay();
	_comp_Sprite.remove(ent.body_debugSprite);
	ent.healthComp->endPlay();
	_comp_CHealth.remove(ent.healthComp);
	ent.endPlay();
	_entity_APlayer.remove(ent);
}

Ref<ASkeleton> Ordinator::spawn_ASkeleton()
{
	return _entity_ASkeleton.push(ASkeleton());
}

void Ordinator::destroy_ASkeleton(ASkeleton& ent)
{
	ent.transform->endPlay();
	_comp_Transform.remove(ent.transform);
	ent.body_debugSprite->endPlay();
	_comp_Sprite.remove(ent.body_debugSprite);
	ent.target->endPlay();
	_comp_CTarget.remove(ent.target);
	ent.healthComp->endPlay();
	_comp_CHealth.remove(ent.healthComp);
	ent.endPlay();
	_entity_ASkeleton.remove(ent);
}

Ref<ASkeletonBigShield> Ordinator::spawn_ASkeletonBigShield()
{
	return _entity_ASkeletonBigShield.push(ASkeletonBigShield());
}

void Ordinator::destroy_ASkeletonBigShield(ASkeletonBigShield& ent)
{
	ent.transform->endPlay();
	_comp_Transform.remove(ent.transform);
	ent.body_debugSprite->endPlay();
	_comp_Sprite.remove(ent.body_debugSprite);
	ent.target->endPlay();
	_comp_CTarget.remove(ent.target);
	ent.healthComp->endPlay();
	_comp_CHealth.remove(ent.healthComp);
	ent.endPlay();
	_entity_ASkeletonBigShield.remove(ent);
}

void Ordinator::init()
{
	// TODO: determine starting capacities depending on the game
	_comp_Transform.init(32);
	_comp_Sprite.init(32);
	_comp_CHealth.init(32);
	_comp_CTarget.init(32);
	_entity_Actor.init(32);
	_entity_APlayer.init(32);
	_entity_ASkeleton.init(32);
	_entity_ASkeletonBigShield.init(32);
}

void Ordinator::update(f64 delta)
{
	for(i32 i = 0; i < _entity_Actor.count(); ++i) {
		if(_entity_Actor.data(i)._markedAsDestroy) {
			destroy_Actor(_entity_Actor.data(i));
			--i;
		}
	}
	for(i32 i = 0; i < _entity_APlayer.count(); ++i) {
		if(_entity_APlayer.data(i)._markedAsDestroy) {
			destroy_APlayer(_entity_APlayer.data(i));
			--i;
		}
	}
	for(i32 i = 0; i < _entity_ASkeleton.count(); ++i) {
		if(_entity_ASkeleton.data(i)._markedAsDestroy) {
			destroy_ASkeleton(_entity_ASkeleton.data(i));
			--i;
		}
	}
	for(i32 i = 0; i < _entity_ASkeletonBigShield.count(); ++i) {
		if(_entity_ASkeletonBigShield.data(i)._markedAsDestroy) {
			destroy_ASkeletonBigShield(_entity_ASkeletonBigShield.data(i));
			--i;
		}
	}

	for(auto& comp : _comp_Transform) {
		comp.update(delta);
	}
	for(auto& comp : _comp_Sprite) {
		comp.update(delta);
	}
	for(auto& comp : _comp_CHealth) {
		comp.update(delta);
	}
	for(auto& comp : _comp_CTarget) {
		comp.update(delta);
	}

	for(auto& ent : _entity_Actor) {
		ent.update(delta);
	}
	for(auto& ent : _entity_APlayer) {
		ent.update(delta);
	}
	for(auto& ent : _entity_ASkeleton) {
		ent.update(delta);
	}
	for(auto& ent : _entity_ASkeletonBigShield) {
		ent.update(delta);
	}
}

void Ordinator::destroy()
{
	for(auto& comp : _comp_Transform) {
		comp.endPlay();
	}
	_comp_Transform.destroy();
	for(auto& comp : _comp_Sprite) {
		comp.endPlay();
	}
	_comp_Sprite.destroy();
	for(auto& comp : _comp_CHealth) {
		comp.endPlay();
	}
	_comp_CHealth.destroy();
	for(auto& comp : _comp_CTarget) {
		comp.endPlay();
	}
	_comp_CTarget.destroy();
	for(auto& ent : _entity_Actor) {
		ent.endPlay();
	}
	_entity_Actor.destroy();
	for(auto& ent : _entity_APlayer) {
		ent.endPlay();
	}
	_entity_APlayer.destroy();
	for(auto& ent : _entity_ASkeleton) {
		ent.endPlay();
	}
	_entity_ASkeleton.destroy();
	for(auto& ent : _entity_ASkeletonBigShield) {
		ent.endPlay();
	}
	_entity_ASkeletonBigShield.destroy();
}

