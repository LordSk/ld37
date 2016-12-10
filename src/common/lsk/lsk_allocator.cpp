#include "lsk_allocator.h"
#include <memory.h>
#include <string.h>

inline i32 alignAdjust(intptr_t addr, u8 alignment)
{
	if(alignment == 0) return 0;
	assert(alignment >= 1 && alignment <= 128);
	assert((alignment & (alignment - 1)) == 0); // power of 2

	intptr_t misalignment = addr % alignment;

	// realign
	if(misalignment > 0) {
		return alignment - misalignment;
	}
	return 0;
}

lsk_Block lsk_Mallocator::_alloc(const char* filename, i32 line, u64 size, u8 alignment)
{
	//lsk_printf("%s:%d allocate(%d)", filename, line, size);
	assert(size > 0);
	size += alignment;

	void* ptr = malloc(size);
	assert(ptr);
	memset(ptr, 0, size);

	++_allocCount;

	i32 adjust = alignAdjust((intptr_t)ptr, alignment);
	return lsk_Block{(void*)((intptr_t)ptr + adjust), ptr, size - adjust};
}

lsk_Block lsk_Mallocator::_realloc(const char* filename, i32 line, lsk_Block block, u64 size, u8 alignment)
{
	//lsk_printf("%s:%d reallocate(%#x, %d, %d)", filename, line, block.ptr, block.size, size);
	assert(size > 0);
	size += alignment;

	void* ptr = realloc(block._notaligned, size);
	assert(ptr);
	if(size > block.size) {
		memset((char*)ptr + block.size, 0, size - block.size);
	}

	i32 adjust = alignAdjust((intptr_t)ptr, alignment);
	return lsk_Block{(void*)((intptr_t)ptr + adjust), ptr, size - adjust};
}

void lsk_Mallocator::_dealloc(const char* filename, i32 line, lsk_Block block)
{
	if(!block.ptr || !block._notaligned) return;
	//lsk_printf("%s:%d deallocate(%#x, %d)", filename, line, block.ptr, block.size);
	free(block._notaligned);
	--_allocCount;
}

void lsk_AllocatorStack::init(lsk_Block block)
{
	assert(block.ptr);
	_block = block;
	_topStackMarker = 0;
}

void lsk_AllocatorStack::split(lsk_AllocatorStack* pOther, u64 size)
{
	assert(_topStackMarker + size <= _block.size);

	// right
	pOther->_block.ptr = (char*)_block.ptr + (_block.size - size);
	pOther->_block.size = size;
	pOther->_topStackMarker = 0;

	// left
	_block.size -= size;
}

lsk_Block lsk_AllocatorStack::_alloc(POSARG, u64 size, u8 alignment)
{
	assert(size > 0);

	i32 adjust = alignAdjust((intptr_t)_block.ptr + _topStackMarker, alignment);
	size += adjust;

	if(_topStackMarker + size > _block.size) {
		return NULL_BLOCK;
	}

	void* addr = (void*)((intptr_t)_block.ptr + _topStackMarker); // start of the block
	_topStackMarker += size; // end of the block
	memset(addr, 0, size);

	return lsk_Block{(void*)((intptr_t)addr + adjust), addr, size - adjust};
}

lsk_Block lsk_AllocatorStack::_realloc(POSARG, lsk_Block block, u64 size, u8 alignment)
{
	assert(size > 0);
	_dealloc(filename, line, block);

	i32 adjust = alignAdjust((intptr_t)_block.ptr + _topStackMarker, alignment);
	size += adjust;

	if(_topStackMarker + size > _block.size) {
		return NULL_BLOCK;
	}

	void* addr = (void*)((intptr_t)_block.ptr + _topStackMarker); // start of the block
	_topStackMarker += size; // end of the block

	u64 ulsize = block.unalignedSize();
	if(size > ulsize) {
		memset((void*)((intptr_t)addr + ulsize), 0, size - ulsize);
	}

	return lsk_Block{(void*)((intptr_t)addr + adjust), addr, size - adjust};
}

void lsk_AllocatorStack::_dealloc(POSARG, lsk_Block block)
{
	if(!block.ptr || !block._notaligned) return;
	u64 marker = (intptr_t)block._notaligned - (intptr_t)_block.ptr;
	assert(marker < _block.size);
	_topStackMarker = marker;
}


void lsk_AllocatorStackCascade::init(lsk_IAllocator* parent, u64 size, u8 alignment)
{
	assert(parent && size > 0);
	_parent = parent;
	_allocatorCount = 1;
	lsk_Block block = _parent->allocate(size, alignment);
	assert_msg(block.ptr, "Out of memory");
	_allocators[0].init(block);
	_capacity = size;
	_align = alignment;
}

void lsk_AllocatorStackCascade::release()
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		_parent->deallocate(_allocators[i]._block);
	}
	_allocatorCount = 0;
	_capacity = 0;
}

lsk_Block lsk_AllocatorStackCascade::_alloc(POSARG, u64 size, u8 alignment)
{
	assert(_capacity > 0 && _allocatorCount > 0);
	i32 lastID = _allocatorCount - 1;
	lsk_Block block = _allocators[lastID].allocate(size, alignment);
	if(block.ptr) {
		return block;
	}

	assert(_allocatorCount < _MAX_ALLOCATORS);
	u64 newCapacity = _capacity * _growth;
	u64 newAllocatorSize = newCapacity - _capacity;
	if(newAllocatorSize < size + alignment) {
		newAllocatorSize = size + alignment;
	}
	i32 newID = _allocatorCount++;
	lsk_Block parentBlock = _parent->allocate(newAllocatorSize, _align);
	assert_msg(parentBlock.ptr, "Out of memory");
	_allocators[newID].init(parentBlock);
	_capacity += newAllocatorSize;

	return _allocators[newID].allocate(size, alignment);
}

lsk_Block lsk_AllocatorStackCascade::_realloc(POSARG, lsk_Block block, u64 size, u8 alignment)
{
	i32 lastID = _allocatorCount - 1;
	assert(_allocators[lastID].owns(block));

	lsk_Block reBlock = _allocators[lastID].reallocate(block, size, alignment);
	if(reBlock.ptr) {
		return reBlock;
	}

	reBlock = allocate(size, alignment);
	memmove(reBlock.ptr, block.ptr, block.size); // works because StackAllocator doesn't erase freed memory
	return reBlock;
}

void lsk_AllocatorStackCascade::_dealloc(POSARG, lsk_Block block)
{
	i32 allocID = -1;
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_allocators[i].owns(block)) {
			allocID = i;
			break;
		}
	}

	if(allocID == -1) {
		return;
	}

	_allocators[allocID].deallocate(block);
	for(i32 i = allocID + 1; i < _allocatorCount; ++i) {
		_parent->deallocate(_allocators[i]._block);
		_capacity -= _allocators[i]._block.size;
	}
	_allocatorCount = allocID + 1;
}

bool lsk_AllocatorStackCascade::owns(lsk_Block block) const
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_allocators[i].owns(block)) {
			return true;
		}
	}
	return false;
}


//
// Field Allocator
//
void lsk_AllocatorField::init(lsk_Block block, u32 elementSize, u8 alignment)
{
	assert(block.ptr);
	_block = block;
	_elementSize = elementSize;
	_elementMaxCount = _block.size / elementSize;

	memset(_block.ptr, 0, _block.size);

	// [fillBitField]..align..[buckets]
	u32 bfSize = _elementMaxCount / 8.0;
	bfSize += (_elementMaxCount & 7);

	_fillBitField = (u8*)_block.ptr;
	_memBucketStart = (char*)_block.ptr + bfSize;

	_memBucketStart += alignAdjust((intptr_t)_memBucketStart, alignment);
	_elementMaxCount = ((intptr_t)_block.ptr + _block.size - (intptr_t)_memBucketStart) / _elementSize;
}

lsk_Block lsk_AllocatorField::_allocElt(POSARG, u32 count)
{
	assert(count > 0);

	if(count > _elementMaxCount) {
		return NULL_BLOCK;
	}

	u32 eltMax = _elementMaxCount;
	for(u32 b = 0; b < eltMax; ++b) {
		if(b + count > eltMax) {
			return NULL_BLOCK;
		}
		if(_checkBit(b) == 0) { // first fit
			bool spaceAvailable = true;

			u32 o = 0;
			for(u32 b2 = 0; b2 < count; ++b2) {
				if(_checkBit(b2 + b) == 1) {
					spaceAvailable = false;
					o = b2;
					break;
				}
			}

			if(spaceAvailable) {
				for(u32 b3 = 0; b3 < count; ++b3) {
					_setBit(b3 + b);
				}

				char* retAddr = _memBucketStart + (_elementSize * b);
				memset(retAddr, 0, count * _elementSize);

				return lsk_Block{retAddr, count * _elementSize};
			}

			b += o;
		}
	}

	return NULL_BLOCK;
}

void lsk_AllocatorField::_dealloc(POSARG, lsk_Block block)
{
	if(!block.ptr) return;
	assert(block.ptr >= _memBucketStart);

	intptr_t delta = (intptr_t)block.ptr - (intptr_t)_memBucketStart;
	u32 bid = delta / _elementSize;

	u32 count = block.size / _elementSize;
	for(u32 b = 0; b < count; ++b) {
		_unsetBit(bid + b);
	}
}

lsk_Block lsk_AllocatorField::_shrink(lsk_Block block, u32 newCount)
{
	if(!block.ptr) return NULL_BLOCK;
	u64 count = block.size / _elementSize;

	intptr_t delta = (intptr_t)block.ptr - (intptr_t)_memBucketStart;
	u32 bid = delta / _elementSize;

	for(u32 b = newCount; b < count; ++b) {
		_unsetBit(bid + b);
	}

	return lsk_Block{block.ptr, newCount * _elementSize};
}

lsk_Block lsk_AllocatorField::_expand(lsk_Block block, u32 newCount)
{
	if(!block.ptr) return NULL_BLOCK;
	u64 count = block.size / _elementSize;

	intptr_t delta = (intptr_t)block.ptr - (intptr_t)_memBucketStart;
	u32 bid = delta / _elementSize;

	bool spaceAvailable = true;
	u32 dcount = newCount - count;
	u32 bidCheck = bid + count;

	for(u32 b2 = 0; b2 < dcount; ++b2) {
		if(_checkBit(bidCheck + b2) == 1) {
			spaceAvailable = false;
			break;
		}
	}

	if(spaceAvailable) {
		for(u32 b2 = 0; b2 < dcount; ++b2) {
			_setBit(bidCheck + b2);
		}

		memset((char*)block.ptr + block.size, 0, newCount * _elementSize - block.size);
		return lsk_Block{block.ptr, newCount * _elementSize}; // success!
	}
	else {
		lsk_Block newBlock = alloc_ELT(newCount);
		if(!newBlock.ptr) {
			return NULL_BLOCK;
		}
		else {
			memcpy(newBlock.ptr, block.ptr, block.size);
			deallocate(block);
			return newBlock; // success!
		}
	}

	assert(false);
	return NULL_BLOCK; // never happening
}

lsk_Block lsk_AllocatorField::_reallocElt(POSARG, lsk_Block block, u32 newCount)
{
	if(!block.ptr) return NULL_BLOCK;
	u32 count = block.size / _elementSize;
	if(count < newCount) {
		return _shrink(block, newCount);
	}
	if(count > newCount) {
		return _expand(block, newCount);
	}
	return block;
}

inline u8 lsk_AllocatorField::_checkBit(u32 eltId)
{
	u32 bfid = eltId / 8;
	u8 bit = 1 << (eltId & 7);
	return _fillBitField[bfid]&bit;
}

inline void lsk_AllocatorField::_setBit(u32 eltId)
{
	u32 bfid = eltId / 8;
	u8 bit = 1 << (eltId & 7);
	_fillBitField[bfid] |= bit;
}

inline void lsk_AllocatorField::_unsetBit(u32 eltId)
{
	u32 bfid = eltId / 8;
	u8 bit = 1 << (eltId & 7);
	_fillBitField[bfid] &= ~bit;
}

void lsk_AllocatorFieldCascade::init(lsk_IAllocator* parent, u64 size, u32 eltSize, u8 alignment)
{
	_parent = parent;
	lsk_Block parentBlock = _parent->allocate(size);
	assert(parentBlock.ptr);
	_allocators[0].init(parentBlock, eltSize, alignment);
	_allocatorCount = 1;
	_growth = 1.0;
	_totalSize = size;
	_eltSize = eltSize;
	_align = alignment;
}

void lsk_AllocatorFieldCascade::release()
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		_parent->deallocate({_allocators[i]._block.ptr, _allocators[i]._block.size});
	}
	_allocatorCount = 0;
}

lsk_Block lsk_AllocatorFieldCascade::_allocElt(const char* filename, i32 line, u32 reqCount)
{
	assert(reqCount > 0);

	for(i32 i = 0; i < _allocatorCount; ++i) {
		lsk_Block buff = _allocators[i].alloc_ELT(reqCount);
		if(buff.ptr) {
			return buff;
		}
	}

	assert(_allocatorCount < _MAX_allocATORS);
	i32 aid = _allocatorCount++;
	u64 growSize = _totalSize * _growth;
	u64 size = reqCount * _eltSize + lsk_AllocatorField::calcBookKeepingSize(reqCount, _align);
	if(growSize < size) {
		growSize = size;
#ifndef NDEBUG
		lsk_warnf("%s:%d lsk_PoolCascade::alloc_ELT() requested size (%d) is > totalsize (%d), \
			adjust starting size or growth", filename, line, size, _totalSize);
#endif
	}

	lsk_Block parentBlock = _parent->allocate(growSize);
	assert_msg(parentBlock.ptr, "lsk_PoolCascade::_allocElt() parent out of memory");

	_allocators[aid].init(parentBlock, _eltSize, _align);
	_totalSize += growSize;
	return _allocators[aid].alloc_ELT(reqCount);
}

lsk_Block lsk_AllocatorFieldCascade::_reallocElt(const char* filename, i32 line, lsk_Block block, u32 newCount)
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_allocators[i].owns(block)) {
			lsk_Block nb = _allocators[i]._reallocElt(filename, line, block, newCount);
			if(nb.ptr) {
				return nb;
			}
			else {
				_allocators[i].deallocate(block);
			}
			break;
		}
	}

	lsk_Block newBlock = alloc_ELT(newCount);
	memmove(newBlock.ptr, block.ptr, block.size);
	return newBlock;
}

void lsk_AllocatorFieldCascade::_dealloc(const char* filename, i32 line, lsk_Block buff)
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_allocators[i].owns(buff)) {
			_allocators[i].deallocate(buff);
			return;
		}
	}
}

bool lsk_AllocatorFieldCascade::owns(lsk_Block block) const
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_allocators[i].owns(block)) {
			return true;
		}
	}
	return false;
}

//
// Pool Allocator
//
void lsk_AllocatorPool::init(lsk_Block block, u32 elementSize, u8 alignment)
{
	assert(block.ptr && elementSize > sizeof(intptr_t));
	_block = block;
	i32 adjust = alignAdjust((intptr_t)block.ptr, alignment);
	_start = (intptr_t)_block.ptr + adjust;
	_elementSize = elementSize;
	_elementCapacity = (block.size - adjust) / elementSize;

	// populate freelist
	_freeListTop = nullptr;
	for(u32 i = 0; i < _elementCapacity; ++i) {
		FreeList* cur = (FreeList*)(_start + i * _elementSize);
		cur->next = _freeListTop;
		_freeListTop = cur;
	}
}

lsk_Block lsk_AllocatorPool::_alloc(POSARG, u64 size, u8 alignment)
{
	if(!_freeListTop) {
		return NULL_BLOCK;
	}

	intptr_t top = (intptr_t)_freeListTop;
	i32 adjust = alignAdjust(top, alignment);

	if(size + adjust > _elementSize) {
		return NULL_BLOCK;
	}

	_freeListTop = _freeListTop->next; // pop it
	memset((void*)top, 0, _elementSize);
	return lsk_Block{(void*)(top + adjust), (void*)top, _elementSize - adjust};
}

lsk_Block lsk_AllocatorPool::_realloc(POSARG, lsk_Block block, u64 size, u8 alignment)
{
	i32 adjust = alignAdjust((intptr_t)block.ptr, alignment);
	if(size + adjust <= _elementSize) {
		return block;
	}
	return NULL_BLOCK;
}

void lsk_AllocatorPool::_dealloc(POSARG, lsk_Block block)
{
	if(!block.ptr || !block._notaligned || !owns(block)) return;

	intptr_t ptr = (intptr_t)block._notaligned;
	if((ptr - _start) % _elementSize != 0) {
		lsk_warnf("lsk_PoolAllocator::_dealloc: tried to deallocate an invalid block");
		return;
	}

	FreeList* cur = (FreeList*)ptr;
	cur->next = _freeListTop;
	_freeListTop = cur;
}

void lsk_AllocatorPoolCascade::init(lsk_IAllocator* parent, u64 size, u32 elementSize, u8 alignment)
{
	assert(parent && size > 0);
	_elementSize = elementSize;
	_alignement = alignment;
	_capacity = size;

	_parent = parent;
	lsk_Block parentBlock = _parent->allocate(size);
	assert(parentBlock.ptr);
	_allocators[0].init(parentBlock, _elementSize, alignment);
	_allocatorCount = 1;
}

void lsk_AllocatorPoolCascade::release()
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		_parent->deallocate(_allocators[i]._block);
	}
	_allocatorCount = 0;
}

lsk_Block lsk_AllocatorPoolCascade::_alloc(POSARG, u64 size, u8 alignment)
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		lsk_Block block = _allocators[i].allocate(size, alignment);
		if(block.ptr) {
			return block;
		}
		else if(!_allocators[i].isFull()) {
			assert(false);
			return NULL_BLOCK; // cant allocate this size
		}
	}

	assert(_allocatorCount < _MAX_allocATORS);
	i32 aid = _allocatorCount++;

	lsk_Block parentBlock = _parent->allocate(_capacity * _growth - _capacity);
	assert_msg(parentBlock.ptr, "Out of memory");
	_allocators[aid].init(parentBlock, _elementSize, _alignement);
	_capacity *= _growth;
	return _allocators[aid].allocate(size, alignment);
}

lsk_Block lsk_AllocatorPoolCascade::_realloc(POSARG, lsk_Block block, u64 size, u8 alignment)
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_allocators[i].owns(block)) {
			return _allocators[i].reallocate(block, size, alignment);
		}
	}
	return NULL_BLOCK;
}

void lsk_AllocatorPoolCascade::_dealloc(POSARG, lsk_Block block)
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_allocators[i].owns(block)) {
			_allocators[i].deallocate(block);
			return;
		}
	}
}

bool lsk_AllocatorPoolCascade::owns(lsk_Block block) const
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_allocators[i].owns(block)) {
			return true;
		}
	}
	return false;
}


//
// HeapAllocator
//
void lsk_AllocatorHeap::init(lsk_Block block)
{
	assert(block.ptr);
	_block = block;
	_first = nullptr;
}

lsk_Block lsk_AllocatorHeap::_alloc(POSARG, u64 size, u8 alignment)
{
	assert(size > 0);
	if(size < _minAllocSize) size = _minAllocSize;
	size += SIZE_BI + alignment;

	if(!_first) {
		if(size <= _block.size) {
			BlockInfo* bi = (BlockInfo*)_block.ptr;
			bi->size = size - SIZE_BI;
			bi->prev = nullptr;
			bi->next = nullptr;
			_first = bi;
			void* ptr = bi + 1;
			memset(ptr, 0, bi->size);
			i32 adjust = alignAdjust((intptr_t)ptr, alignment);
			return {(void*)((intptr_t)ptr + adjust), ptr, bi->size - adjust};
		}
		return NULL_BLOCK;
	}

	// first fit
	char* fit = nullptr;
	BlockInfo* cur = _first;
	BlockInfo* last = nullptr;
	BlockInfo* fitPrev = nullptr;
	BlockInfo* fitNext = nullptr;

	while(cur && !fit) {
		i64 space;
		if(last) {
			space = (char*)cur - ((char*)last + SIZE_BI + last->size);
		}
		else {
			space = (char*)cur - (char*)_block.ptr;
		}

		if((i64)size <= space) {
			fit = (char*)cur - space;
			fitPrev = cur->prev;
			fitNext = cur;

			if(space - size < (_minAllocSize + SIZE_BI)) {
				size = space;
			}
		}

		last = cur;
		cur = cur->next;
	}

	if(fit) {
		BlockInfo* bi = (BlockInfo*)fit;
		bi->size = size - SIZE_BI;
		bi->prev = fitPrev;
		bi->next = fitNext;
		if(fitNext == _first) {
			_first = bi;
		}
		if(bi->prev) {
			bi->prev->next = bi;
		}
		if(bi->next) {
			bi->next->prev = bi;
		}
		void* ptr = bi + 1;
		memset(ptr, 0, bi->size);
		i32 adjust = alignAdjust((intptr_t)ptr, alignment);
		return {(void*)((intptr_t)ptr + adjust), ptr, bi->size - adjust};
	}
	else if((i64)size <= ((char*)_block.ptr + _block.size - ((char*)last + SIZE_BI + last->size))) {
		BlockInfo* bi = (BlockInfo*)((char*)last + SIZE_BI + last->size);
		bi->size = size - SIZE_BI;
		bi->prev = last;
		bi->next = nullptr;
		bi->prev->next = bi;
		void* ptr = bi + 1;
		memset(ptr, 0, bi->size);
		i32 adjust = alignAdjust((intptr_t)ptr, alignment);
		return {(void*)((intptr_t)ptr + adjust), ptr, bi->size - adjust};
	}

	return NULL_BLOCK;
}

void lsk_AllocatorHeap::_dealloc(POSARG, lsk_Block block)
{
	if(!block.ptr || !block._notaligned) return;
	BlockInfo* bi = (BlockInfo*)((intptr_t)block._notaligned - SIZE_BI);
	if(bi == _first) {
		_first = _first->next;
	}
	if(bi->prev) {
		bi->prev->next = bi->next;
	}
	if(bi->next) {
		bi->next->prev = bi->prev;
	}
}

void lsk_AllocatorHeapCascade::init(lsk_IAllocator* parent, u64 size)
{
	assert(parent && size > 0);
	_parent = parent;
	lsk_Block parentBlock = _parent->allocate(size);
	assert(parentBlock.ptr);
	_allocators[0].init(parentBlock);
	_allocatorCount = 1;
	_capacity = size;
}

void lsk_AllocatorHeapCascade::release()
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		_parent->deallocate(_allocators[i]._block);
	}
	_allocatorCount = 0;
}

lsk_Block lsk_AllocatorHeapCascade::_alloc(POSARG, u64 size, u8 alignment)
{
	assert(size > 0);
	for(i32 i = 0; i < _allocatorCount; ++i) {
		lsk_Block buff = _allocators[i].allocate(size, alignment);
		if(buff.ptr) {
			return buff;
		}
	}

	assert(_allocatorCount < _MAX_allocATORS);
	i32 aid = _allocatorCount++;
	u64 mallocSize = _capacity * _growth - _capacity;
	if(mallocSize <= size) {
		mallocSize = size + lsk_AllocatorHeap::SIZE_BI + alignment;
	}

	lsk_Block parentBlock = _parent->allocate(mallocSize);
	assert_msg(parentBlock.ptr, "lsk_HeapCascade::_alloc() parent out of memory");
	_allocators[aid].init(parentBlock);
	_capacity += mallocSize;

	lsk_Block block = _allocators[aid].allocate(size, alignment);
	assert(block.ptr);
	return block;
}

lsk_Block lsk_AllocatorHeapCascade::_realloc(POSARG, lsk_Block block, u64 size, u8 alignment)
{
	assert(size > 0);
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_allocators[i].owns(block)) {
			lsk_Block nb = _allocators[i].reallocate(block, size, alignment);
			if(nb.ptr) {
				return nb;
			}
			else {
				_allocators[i].deallocate(block);
			}
			break;
		}
	}

	lsk_Block newBlock = allocate(size, alignment);
	memmove(newBlock.ptr, block.ptr, block.size);
	return newBlock;
}

void lsk_AllocatorHeapCascade::_dealloc(POSARG, lsk_Block block)
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_allocators[i].owns(block)) {
			_allocators[i].deallocate(block);
			return;
		}
	}
}

void lsk_AllocatorStep::init(lsk_IAllocator** pAllocators, u64* maxAllocSize, u32 count)
{
	assert(pAllocators && maxAllocSize && count > 0);
	memmove(_pAllocs, pAllocators, sizeof(lsk_IAllocator*) * count);
	memmove(_maxAllocSize, maxAllocSize, sizeof(u64) * count);
	_allocatorCount = count;

	// bubble sort
	bool sorting = true;
	while(sorting) {
		sorting = false;

		for(i32 i = 1; i < _allocatorCount; ++i) {
			if(_maxAllocSize[i-1] > _maxAllocSize[i]) {
				lsk_swap(_maxAllocSize[i-1], _maxAllocSize[i]);
				lsk_swap(_pAllocs[i-1], _pAllocs[i]);
				sorting = true;
			}
		}
	}
}

void lsk_AllocatorStep::release()
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		_pAllocs[i]->release();
	}
}

lsk_Block lsk_AllocatorStep::_alloc(POSARG, u64 size, u8 alignment)
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(size <= _maxAllocSize[i]) {
			lsk_Block block = _pAllocs[i]->allocate(size, alignment);
			if(block.ptr) {
				return block;
			}
		}
	}

	assert_msg(false, "lsk_StepAllocator size doesn't fit any allocator or all allocators are full");
	return NULL_BLOCK;
}

lsk_Block lsk_AllocatorStep::_realloc(POSARG, lsk_Block block, u64 size, u8 alignment)
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_pAllocs[i]->owns(block)) {
			lsk_Block reblock = _pAllocs[i]->reallocate(block, size, alignment);
			if(reblock.ptr) {
				return reblock;
			}
			else {
				_pAllocs[i]->deallocate(block);
			}
			break;
		}
	}

	lsk_Block newBlock = allocate(size, alignment);
	memmove(newBlock.ptr, block.ptr, block.size);
	return newBlock;
}

void lsk_AllocatorStep::_dealloc(POSARG, lsk_Block block)
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_pAllocs[i]->owns(block)) {
			_pAllocs[i]->deallocate(block);
			return;
		}
	}
}

bool lsk_AllocatorStep::owns(lsk_Block block) const
{
	for(i32 i = 0; i < _allocatorCount; ++i) {
		if(_pAllocs[i]->owns(block)) {
			return true;
		}
	}
	return false;
}

lsk_IAllocator* _pDefaultAllocator = nullptr;
lsk_IAllocator* _pOldDefaultAllocator = nullptr;


lsk_AllocatorHeapCascade* GAllocHeap_init(u64 size)
{
	static lsk_AllocatorHeapCascade hs;
	hs.init(&GMalloc, size);
	AllocDefault_set(&hs);
	return &hs;
}

lsk_AllocatorStep* GAllocStep_init(u32 ko1, u32 ko2, u32 ko4, u32 mo1, u32 mo2, u32 mo4,
									lsk_Block* out_mallocBlock)
{
	u64 totalSize = Kilobyte(ko1) + Kilobyte(ko2 * 2) + Kilobyte(ko4 * 4) +
			Megabyte(mo1) + Megabyte(mo2 * 2) + Megabyte(mo4 * 4);
	lsk_Block mallBlock = GMalloc.allocate(totalSize);
	lsk_AllocatorStack stack;
	stack.init(mallBlock);

	static lsk_AllocatorPool pool_ko1;
	pool_ko1.init(stack.allocate(Kilobyte(ko1)), Kilobyte(1));
	static lsk_AllocatorPool pool_ko2;
	pool_ko2.init(stack.allocate(Kilobyte(ko2 * 2)), Kilobyte(2));
	static lsk_AllocatorPool pool_ko4;
	pool_ko4.init(stack.allocate(Kilobyte(ko4 * 4)), Kilobyte(4));

	static lsk_AllocatorPool pool_mo1;
	pool_mo1.init(stack.allocate(Megabyte(mo1)), Megabyte(1));
	static lsk_AllocatorPool pool_mo2;
	pool_mo2.init(stack.allocate(Megabyte(mo2 * 2)), Megabyte(2));
	static lsk_AllocatorPool pool_mo4;
	pool_mo4.init(stack.allocate(Megabyte(mo4 * 4)), Megabyte(4));

	lsk_IAllocator* allocators[] = {
		&pool_ko1,
		&pool_ko2,
		&pool_ko4,

		&pool_mo1,
		&pool_mo2,
		&pool_mo4,
		&GMalloc
	};

	u64 allocSizes[] = {
		Kilobyte(1),
		Kilobyte(2),
		Kilobyte(4),

		Megabyte(1),
		Megabyte(2),
		Megabyte(4),
		Gigabyte(32)
	};

	*out_mallocBlock = mallBlock;

	static lsk_AllocatorStep step;
	step.init(allocators, allocSizes, 7);
	AllocDefault_set(&step);
	return &step;
}
