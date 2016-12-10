#include "tiledmap.h"
#include "renderer.h"
#include <external/parson.h>

LayerTile::~LayerTile()
{
	if(dataBlock.ptr) {
		AllocDefault.deallocate(dataBlock);
	}
}

TiledMap::~TiledMap()
{
	if(tilesetMaterialMemBlock.ptr) {
		AllocDefault.deallocate(tilesetMaterialMemBlock);
	}
}

// TODO: check for json validity (will crash for now if invalid)
bool TiledMap::load(const char* buff, bool verbose)
{
	auto jsonMalloc_func = [](u64 size) -> void* {
		lsk_Block b = AllocDefault.allocate(size);
		assert(b.ptr);
		return b.ptr;
	};

	auto jsonFree_func = [](void* ptr) -> void {
		if(!ptr) return;
		lsk_Block b;
		b.ptr = ptr;
		b._notaligned = ptr;
		AllocDefault.deallocate(b);
	};

	json_set_allocation_functions(jsonMalloc_func, jsonFree_func);

	JSON_Value* rootValue = json_parse_string(buff);
	if(!rootValue) {
		lsk_errf("Error: could not parse tiledmap");
		return false;
	}
	defer(json_value_free(rootValue););

	JSON_Object* root = json_value_get_object(rootValue);

	width = json_object_get_number(root, "width");
	height = json_object_get_number(root, "height");
	tileWidth = json_object_get_number(root, "tilewidth");
	tileHeight = json_object_get_number(root, "tileheight");

	// LAYERS
	JSON_Array* layers = json_object_get_array(root, "layers");
	const i32 layerCount = json_array_get_count(layers);
	if(verbose) lsk_printf("layers.count=%d", layerCount);

	for(i32 i = 0; i < layerCount; ++i) {
		JSON_Object* layerJsonObj = json_array_get_object(layers, i);
		const char* name = json_object_get_string(layerJsonObj, "name");
		const char* type = json_object_get_string(layerJsonObj, "type");
		i32 width = (i32)json_object_get_number(layerJsonObj, "width");
		i32 height = (i32)json_object_get_number(layerJsonObj, "height");
		i32 visible = (i32)json_object_get_boolean(layerJsonObj, "visible");

		if(verbose) lsk_printf("layer %d { name=%s, type=%s}", i, name, type);

		// TILE LAYER
		if(lsk_strEq(type, "tilelayer")) {
			LayerTile& layerTile = tileLayers.push(LayerTile());
			layerTile.name.set(name);
			layerTile.width = width;
			layerTile.height = height;
			layerTile.visible = visible;
			layerTile.dataBlock = AllocDefault.allocate(layerTile.width * layerTile.height * sizeof(i32),
														alignof(i32));
			layerTile.data = (i32*)layerTile.dataBlock.ptr;

			JSON_Array* dataArray = json_object_get_array(layerJsonObj, "data");
			const i32 dataArrayCount = json_array_get_count(dataArray);
			for(i32 d = 0; d < dataArrayCount; ++d) {
				layerTile.data[d] = (i32)json_array_get_number(dataArray, d);
			}
		}

		// OBJECT LAYER
		else if(lsk_strEq(type, "objectgroup")) {
			LayerObject& layerObj = objectLayers.push(LayerObject());
			layerObj.name.set(name);

			JSON_Array* objArray = json_object_get_array(layerJsonObj, "objects");
			const i32 objArrayCount = json_array_get_count(objArray);
			if(verbose) lsk_printf("objects.count=%d", objArrayCount);
			for(i32 o = 0; o < objArrayCount; ++o) {
				JSON_Object* jsonObj = json_array_get_object(objArray, o);

				LayerObject::Object& obj = layerObj.objects.push(LayerObject::Object());
				obj.width = (i32)json_object_get_number(jsonObj, "width");
				obj.height = (i32)json_object_get_number(jsonObj, "height");
				obj.x = (i32)json_object_get_number(jsonObj, "x");
				obj.y = (i32)json_object_get_number(jsonObj, "y");
				obj.name.set(json_object_get_string(jsonObj, "name"));
				obj.type.set(json_object_get_string(jsonObj, "type"));
			}
		}
	}

	// TILESETS
	JSON_Array* tilesetArray = json_object_get_array(root, "tilesets");
	const i32 tilesetArrayCount = json_array_get_count(tilesetArray);
	if(verbose) lsk_printf("tilesets.count=%d", tilesetArrayCount);

	for(i32 i = 0; i < tilesetArrayCount; ++i) {
		JSON_Object* item = json_array_get_object(tilesetArray, i);
		Tileset& tileset = tilesets.push(Tileset());
		tileset.imageName.set(json_object_get_string(item, "image"));
		tileset.firstGid = (i32)json_object_get_number(item, "firstgid");
		tileset.width = (i32)json_object_get_number(item, "imagewidth");
		tileset.height = (i32)json_object_get_number(item, "imageheight");
		tileset.tileWidth = (i32)json_object_get_number(item, "tilewidth");
		tileset.tileHeight = (i32)json_object_get_number(item, "tileheight");
		if(verbose) lsk_printf("tileset %s { firstGid=%d, tileWidth=%d, tileHeight=%d}",
				   tileset.imageName.c_str(), tileset.firstGid,
				   tileset.tileWidth, tileset.tileHeight);
	}

	// sort tilesets by firstgid
	auto compareTilesets_func = [](const void* a, const void* b) {
		const Tileset* ta = (Tileset*)a;
		const Tileset* tb = (Tileset*)b;
		if(ta->firstGid < tb->firstGid) {
			return -1;
		}
		if(ta->firstGid > tb->firstGid) {
			return 1;
		}
		return 0;
	};

	qsort(tilesets.data(), tilesets.count(), sizeof(tilesets.data()[0]), compareTilesets_func);

	return true;
}

void TiledMap::initForDrawing()
{
	u64 tilesetMaterialDataSize = 0;

	for(const auto& ti: tilesets) {
		tilesetMaterialDataSize += (ti.width / ti.tileWidth) * (ti.height / ti.tileHeight) *
				sizeof(Shader_Textured::Material);
	}

	tilesetMaterialMemBlock = AllocDefault.allocate(tilesetMaterialDataSize,
													alignof(Shader_Textured::Material));
	assert_msg(tilesetMaterialMemBlock.ptr, "Out of memory");
	tilesetMaterialStack.init(tilesetMaterialMemBlock);

	for(const auto& ti: tilesets) {
		const i32 columns = ti.width / ti.tileWidth;
		const i32 rows = ti.height / ti.tileHeight;

		lsk_Block matDataBlock = tilesetMaterialStack.allocate(columns * rows *
															   sizeof(Shader_Textured::Material));
		assert(matDataBlock.ptr);
		Shader_Textured::Material* matData = (Shader_Textured::Material*)matDataBlock.ptr;

		for(i32 y = 0; y < rows; ++y) {
			for(i32 x = 0; x < columns; ++x) {
				Shader_Textured::Material mat;
				mat.setTexture(H(ti.imageName.c_str()));
				mat.uvParams.x = x;
				mat.uvParams.y = y;
				mat.uvParams.z = 1.f / columns;
				mat.uvParams.w = 1.f / rows;
				matData[y * columns + x] = mat;
			}
		}
	}
}

void TiledMap::draw()
{
	Shader_Textured::Material* matData = (Shader_Textured::Material*)tilesetMaterialMemBlock.ptr;

	i32 z = 0;
	for(const auto& layer: tileLayers) {
		if(!layer.visible) continue;

		for(i32 y = 0; y < layer.height; ++y) {
			for(i32 x = 0; x < layer.width; ++x) {
				i32 gid = layer.data[y * layer.width + x] - 1;
				if(gid < 0) continue;

				DrawCommand cmd;
				// FIXME: some tiles dont have the same dimensions
				cmd.modelMatrix = lsk_Mat4Translate({(f32)x * tileWidth, (f32)y * tileHeight, 0}) *
						lsk_Mat4Scale({(f32)tileWidth, (f32)tileHeight, 0});
				cmd.vao = Renderer._quadVao;
				cmd.z = z++;
				cmd._materialType = MaterialType::TEXTURED;
				cmd._pMaterialData = matData + gid;
				Renderer.queue(cmd);
			}
		}
	}
}
