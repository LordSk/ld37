#include "ordinator.h"
Ref<Transform> Ordinator::make_Transform()
{
	return _comp_Transform.push(Transform());
}

Ref<Sprite> Ordinator::make_Sprite()
{
	return _comp_Sprite.push(Sprite());
}

void Ordinator::init()
{
	// TODO: determine starting capacities depending on the game
	_comp_Transform.init(32);
	_comp_Sprite.init(32);
}

void Ordinator::update(f64 delta)
{

	for(auto& comp : _comp_Transform) {
		comp.update(delta);
	}
	for(auto& comp : _comp_Sprite) {
		comp.update(delta);
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
}

