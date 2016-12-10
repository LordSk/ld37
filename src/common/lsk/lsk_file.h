#pragma once
#include "lsk_types.h"
#include "lsk_allocator.h"

/**
 * @brief Allocate a buffer containing the contents of the file
 * @param path File path
 * @param pLength File buffer length
 * @return Allocated buffer (0 if error)
 */
lsk_Block lsk_fileReadWhole(const char* path, i32* out_pFileSize = nullptr,
					   lsk_IAllocator* pAlloc = &AllocDefault);

/**
 * @brief Copy content of the file to pFileBuff
 * @param path
 * @param pFileBuff
 * @param buffSize
 * @param pFileLength
 * @return success
 */
bool lsk_fileReadWholeCopy(const char* path, char* out_pFileBuff, u32 buffSize, i32* out_pFileLength);

/**
 * @brief Write pBuffer to file
 * @param path
 * @param pBuffer
 * @param buffSize
 * @return success
 */
bool lsk_fileWriteBuffer(const char* path, const char* pBuffer, u32 buffSize);
