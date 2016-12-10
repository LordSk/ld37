#pragma once
#include <lsk/lsk_array.h>
#include <lsk/lsk_string.h>
#include "renderer.h"

enum class ArchiveFileType: i32 {
	INVALID = -1,
	TEXTURE,
	SOUND,
	MATERIAL,
	TILEDMAP,
	COUNT
};

struct ArchiveFile
{
	ArchiveFileType type = ArchiveFileType::INVALID;
	lsk_DStr256 name;
	lsk_Block buffer;
	i32 fileSize;

	~ArchiveFile() {
		if(buffer.ptr) {
			AllocDefault.deallocate(buffer);
		}
	}
};

struct ArchiveFile_MaterialHeader
{
	MaterialType type = MaterialType::INVALID;
};

struct ArchiveFile_MaterialColor: ArchiveFile_MaterialHeader
{
	f32 color[4] = {1, 0, 0, 1};
};

struct ArchiveFile_MaterialTextured: ArchiveFile_MaterialHeader
{
	f32 color[4] = {1, 0, 0, 1};
	f32 uvOffset[2];
	f32 uvScale[2];
	u32 textureNameHash = 0;
};

struct ArchiveFile_Texture
{
	i32 width, height, comp;
	u8 data[1];
};

struct Archive
{
	lsk_DSparseArray<ArchiveFile> fileList;
	lsk_DStrHashMap<Ref<ArchiveFile>> fileStrMap;

	void init();
	void deinit();

	bool open(const char* path);
	bool saveTo(const char* path);
	void loadData() const;
};
