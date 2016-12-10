#include "texture.h"
#include <external/stb_image.h>
#include <external/gl3w.h>

void TextureManager::init()
{
	_diskStorage.init(32);
	_gpuStorage.init(32);

	for(i32 i = 0; i < TextureManager_TEXTURE_ARRAY_COUNT; ++i) {
		_textureArray[i].slot = _gpuNextActiveTextureSlot++;
		glActiveTexture(GL_TEXTURE0 + _textureArray[i].slot);
		glGenTextures(1, &_textureArray[i].id);

		glBindTexture(GL_TEXTURE_2D_ARRAY, _textureArray[i].id);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, _textureArraySize[i], _textureArraySize[i],
					   _textureArray[i].capacity);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	glActiveTexture(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1);
}

void TextureManager::destroy()
{
	_diskStorage.destroy();
	_gpuStorage.destroy();

	for(i32 i = 0; i < TextureManager_TEXTURE_ARRAY_COUNT; ++i) {
		glDeleteTextures(1, &_textureArray[i].id);
	}
}

void TextureManager::registerTexture(u32 textureNameHash, const TextureData& data)
{
	_diskStorage.seth(textureNameHash, data);
}

void TextureManager::loadToGpu(const u32* textureNameHashes, u32 count)
{
	// TODO: manage gpu memory, for now it loads everything in
	for(u32 i = 0; i < count; ++i) {
		const GpuStorage* st = _gpuStorage.geth(textureNameHashes[i]);
		if(!st || st->layerID < 0) {
			assert(_diskStorage.geth(textureNameHashes[i]));
			const TextureData& diskStorage = *_diskStorage.geth(textureNameHashes[i]);

			for(i32 a = 0; a < TextureManager_TEXTURE_ARRAY_COUNT; ++a) {
				if(lsk_max(diskStorage.width, diskStorage.height) <= _textureArraySize[a]) {
					GpuStorage gpuStorage;
					gpuStorage.nx = diskStorage.width / (f32)_textureArraySize[a];
					gpuStorage.ny = diskStorage.height / (f32)_textureArraySize[a];
					gpuStorage.texArrayID = a;
					gpuStorage.layerID = _textureArray[a].count++;
					glBindTexture(GL_TEXTURE_2D_ARRAY, _textureArray[a].id);

					glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
									0, 0, 0,
									gpuStorage.layerID,
									diskStorage.width, diskStorage.height,
									1,
									diskStorage.comp == 4 ? GL_RGBA : GL_RGB,
									GL_UNSIGNED_BYTE,
									diskStorage.data);
					glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

					_gpuStorage.seth(textureNameHashes[i], gpuStorage);
					break;
				}
			}
		}
	}
}
