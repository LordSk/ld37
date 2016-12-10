#include <lsk/lsk_allocator.h>
#define GL3W_IMPLEMENTATION
#include "gl3w.h"

inline void* my_malloc(u64 size) {
	return AllocDefault.allocate(size).ptr;
}

inline void* my_realloc(void* ptr, u64 oldSize, u64 newSize) {
	lsk_Block block{ptr, ptr, oldSize};
	return AllocDefault.reallocate(block, newSize).ptr;
}

inline void my_free(void* ptr) {
	lsk_Block block{ptr, ptr, 0};
	AllocDefault.deallocate(block);
}

#define STBI_MALLOC(size) my_malloc(size)
#define STBI_REALLOC_SIZED(ptr, oldSize, newSize) my_realloc(ptr, oldSize, newSize)
#define STBI_FREE(ptr) my_free(ptr)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
