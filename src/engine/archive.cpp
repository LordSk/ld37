#include "archive.h"
#include <lz4.h>
#include <lsk/lsk_file.h>
#include <engine/texture.h>
#include <engine/renderer.h>
#include <engine/audio.h>

#define ARCHIVE_HEADER "LSK_ARCH"
#define ARCHIVE_HEADER_LEN lsk_const_strLen(ARCHIVE_HEADER)

void Archive::init()
{
	fileList.init(256);
}

void Archive::deinit()
{
	fileList.destroy();
	fileStrMap.destroy();
}

inline void pack_u16(u8** buffer, u16 data)
{
	memmove(*buffer, &data, sizeof(u16));
	*buffer += sizeof(u16);
}

inline void pack_u32(u8** buffer, u32 data)
{
	memmove(*buffer, &data, sizeof(u32));
	*buffer += sizeof(u32);
}

inline void pack_data(u8** buffer, const void* data, u64 size)
{
	memmove(*buffer, data, size);
	*buffer += size;
}

inline void unpack_u16(u8** buffer, u16* data)
{
	memmove(data, *buffer, sizeof(u16));
	*buffer += sizeof(u16);
}

inline void unpack_u32(u8** buffer, u32* data)
{
	memmove(data, *buffer, sizeof(u32));
	*buffer += sizeof(u32);
}

inline void unpack_data(u8** buffer, void* data, u64 size)
{
	memmove(data, *buffer, size);
	*buffer += size;
}

bool Archive::open(const char* path)
{
	lsk_printf("Opening archive %s...", path);

	lsk_Block fileBuffer = lsk_fileReadWhole(path);
	if(!fileBuffer.ptr) {
		return false;
	}
	defer(AllocDefault.deallocate(fileBuffer););

	u8* cursor = (u8*)fileBuffer.ptr;

	char headerStr[ARCHIVE_HEADER_LEN];
	unpack_data(&cursor, headerStr, ARCHIVE_HEADER_LEN);

	if(lsk_strCmp(headerStr, ARCHIVE_HEADER, ARCHIVE_HEADER_LEN) != -1) {
		return false;
	}

	u16 fileCount;
	unpack_u16(&cursor, &fileCount);

	fileStrMap.destroy();
	fileStrMap.init(fileCount);
	fileList.clear();
	fileList.reserve(fileCount);

	for(u16 i = 0; i < fileCount; ++i) {
		auto fileRef = fileList.push(ArchiveFile());
		ArchiveFile& file = fileRef.get();

		u16 fileNameLen;
		unpack_u16(&cursor, &fileNameLen);

		u16 fileType;
		unpack_u16(&cursor, &fileType);
		file.type = (ArchiveFileType)fileType;

		u32 comprFileSize;
		unpack_u32(&cursor, &comprFileSize);
		u32 decomprFileSize;
		unpack_u32(&cursor, &decomprFileSize);

		u32 fileOffset;
		unpack_u32(&cursor, &fileOffset);

		file.name.set((char*)cursor, fileNameLen);
		fileStrMap.set(file.name.c_str(), fileRef);
		cursor += fileNameLen;

		lsk_Block buff = AllocDefault.allocate(decomprFileSize, 4);
		assert_msg(buff.ptr, "Out of memory");

		if(comprFileSize == decomprFileSize) {
			memmove(buff.ptr, (void*)((intptr_t)fileBuffer.ptr + fileOffset), comprFileSize);
		}
		else {
			LZ4_decompress_safe((const char*)((intptr_t)fileBuffer.ptr + fileOffset), (char*)buff.ptr,
								comprFileSize, decomprFileSize);
		}

		file.buffer = buff;
		file.fileSize = decomprFileSize;
	}

	return true;
}

bool Archive::saveTo(const char* path)
{
	// TODO: validate files!
	lsk_printf("Saving archive %s...", path);

	u64 archiveFileSize = 8; // header
	archiveFileSize += sizeof(u16); // file count

	u64 maxFileDataSize = 0;
	for(const auto& file: fileList) {
		archiveFileSize += sizeof(u16); // filename string size
		archiveFileSize += sizeof(u16); // file type
		archiveFileSize += sizeof(u32); // compressed file size
		archiveFileSize += sizeof(u32); // decompressed file size
		archiveFileSize += sizeof(u32); // file offset (where to find data from 0x0)
		archiveFileSize += file.name.len(); // TODO: make a path from directory hierarchy

		maxFileDataSize += file.buffer.size;
	}

	u64 headerSize = archiveFileSize;

	lsk_DArray<i32> compressedFileSize(fileList.count());
	lsk_Block totalComprFileBlock = AllocDefault.allocate(maxFileDataSize);
	assert_msg(totalComprFileBlock.ptr, "Out of memory");
	defer(AllocDefault.deallocate(totalComprFileBlock));

	u64 fileOffset = 0;
	for(const auto& file: fileList) {
		i32 compressedSize = LZ4_compress_default((const char*)file.buffer.ptr,
										  ((char*)totalComprFileBlock.ptr) + fileOffset,
										  file.fileSize, file.fileSize);
		if(compressedSize == 0) {
			memmove(((char*)totalComprFileBlock.ptr) + fileOffset, file.buffer.ptr, file.fileSize);
			compressedSize = file.fileSize;
		}

		compressedFileSize.push(compressedSize);

		lsk_printf("ratio %.2f%% (%s)",
				   (f64)compressedSize / file.fileSize * 100,
				   file.name.c_str());
		fileOffset += compressedSize;
	}
	u64 compressedTotalFileDataSize = fileOffset;

	archiveFileSize += compressedTotalFileDataSize;

	lsk_Block archBlock = AllocDefault.allocate(archiveFileSize);
	assert_msg(archBlock.ptr, "Out of memory");
	defer(AllocDefault.deallocate(archBlock));

	u8* cursor = (u8*)archBlock.ptr;
	pack_data(&cursor, ARCHIVE_HEADER, ARCHIVE_HEADER_LEN);

	pack_u16(&cursor, (u16)fileList.count());

	i32 i = 0;
	fileOffset = 0;
	for(const auto& file: fileList) {
		pack_u16(&cursor, (u16)file.name.len());
		pack_u16(&cursor, (u16)file.type);
		pack_u32(&cursor, (u32)compressedFileSize[i]);
		pack_u32(&cursor, (u32)file.fileSize);
		pack_u32(&cursor, (u32)(headerSize + fileOffset));
		pack_data(&cursor, file.name.c_str(), file.name.len());
		fileOffset += compressedFileSize[i];
		++i;
	}

	pack_data(&cursor, totalComprFileBlock.ptr, compressedTotalFileDataSize);

	if(lsk_fileWriteBuffer(path, (const char*)archBlock.ptr, archBlock.size)) {
		lsk_succf("Archive %s (%llu) successfully written", path, archBlock.size);
		return true;
	}

	lsk_errf("Error: could not write archive %s (%llu)", path, archBlock.size);
	return false;
}

void Archive::loadData() const
{
	for(const auto& file: fileList) {
		//lsk_printf("- %s : %d", file.name.c_str(), file.type);

		switch(file.type) {
			case ArchiveFileType::TEXTURE: {
				ArchiveFile_Texture* pTex = (ArchiveFile_Texture*)file.buffer.ptr;
				TextureData texData;
				texData.comp = pTex->comp;
				texData.width = pTex->width;
				texData.height = pTex->height;
				texData.data = pTex->data;
				Textures.registerTexture(H(file.name.c_str()), texData);
			} break;

			case ArchiveFileType::MATERIAL: {
				ArchiveFile_MaterialHeader* pHeader = (ArchiveFile_MaterialHeader*)file.buffer.ptr;
				switch(pHeader->type) {
					case MaterialType::COLOR: {
						ArchiveFile_MaterialColor& matColor =
								*(ArchiveFile_MaterialColor*)file.buffer.ptr;

						Shader_Color::Material rmat;
						memmove(rmat.color.data, matColor.color, sizeof(f32) * 4);
						Renderer.materials.set(MaterialType::COLOR, H(file.name.c_str()), rmat);
					} break;

					case MaterialType::TEXTURED: {
						ArchiveFile_MaterialTextured& matTextured =
								*(ArchiveFile_MaterialTextured*)file.buffer.ptr;

						Shader_Textured::Material rmat;
						memmove(rmat.color.data, matTextured.color, sizeof(f32) * 4);
						rmat.uvParams = {
							matTextured.uvOffset[0],
							matTextured.uvOffset[1],
							matTextured.uvScale[0],
							matTextured.uvScale[1],
						};
						rmat.setTexture(matTextured.textureNameHash);

						Renderer.materials.set(MaterialType::TEXTURED, H(file.name.c_str()), rmat);
					} break;
				}
			} break;

			case ArchiveFileType::SOUND: {
				AudioGet.loadFromMem((u8*)file.buffer.ptr, file.fileSize, H(file.name.c_str()));
			}
		}
	}
}
