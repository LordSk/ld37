#include "lsk_file.h"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>

lsk_Block lsk_fileReadWhole(const char* path, i32* out_pFileSize, lsk_IAllocator* pAlloc)
{
	FILE* file = fopen(path, "rb");
	if(!file) {
		return NULL_BLOCK;
	}
	defer(fclose(file));

	u32 start = ftell(file);
	fseek(file, 0, SEEK_END);
	u32 len = ftell(file) - start;
	fseek(file, start, SEEK_SET);

	lsk_Block block = pAlloc->allocate(len + 1);
	char* pFileBuff = (char*)block.ptr;

	// read
	fread(pFileBuff, 1, len, file);
	pFileBuff[len] = 0;
	if(out_pFileSize) {
		*out_pFileSize = len;
	}
	return block;
}


bool lsk_fileReadWholeCopy(const char* path, char* out_pFileBuff, u32 buffSize, i32* out_pFileLength)
{
	// https://msdn.microsoft.com/en-us/library/40bbyw78.aspx
	int fileHandle = _open(path, O_RDONLY | O_BINARY);
	if(fileHandle == -1) {
		return false;
	}
	int bytesRead = _read(fileHandle, out_pFileBuff, buffSize);
	*out_pFileLength = bytesRead;
	_close(fileHandle);
	return true;
}

bool lsk_fileWriteBuffer(const char* path, const char* pBuffer, u32 buffSize)
{
	int fileHandle = _open(path, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IWRITE);
	if(fileHandle == -1) {
		return false;
	}
	int bytesWritten = _write(fileHandle, pBuffer, buffSize);
	_close(fileHandle);
	return bytesWritten != -1;
}
