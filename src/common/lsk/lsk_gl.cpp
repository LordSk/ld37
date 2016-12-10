#include "lsk_gl.h"
#include "lsk_console.h"
#include "lsk_allocator.h"

struct DDS_PIXELFORMAT
{
	u32 dwSize;
	u32 dwFlags;
	u32 dwFourCC;
	u32 dwRGBBitCount;
	u32 dwRBitMask;
	u32 dwGBitMask;
	u32 dwBBitMask;
	u32 dwABitMask;
};

enum: u32 {
	DDPF_ALPHAPIXELS = 0x1,
	DDPF_ALPHA = 0x2,
	DDPF_FOURCC = 0x4,
	DDPF_RGB = 0x4,
	DDPF_YUV = 0x200,
	DDPF_LUMINANCE = 0x20000
};

struct DDS_HEADER
{
	u32 dwSize;
	u32 dwFlags;
	u32 dwHeight;
	u32 dwWidth;
	u32 dwPitchOrLinearSize;
	u32 dwDepth;
	u32 dwMipMapCount;
	u32 dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	u32 dwCaps;
	u32 dwCaps2;
	u32 dwCaps3;
	u32 dwCaps4;
	u32 dwReserved2;
};

enum: u32 {
	FMT_DXT1 = 0x31545844,
	FMT_DXT2 = 0x32545844,
	FMT_DXT3 = 0x33545844,
	FMT_DXT4 = 0x34545844,
	FMT_DXT5 = 0x35545844
};

bool lsk_ddsRead(const char* pRawBuff, u32 buffSize, lsk_ImageData* pImgData)
{
	const u32* u32Magic = (const u32*)pRawBuff;
	if(*u32Magic != 0x20534444) { // "DDS"
		lsk_errf("lsk_ddsRead() Not a DDS file");
		return false;
	}

	pRawBuff += sizeof(u32);

	const DDS_HEADER* pDdsHeader = (const DDS_HEADER*)pRawBuff;

	if(pDdsHeader->dwSize != 124) { // "DDS"
		lsk_errf("lsk_ddsRead() DDS_HEADER size != 124");
		return false;
	}

	i32 width = (i32)pDdsHeader->dwWidth;
	i32 height = (i32)pDdsHeader->dwHeight;
	/*Dcout() << "width: "<< width << " height: " << height << endl;
	Dcout() << "MimapCount:" << pDdsHeader->dwMipMapCount << endl;
	Dcout() << "PixelFormat DDPF_ALPHAPIXELS:" << bool(pDdsHeader->ddspf.dwFlags&DDPF_ALPHAPIXELS) << endl;
	Dcout() << "PixelFormat DDPF_ALPHA:" << bool(pDdsHeader->ddspf.dwFlags&DDPF_ALPHA) << endl;
	Dcout() << "PixelFormat DDPF_FOURCC:" << bool(pDdsHeader->ddspf.dwFlags&DDPF_FOURCC) << endl;
	Dcout() << "PixelFormat DDPF_RGB:" << bool(pDdsHeader->ddspf.dwFlags&DDPF_RGB) << endl;
	Dcout() << "PixelFormat DDPF_YUV:" << bool(pDdsHeader->ddspf.dwFlags&DDPF_YUV) << endl;
	Dcout() << "PixelFormat DDPF_LUMINANCE:" << bool(pDdsHeader->ddspf.dwFlags&DDPF_LUMINANCE) << endlf();
	Dcout() << "Pitch?:" << pDdsHeader->dwPitchOrLinearSize << endlf();*/

	// compression type
	if(pDdsHeader->ddspf.dwFourCC != FMT_DXT5 && pDdsHeader->ddspf.dwFourCC != FMT_DXT1) {
		lsk_errf("lsk_ddsRead() only DXT1 and DXT5 supported");
		return false;
	}

	pRawBuff += pDdsHeader->dwSize;

	DDSCompType ddsType = DDS_TYPE_DXT5;

	// pitch (data size)
	u8 blockSize = 16;
	if(pDdsHeader->ddspf.dwFourCC == FMT_DXT1) {
		blockSize = 8;
		ddsType = DDS_TYPE_DXT1;
	}

	u32 dataSize = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;

	if(dataSize + 128 > buffSize) {
		lsk_errf("lsk_ddsRead() buffer too small to hold pixel data");
		return false;
	}

	lsk_ImageData imgData;
	imgData.pPixelBuff = (const u8*)pRawBuff;
	imgData.pixelBuffSize = dataSize;
	imgData.width = width;
	imgData.height = height;
	imgData.compType = ddsType;
	*pImgData = imgData;

	/**pImgData = lsk_ImageData{
		(const u8*)pRawBuff,
		dataSize,
		width,
		height,
		ddsType
	};*/

	return true;
}

GLuint lsk_glMakeTextureDDS(const void* pPixelData, i32 pixelDataSize, i32 width, i32 height,
							DDSCompType compType, bool repeat)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if(!repeat) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	u32 glCompType = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	if(compType == DDS_TYPE_DXT1) {
		glCompType = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
	}

	/*
	void glCompressedTexImage2D(
		GLenum target,
		GLint level,
		GLenum internalformat,
		GLsizei width,
		GLsizei height,
		GLint border,
		GLsizei imageSize,
		const GLvoid * data)
	*/

	glCompressedTexImage2D(
		GL_TEXTURE_2D,
		0,
		glCompType,
		width,
		height,
		0,
		pixelDataSize,
		pPixelData
	);

	/*
	glGenerateMipmap(GL_TEXTURE_2D);
	*/

	return texture;
}

GLuint lsk_glMakeShader(GLenum type, const char* pFileBuff, i32 fileSize,
						lsk_IAllocator* pAlloc)
{
	GLuint shader = glCreateShader(type);

	// compile
	glShaderSource(shader, 1, &pFileBuff, &fileSize);
	glCompileShader(shader);

	// check result
	GLint compileResult = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);

	if(!compileResult) {
		int logLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		lsk_Block block = pAlloc->allocate(logLength);
		char* logBuff = (char*)block.ptr;
		glGetShaderInfoLog(shader, logLength, NULL, logBuff);
		lsk_errf("Error [shader compilation]: %s", logBuff);
		pAlloc->deallocate(block);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint lsk_glMakeProgram(const GLuint* shaders, u32 shaderCount,
						 lsk_IAllocator* pAlloc)
{
	// link
	GLuint program = glCreateProgram();

	for(u32 i = 0; i < shaderCount; ++i) {
		glAttachShader(program, shaders[i]);
	}

	glLinkProgram(program);

	// check
	GLint linkResult = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linkResult);

	if(!linkResult) {
		int logLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		lsk_Block block = pAlloc->allocate(logLength);
		char* logBuff =  (char*)block.ptr;
		glGetProgramInfoLog(program, logLength, NULL, logBuff);
		lsk_errf("Error [program link]: %s", logBuff);
		pAlloc->deallocate(block);
		glDeleteProgram(program);
		return 0;
	}

	return program;
}
