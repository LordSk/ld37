#pragma once
#include <external/gl3w.h>
#include "lsk_types.h"
#include "lsk_allocator.h"

// EXT_texture_compression_s3tc
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3

// NVX_gpu_memory_info
#define GL_NV_MEMORY_DEDICATED 0x9047
#define GL_NV_MEMORY_MAX 0x9048
#define GL_NV_MEMORY_AVAILABLE 0x9049


enum DDSCompType: u32 {
	DDS_TYPE_INVALID=0,
	DDS_TYPE_DXT1,
	DDS_TYPE_DXT5,
};

struct lsk_ImageData
{
	const u8* pPixelBuff = nullptr;
	u32 pixelBuffSize = 0;
	i32 width = 0;
	i32 height = 0;
	DDSCompType compType = DDS_TYPE_INVALID; // compression type
};

/**
 * @brief Read pixel data from DDS file buffer
 * @param pRawBuff
 * @param buffSize
 * @param pImgData
 * @return success
 */
bool lsk_ddsRead(const char* pRawBuff, u32 buffSize, lsk_ImageData* pImgData);

GLuint lsk_glMakeTextureDDS(const void* pPixelData, i32 pixelDataSize, i32 width, i32 height,
							DDSCompType compType, bool repeat = false);
inline GLuint lsk_glMakeTextureDDS(const lsk_ImageData& imgData, bool repeat = false) {
	return lsk_glMakeTextureDDS(imgData.pPixelBuff, imgData.pixelBuffSize, imgData.width, imgData.height,
								imgData.compType, repeat);
}

GLuint lsk_glMakeShader(GLenum type, const char* pFileBuff, i32 fileSize,
						lsk_IAllocator* pAlloc = &AllocDefault);
GLuint lsk_glMakeProgram(const GLuint* shaders, u32 shaderCount,
						 lsk_IAllocator* pAlloc = &AllocDefault);
