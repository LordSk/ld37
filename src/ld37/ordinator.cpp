#include "ordinator.h"
Ref<Transform> Ordinator::make_Transform()
{
	return _comp_Transform.push(Transform());
}

Ref<Sprite> Ordinator::make_Sprite()
{
	return _comp_Sprite.push(Sprite());
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

void Ordinator::init()
{
	// TODO: determine starting capacities depending on the game
	_comp_Transform.init(32);
	_comp_Sprite.init(32);
	_entity_Actor.init(32);
}

void Ordinator::update(f64 delta)
{
	for(i32 i = 0; i < _entity_Actor.count(); ++i) {
		if(_entity_Actor.data(i)._markedAsDestroy) {
			destroy_Actor(_entity_Actor.data(i));
			--i;
		}
	}

	for(auto& comp : _comp_Transform) {
		comp.update(delta);
	}
	for(auto& comp : _comp_Sprite) {
		comp.update(delta);
	}

	for(auto& ent : _entity_Actor) {
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
	for(auto& ent : _entity_Actor) {
		ent.endPlay();
	}
	_entity_Actor.destroy();
}

