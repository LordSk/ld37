#pragma once

/*
 * Memory allocators
 *
 * Design choices:
 * - Fast and somewhat user-friendly
 * - Zero'ed out memory returned
 * - As few calls to context-switch functions possible (malloc, free, etc...)
 * - Share the same interface for interpobality
 * - Allocation size must be > 0
 * - Dealloc doesn't zero memory
 *
 * Cascades:
 * - Allocate more allocators if needed
 * - Tries to never return NULL_BLOCK
 * - Caps at 32 allocators (should be more than enough)
*/

#include <stdlib.h>
#include "lsk_types.h"
#include "lsk_console.h"
#include "lsk_utils.h"

#define Kilobyte(v) (i64)(v)*(i64)1024
#define Megabyte(v) (i64)Kilobyte(v)*(i64)1024
#define Gigabyte(v) (i64)Megabyte(v)*(i64)1024

struct lsk_Block
{
	void* ptr = nullptr;
	void* _notaligned = nullptr;
	u64 size = 0;

	lsk_Block() = default;
	lsk_Block(void* ptr_, u64 size_) {
		ptr = ptr_;
		_notaligned = ptr_;
		size = size_;
	}
	lsk_Block(void* ptr_, void* unaligned_, u64 size_) {
		ptr = ptr_;
		_notaligned = unaligned_;
		size = size_;
	}

	inline u64 unalignedSize() const {
		return size + ((intptr_t)_notaligned - (intptr_t)ptr);
	}
};

#define NULL_BLOCK lsk_Block{nullptr,0}

// TODO: only use this in debug mode?
#define allocate(...) _alloc(__FILE__, __LINE__, ##__VA_ARGS__)
#define reallocate(...) _realloc(__FILE__, __LINE__, ##__VA_ARGS__)
#define deallocate(...) _dealloc(__FILE__, __LINE__, ##__VA_ARGS__)
#define POSARG const char* filename, i32 line

// Allocator interface
struct lsk_IAllocator
{
	virtual void release() {}
	virtual lsk_Block _alloc(POSARG, u64 size, u8 alignment = 0) = 0;
	virtual lsk_Block _realloc(POSARG, lsk_Block block, u64 size, u8 alignment = 0) = 0;
	virtual void _dealloc(POSARG, lsk_Block block) = 0;
	virtual bool owns(lsk_Block block) const = 0;
};

// Allocator using malloc
struct lsk_Mallocator: public lsk_IAllocator
{
	SINGLETON_IMP(lsk_Mallocator)

	i32 _allocCount = 0;
	~lsk_Mallocator() {
		if(_allocCount != 0) {
			lsk_errf("%d leaks", _allocCount);
		}
	}

	lsk_Block _alloc(POSARG, u64 size, u8 alignment = 0);

	lsk_Block _realloc(POSARG, lsk_Block block, u64 size, u8 alignment = 0);

	void _dealloc(POSARG, lsk_Block block);

	inline bool owns(lsk_Block block) const {
		return true;
	}
};

struct lsk_AllocatorStack: public lsk_IAllocator
{
	lsk_Block _block = NULL_BLOCK;
	u64 _topStackMarker = 0;

	/**
	 * @brief Allocate the global block of memory
	 * @param size
	 */
	void init(lsk_Block block);

	/**
	 * @brief Split into two separate allocators
	 * @param pOther
	 * @param size
	 */
	void split(lsk_AllocatorStack* pOther, u64 size);

	/**
	 * @brief Allocate
	 * @param size
	 * @return pointer to allocated block
	 */
	lsk_Block _alloc(POSARG, u64 size, u8 alignment = 0);

	/**
	 * @brief Realloc block. /!\ WILL deallocate to block start. /!\
	 * @param filename
	 * @param line
	 * @param block
	 * @param size
	 * @param alignment
	 * @return
	 */
	lsk_Block _realloc(POSARG, lsk_Block block, u64 size, u8 alignment = 0);

	/**
	 * @brief Dealloc TO block start
	 * @param filename
	 * @param line
	 * @param block
	 */
	void _dealloc(POSARG, lsk_Block block);

	/**
	 * @brief Returns top marker. Doubles as a "how much memory are we using"
	 * @return
	 */
	inline u64 getTopMarker() const {
		return _topStackMarker;
	}

	/**
	 * @brief Returns malloc'd size
	 * @return
	 */
	inline u64 size() const {
		return _block.size;
	}

	inline bool owns(lsk_Block block) const {
		return block.ptr >= _block.ptr &&
				(intptr_t)block.ptr < ((intptr_t)_block.ptr + (intptr_t)_block.size);
	}
};

struct lsk_AllocatorStackCascade: lsk_IAllocator
{
	lsk_IAllocator* _parent = nullptr;
	static constexpr i32 _MAX_ALLOCATORS = 32;
	lsk_AllocatorStack _allocators[_MAX_ALLOCATORS];
	i32 _allocatorCount = 0;
	f64 _growth = 2.0;
	u64 _capacity = 0;
	u8 _align = 0;

	void init(lsk_IAllocator* parent, u64 size, u8 alignment = 0);
	void release();

	lsk_Block _alloc(POSARG, u64 size, u8 alignment = 0);
	lsk_Block _realloc(POSARG, lsk_Block block, u64 size, u8 alignment = 0);
	void _dealloc(POSARG, lsk_Block block);
	bool owns(lsk_Block block) const;
};


// TODO: remove those functions
#define alloc_ELT(...) _allocElt(__FILE__, __LINE__, ##__VA_ARGS__)
#define realloc_ELT(...) _reallocElt(__FILE__, __LINE__, ##__VA_ARGS__)

struct lsk_AllocatorField: public lsk_IAllocator
{
	lsk_Block _block = NULL_BLOCK;
	char* _memBucketStart = nullptr;
	u32 _elementSize = 1;
	u32 _elementMaxCount = 0;
	u8* _fillBitField = nullptr;

	void init(lsk_Block block, u32 elementSize, u8 alignment = 0);

	lsk_Block _allocElt(POSARG, u32 count);
	inline lsk_Block _alloc(POSARG, u64 size, u8 alignment = 0) {
		lsk_warnf("%s:%d _alloc(%d) %d", filename, line, size, lsk_ceil(size / (f64)_elementSize));
		lsk_Block block = _allocElt(filename, line, lsk_ceil(size / (f64)_elementSize));
		assert(block.ptr);
		return block;
	}
	void _dealloc(POSARG, lsk_Block block);
	lsk_Block _shrink(lsk_Block block, u32 newCount);
	lsk_Block _expand(lsk_Block block, u32 newCount);
	lsk_Block _reallocElt(POSARG, lsk_Block block, u32 newCount);
	inline lsk_Block _realloc(POSARG, lsk_Block block, u64 size, u8 alignment = 0) {
		return _reallocElt(filename, line, block, lsk_ceil(size / (f64)_elementSize));
	}

	u8 _checkBit(u32 eltId);
	void _setBit(u32 eltId);
	void _unsetBit(u32 eltId);

	inline bool owns(lsk_Block block) const {
		return block.ptr > _block.ptr && block.ptr < ((char*)_block.ptr + _block.size);
	}

	static inline i32 calcBookKeepingSize(u32 count, u8 alignment = 0) {
		return count / 8 + (count & 7) + alignment;
	}
};

struct lsk_AllocatorFieldCascade: lsk_IAllocator
{
	lsk_IAllocator* _parent = nullptr;
	static constexpr i32 _MAX_allocATORS = 32;
	lsk_AllocatorField _allocators[_MAX_allocATORS];
	i32 _allocatorCount = 0;
	f64 _growth = 1.0; // TODO: fix growth to be a percentile (1.5 = 150% not 250%)
	i32 _totalSize = 0;
	u32 _eltSize = 0;
	i32 _align = -1;

	void init(lsk_IAllocator* parent, u64 size, u32 eltSize, u8 alignment = 0);
	void release();

	inline lsk_Block _alloc(POSARG, u64 size, u8 alginment = 0) {
		return _allocElt(filename, line, lsk_ceil(size / (f64)_eltSize));
	}
	inline lsk_Block _realloc(POSARG, lsk_Block block, u64 size, u8 alignment) {
		return _reallocElt(filename, line, block, lsk_ceil(size / (f64)_eltSize));
	}

	lsk_Block _allocElt(POSARG, u32 reqCount);
	lsk_Block _reallocElt(POSARG, lsk_Block block, u32 newCount);
	void _dealloc(POSARG, lsk_Block buff);
	bool owns(lsk_Block block) const;
};

/**
 * @brief Pool Allocator
 *	- very fast
 *	- limited to elementSize alllocations
 */
struct lsk_AllocatorPool: lsk_IAllocator
{
	lsk_Block _block = NULL_BLOCK;
	intptr_t _start = 0;
	u32 _elementSize = 0;
	u32 _elementCapacity = 0;

	struct FreeList {
		FreeList* next;
	};

	FreeList* _freeListTop = nullptr;

	void init(lsk_Block block, u32 elementSize, u8 alignment = 0);

	lsk_Block _alloc(POSARG, u64 size, u8 alignment = 0);
	lsk_Block _realloc(POSARG, lsk_Block block, u64 size, u8 alignment = 0);
	void _dealloc(POSARG, lsk_Block block);

	inline bool owns(lsk_Block block) const {
		return ((intptr_t)block._notaligned >= (intptr_t)_block.ptr &&
				(intptr_t)block._notaligned < (intptr_t)_block.ptr + (intptr_t)_block.size);
	}

	inline bool isFull() const {
		return _freeListTop == nullptr;
	}
};

struct lsk_AllocatorPoolCascade: lsk_IAllocator
{
	lsk_IAllocator* _parent = nullptr;
	static constexpr i32 _MAX_allocATORS = 32;
	lsk_AllocatorPool _allocators[_MAX_allocATORS];
	i32 _allocatorCount = 0;
	f64 _growth = 2.0;
	u64 _capacity = 0;
	u32 _elementSize = 0;
	u8 _alignement = 0;

	void init(lsk_IAllocator* parent, u64 size, u32 elementSize, u8 alignment = 0);
	void release();

	lsk_Block _alloc(POSARG, u64 size, u8 alignment = 0);
	lsk_Block _realloc(POSARG, lsk_Block block, u64 size, u8 alignment = 0);
	void _dealloc(POSARG, lsk_Block block);

	bool owns(lsk_Block block) const;
};


/**
 * @brief Heap Allocator (first fit)
 * [(blockinfo)#####----(blockinfo)##---]
 */
struct lsk_AllocatorHeap: public lsk_IAllocator
{
	lsk_Block _block = NULL_BLOCK;
	static constexpr u64 _minAllocSize = 32;

	struct BlockInfo {
		u64 size = 0;
		BlockInfo* prev = nullptr;
		BlockInfo* next = nullptr;
	};

	BlockInfo* _first = nullptr;
	static constexpr u8 SIZE_BI = sizeof(BlockInfo);

	void init(lsk_Block block);

	lsk_Block _alloc(POSARG, u64 size, u8 alignment = 0);

	// TODO: optimize this
	inline lsk_Block _realloc(POSARG, lsk_Block block, u64 size, u8 alignment = 0) {
		assert(size > 0);
		lsk_Block newBlock = _alloc(filename, line, size, alignment);
		if(!newBlock.ptr) {
			return NULL_BLOCK;
		}
		memmove(newBlock.ptr, block.ptr, block.size);
		_dealloc(filename, line, block);
		return newBlock;
	}

	void _dealloc(POSARG, lsk_Block buff);

	inline bool owns(lsk_Block block) const {
		return block.ptr > _block.ptr && block.ptr < ((char*)_block.ptr + _block.size);
	}
};


struct lsk_AllocatorHeapCascade: public lsk_IAllocator
{
	lsk_IAllocator* _parent = nullptr;
	static constexpr i32 _MAX_allocATORS = 32;
	lsk_AllocatorHeap _allocators[_MAX_allocATORS];
	i32 _allocatorCount = 0;
	f64 _growth = 2.0;
	u64 _capacity = 0;

	void init(lsk_IAllocator* parent, u64 size);
	void release();

	lsk_Block _alloc(POSARG, u64 size, u8 alignment = 0);
	lsk_Block _realloc(POSARG, lsk_Block block, u64 size, u8 alignment = 0);
	void _dealloc(POSARG, lsk_Block block);

	inline bool owns(lsk_Block block) const {
		for(i32 i = 0; i < _allocatorCount; ++i) {
			if(_allocators[i].owns(block)) {
				return true;
			}
		}
		return false;
	}
};

/**
 * @brief Step allocator: use a different allocator based of allocation size
 */
struct lsk_AllocatorStep: lsk_IAllocator
{
	static constexpr i32 _MAX_allocATORS = 32;
	lsk_IAllocator* _pAllocs[_MAX_allocATORS];
	u64 _maxAllocSize[_MAX_allocATORS];
	u32 _allocatorCount = 0;

	/**
	 * @brief init
	 * @param pAllocators array of allocator pointers
	 * @param maxAllocSize array of maximum size to allocate for each allocator
	 * @param count allocator count
	 */
	void init(lsk_IAllocator** pAllocators, u64* maxAllocSize, u32 count);
	void release();

	lsk_Block _alloc(POSARG, u64 size, u8 alignment = 0);
	lsk_Block _realloc(POSARG, lsk_Block block, u64 size, u8 alignment = 0);
	void _dealloc(POSARG, lsk_Block block);
	bool owns(lsk_Block block) const;
};

// Easy access
extern lsk_IAllocator* _pDefaultAllocator;
extern lsk_IAllocator* _pOldDefaultAllocator;
#define AllocDefault (*_pDefaultAllocator)

inline void AllocDefault_push(lsk_IAllocator* pAlloc)
{
	_pOldDefaultAllocator = _pDefaultAllocator;
	_pDefaultAllocator = pAlloc;
}

inline void AllocDefault_pop()
{
	_pDefaultAllocator = _pOldDefaultAllocator;
}

inline void AllocDefault_set(lsk_IAllocator* pAlloc)
{
	_pDefaultAllocator = pAlloc;
	_pOldDefaultAllocator = pAlloc;
}

#define GMalloc (lsk_Mallocator::get())

lsk_AllocatorHeapCascade* GAllocHeap_init(u64 size);

lsk_AllocatorStep* GAllocStep_init(u32 ko1, u32 ko2, u32 ko4, u32 mo1, u32 mo2, u32 mo4,
									lsk_Block* out_mallocBlock);
