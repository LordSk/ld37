#pragma once
#include <lsk/lsk_array.h>

struct LayerTile
{
	i32 visible = 1;
	i32 width = 0, height = 0;
	i32* data = nullptr;
	lsk_DStr64 name;
	lsk_Block dataBlock = NULL_BLOCK;

	~LayerTile();
};

struct LayerObject
{
	lsk_DStr64 name;

	struct Object {
		i32 width = 0, height = 0;
		i32 x = 0, y = 0;
		lsk_DStr64 name;
		lsk_DStr64 type;
	};

	lsk_DArray<Object> objects = lsk_DArray<Object>(1);
};

struct Tileset
{
	lsk_DStr64 imageName;
	i32 firstGid = 0;
	i32 width = 0, height = 0;
	i32 tileWidth = 0, tileHeight = 0;
};

struct TiledMap
{
	~TiledMap();

	i32 width = 0, height = 0;
	i32 tileWidth = 0, tileHeight = 0;

	lsk_DArray<LayerTile> tileLayers = lsk_DArray<LayerTile>(1);
	lsk_DArray<LayerObject> objectLayers = lsk_DArray<LayerObject>(1);
	lsk_DArray<Tileset> tilesets = lsk_DArray<Tileset>(1);

	lsk_Block tilesetMaterialMemBlock = NULL_BLOCK;
	lsk_AllocatorStack tilesetMaterialStack;

	bool load(const char* buff, bool verbose = false);

	void initForDrawing();
	void draw();
};
