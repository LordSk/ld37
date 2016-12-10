#pragma once
#include <lsk/lsk_array.h>
#include "C:\Prog\Projects\ld37\src\engine/base_entity.h"


struct Ordinator
{
	lsk_DSparseArray<Transform> _comp_Transform;
	Ref<Transform> make_Transform();
	lsk_DSparseArray<Sprite> _comp_Sprite;
	Ref<Sprite> make_Sprite();


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
