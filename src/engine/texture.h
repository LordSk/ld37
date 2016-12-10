#pragma once
#include <lsk/lsk_array.h>

struct TextureData
{
	i32 width = 0;
	i32 height = 0;
	i32 comp = 4;
	u8* data = nullptr;
};

#define TextureManager_TEXTURE_ARRAY_COUNT 3

struct TextureManager
{
	SINGLETON_IMP(TextureManager)
	struct GpuStorage {
		u32 texArrayID = 0;
		i32 layerID = 0;
		f32 nx = 0; // width normalized on texture array width
		f32 ny = 0; // height normalized ---
	};

	lsk_DStrHashMap<TextureData> _diskStorage; // TODO: move to content manager?
	lsk_DStrHashMap<GpuStorage> _gpuStorage;
	u32 _gpuNextActiveTextureSlot = 1;

	struct TextureArray {
		u32 id = 0;
		i32 slot = -1;
		u32 capacity = 64;
		u32 count = 0;
	};

	// TODO: 256/512/1024 DYNAMICALLY sized arrays
	const i32 _textureArraySize[TextureManager_TEXTURE_ARRAY_COUNT] = {
		256, 512, 1024
	};

	TextureArray _textureArray[TextureManager_TEXTURE_ARRAY_COUNT];

	void init();
	void destroy();

	void registerTexture(u32 textureNameHash, const TextureData& data);
	void loadToGpu(const u32* textureNameHashes, u32 count);

	inline const GpuStorage& getGpuTex(u32 textureNameHash) {
		GpuStorage* gpuSt = _gpuStorage.geth(textureNameHash);
		assert(gpuSt);
		return *gpuSt;
	}
};


#define Textures TextureManager::get()
