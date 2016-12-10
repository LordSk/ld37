#include "base_entity.h"
#include <lsk/lsk_console.h>
#include "renderer.h"

// TODO: make this work again (make a base ordinator and a derived one?)
/*
Actor::Actor()
{
	transform = Ord.make_Transform();
}
*/

void Sprite::update(f64 delta)
{
	assert(transform.valid() && materialName > 0);

	lsk_Mat4 modelMatrix = lsk_Mat4Translate(lsk_Vec3{transform->position.x, transform->position.y, 0}
											 + lsk_Vec3{localPos, 0});
	if(!lsk_QuatIsNull(transform->rotation)) {
		modelMatrix = modelMatrix * lsk_QuatMatrix(transform->rotation);
	}
	if(origin.x != 0 || origin.y != 0) {
		modelMatrix = modelMatrix * lsk_Mat4Translate(lsk_Vec3{-origin, 0});
	}
	modelMatrix = modelMatrix * lsk_Mat4Scale(transform->scale * lsk_Vec3{size, 1});

	DrawCommand cmd;
	cmd.vao = Renderer._quadVao;
	cmd.modelMatrix = modelMatrix;
	cmd.z = transform->position.z;
	cmd.setMaterial(materialName);
	Renderer.queue(cmd);
}
