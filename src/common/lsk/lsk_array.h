#pragma once
#include <io.h>
#include "lsk_types.h"
#include "lsk_string.h"
#include "lsk_allocator.h"

/**
 * @brief Basic itertor class (for things like range-fors)
 */
template<typename T>
struct lsk_Iter
{
	T* val;

	lsk_Iter() = default;
	lsk_Iter(T* val_) {
		val = val_;
	}

	inline bool operator!=(const lsk_Iter& o) {
		return val != o.val;
	}

	inline lsk_Iter& operator++() {
		++val;
		return *this;
	}

	inline T& operator*() {
		return *val;
	}
};


/**
 * Basic fixed array class
 * - pointers to elements are never invalidated (order does not change)
 * - elements can be null though, after use of shrink()
 */
template<typename T, u32 maxElementCount>
struct lsk_Array
{
	T _data[maxElementCount];
	u32 _count = 0;

	inline T& push(const T& element) {
		assert(_count < maxElementCount);
		new(_data + _count) T((T&&)element);
		return _data[_count++];
	}

	void remove(const T& elt) {
		const T* eltPtr = &elt;
		assert_msg(eltPtr >= _data && eltPtr < _data + _count, "Element out of range");
		u32 id = eltPtr - _data;
		_data[id].~T();
		_data[id] = _data[_count-1];
		--_count;
	}

	void remove(u32 id) {
		assert_msg(id >= 0 && id < _count, "Element out of range");
		_data[id] = _data[_count-1];
		--_count;
	}

	inline void clear() {
		for(u32 i = 0; i < _count; ++i) {
			_data[i].~T();
		}
		_count = 0;
	}

	inline u32 count() const {
		return _count;
	}

	inline bool isEmpty() const {
		return _count == 0;
	}

	inline T* data() {
		return _data;
	}

	T& operator[](u32 index) {
		assert(index < _count);
		return _data[index];
	}

	const T& operator[](u32 index) const {
		assert(index < _count);
		return _data[index];
	}

	inline void shrink(u32 num) {
		assert(num < _count);
		_count = num;
	}

	inline lsk_Iter<T> begin() {
		return lsk_Iter<T>(_data);
	}

	inline lsk_Iter<T> end() {
		return lsk_Iter<T>(_data + _count);
	}
};

template<typename T, u32 maxElementCount>
struct lsk_ListFIFO
{
	T _data[maxElementCount];
	u32 _count = 0;

	inline void push(T element) {
		assert(_count <= maxElementCount);
		memmove(_data + 1, _data, _count * sizeof(T));
		_data[0] = element;
		++_count;
	}

	inline void pushMany(T* pElements, u32 count) {
		assert(_count + count <= maxElementCount);
		memmove(_data + count, _data, _count * sizeof(T));
		memmove(_data, pElements, count * sizeof(T));
		_count += count;
	}

	inline T* pop() {
		if(_count > 0) {
			--_count;
			return &_data[_count];
		}
		return nullptr;
	}

	inline void clear() {
		_count = 0;
	}

	inline T* first() {
		assert(_count > 0);
		return &_data[0];
	}

	inline T* last() {
		assert(_count > 0);
		return &_data[_count-1];
	}

	inline T* data() {
		return _data;
	}

	inline u32 count() {
		return _count;
	}

	inline bool empty() {
		return _count == 0;
	}
};

/**
 *	Dynamically growing HashMap
 *	- _hashFunction: hash function used to hash keys
 *	- _growth: grow rate (default: 2)
 *	- jumpLimit: maximum number of jumps to find a spot, will grow if limit is reachs
 *	- iteration is not O(1)
 */
template<typename KeyT, typename ValueT>
struct lsk_DHashMap
{
	lsk_IAllocator* _pAlloc = nullptr;
	u8* _occupied = nullptr;
	u32* _keyHash = nullptr;
	ValueT* _data = nullptr;
	u32 _capacity = 0;
	u32 jumpLimit = 10;
	lsk_Block _occupiedBlock, _keyHashBlock, _dataBlock;
	f32 _growth = 2.f;

	// can use generic lambda here instead of template
	typedef u32 (*FUNC_HASH)(const KeyT& key);
	FUNC_HASH _hashFunction = [](const KeyT& key) {
		return lsk_hash32_fnv1a(&key, sizeof(KeyT));
	};

	explicit lsk_DHashMap(u32 capacity, lsk_IAllocator* pAlloc = &AllocDefault) {
		init(capacity, pAlloc);
	}

	lsk_DHashMap() {}

	void init(u32 capacity, lsk_IAllocator* pAlloc = &AllocDefault) {
		_capacity = capacity;
		_pAlloc = pAlloc;
		_occupiedBlock = _pAlloc->allocate(_capacity * sizeof(u8), alignof(u8));
		assert_msg(_occupiedBlock.ptr, "Out of memory");
		_occupied = (u8*)_occupiedBlock.ptr;
		_keyHashBlock = _pAlloc->allocate(_capacity * sizeof(u32), alignof(u32));
		assert_msg(_keyHashBlock.ptr, "Out of memory");
		_keyHash = (u32*)_keyHashBlock.ptr;
		_dataBlock = _pAlloc->allocate(_capacity * sizeof(ValueT), alignof(ValueT));
		assert_msg(_dataBlock.ptr, "Out of memory");
		_data = (ValueT*)_dataBlock.ptr;
	}

	~lsk_DHashMap() {
		destroy();
	}

	void destroy(bool callDestructor = true) {
		if(_pAlloc) {
			if(callDestructor) {
				clear();
			}
			_pAlloc->deallocate(_occupiedBlock);
			_pAlloc->deallocate(_keyHashBlock);
			_pAlloc->deallocate(_dataBlock);
			_pAlloc = nullptr;
		}
	}

	void clear() {
		for(u32 i = 0; i < _capacity; ++i) {
			if(_occupied[i]) {
				_data[i].~ValueT();
				_occupied[i] = false;
				_keyHash[i] = 0;
			}
		}
	}

	void reserve(u32 newCapacity) {
		if(newCapacity <= _capacity) return;

		lsk_Block new_occupiedBlock = _pAlloc->allocate(newCapacity * sizeof(u8), alignof(u8));
		assert_msg(new_occupiedBlock.ptr, "Out of memory");
		u8* new_occupied = (u8*)new_occupiedBlock.ptr;
		lsk_Block new_keyHashBlock = _pAlloc->allocate(newCapacity * sizeof(u32), alignof(u32));
		assert_msg(new_keyHashBlock.ptr, "Out of memory");
		u32* new_keyHash = (u32*)new_keyHashBlock.ptr;
		lsk_Block new_dataBlock = _pAlloc->allocate(newCapacity * sizeof(ValueT), alignof(ValueT));
		assert_msg(new_dataBlock.ptr, "Out of memory");
		ValueT* new_data = (ValueT*)new_dataBlock.ptr;

		for(u32 i = 0; i < _capacity; ++i) {
			if(_occupied[i]) {
				u32 id = _keyHash[i] % newCapacity;
				u32 jumps = 0;
				while(new_occupied[id]) {
					++id;
					++jumps;
					if(id == newCapacity) {
						id = 0;
					}
					if(jumps == jumpLimit) {
						_pAlloc->deallocate(new_occupiedBlock);
						_pAlloc->deallocate(new_keyHashBlock);
						_pAlloc->deallocate(new_dataBlock);
						reserve(newCapacity * _growth);
						return;
					}
				}

				new_occupied[id] = 1;
				new_keyHash[id] = _keyHash[i];
				new_data[id] = _data[i];
			}
		}

		_pAlloc->deallocate(_occupiedBlock);
		_pAlloc->deallocate(_keyHashBlock);
		_pAlloc->deallocate(_dataBlock);
		_occupiedBlock = new_occupiedBlock;
		_keyHashBlock = new_keyHashBlock;
		_dataBlock = new_dataBlock;

		_occupied = new_occupied;
		_keyHash = new_keyHash;
		_data = new_data;

		_capacity = newCapacity;
	}

	// warning: can be slow if it doesn't find a spot with the jump limit (will grow)
	ValueT& seth(u32 keyHash, const ValueT& value) {
		assert_msg(_capacity > 0, "HashMap not initialized"); // init array first
		u32 id = keyHash % _capacity;
		u32 jumps = 0;
		while(_occupied[id] && _keyHash[id] != keyHash) {
			++id;
			++jumps;
			if(id == _capacity) {
				id = 0;
			}

			if(jumps == jumpLimit) {
				// expand
				reserve(_capacity * _growth);
				return seth(keyHash, value);
			}
		}

		if(_occupied[id] && _keyHash[id] == keyHash) {
			_data[id].~ValueT();
		}
		_keyHash[id] = keyHash;
		new(_data + id) ValueT((ValueT&&)value);
		_occupied[id] = true;
		return _data[id];
	}

	inline ValueT& set(const KeyT& key, const ValueT& value) {
		u32 keyHash = _hashFunction(key);
		return seth(keyHash, value);
	}

	void removeh(u32 keyHash) {
		u32 id = keyHash % _capacity;
		u32 jumps = 0;
		while(jumps++ < jumpLimit) {
			if(_occupied[id] && _keyHash[id] == keyHash) {
				_occupied[id] = false;
				_keyHash[id] = 0;
				_data[id].~ValueT();
				return;
			}
			++id;
			if(id == _capacity) {
				id = 0;
			}
		}
	}

	inline void remove(const KeyT& key) {
		u32 keyHash = _hashFunction(key);
		return removeh(keyHash);
	}

	ValueT* geth(u32 keyHash) const {
		u32 id = keyHash % _capacity;
		u32 jumps = 0;
		while(jumps++ < jumpLimit) {
			if(_occupied[id] && _keyHash[id] == keyHash) {
				return &_data[id];
			}
			++id;
			if(id == _capacity) {
				id = 0;
			}
		}
		return nullptr;
	}

	inline ValueT* get(const KeyT& key) const {
		u32 keyHash = _hashFunction(key);
		return geth(keyHash);
	}

	// FIXME: iteration doesn't work properly
	/*struct Iter {
		ValueT* val = nullptr;
		const u8* _occupied = nullptr;
		u32 id = 0;
		u32 _capacity = 0;

		Iter() = default;
		Iter(ValueT* val_, u32 id_, const u8* occupied_, u32 capacity_) {
			val = val_;
			id = id_;
			_occupied = occupied_;
			_capacity = capacity_;
		}

		inline bool operator!=(const Iter& o) {
			return val != o.val;
		}

		inline Iter& operator++() {
			if(id >= _capacity) return *this;
			u32 nextId = id + 1;
			ValueT* nextVal = val + 1;
			while(!_occupied[nextId] && nextId < _capacity) {
				++nextId;
				++nextVal;
			}
			id = nextId;
			val = nextVal;
			return *this;
		}

		inline ValueT& operator*() {
			return *val;
		}
	};

	inline Iter begin() {
		i32 i;
		bool found = false;
		for(i = 0; i < _capacity; ++i) {
			if(_occupied[i]) {
				found = true;
				break;
			}
		}

		if(!found) {
			i = 0;
		}

		return Iter(_data + i, i, _occupied, _capacity);
	}

	inline Iter end() {
		i32 i;
		bool found = false;
		for(i = _capacity - 1; i >= 0; --i) {
			if(_occupied[i]) {
				found = true;
				break;
			}
		}

		if(!found) {
			i = 0;
		}

		return Iter(_data + i, i, _occupied, _capacity);
	}*/
};

template<typename KeyT>
u32 lsk_DStrHashMap_hashString(const KeyT& key) {
	return lsk_hash32_fnv1a(key, lsk_strLen(key));
}

#define H(str) lsk_const_strHash32_fnv1a(str)

/**
 * String HashMap
 *	- prefer using geth(H("MyString")) for compile-time hash
 */
template<typename ValueT>
struct lsk_DStrHashMap: lsk_DHashMap<const char*, ValueT>
{
	explicit lsk_DStrHashMap(u32 capacity, lsk_IAllocator* pAlloc = &AllocDefault)
		: lsk_DHashMap(capacity, pAlloc) { _hashFunction = lsk_DStrHashMap_hashString; }

	lsk_DStrHashMap(): lsk_DHashMap() { _hashFunction = lsk_DStrHashMap_hashString; }
};

// TODO: redo this
/**
 *	Basic Set container
 * - data is packed
 * - can use custom hash function
 */
template<typename T, u32 maxElementCount>
struct lsk_Set
{
#define _indexCount (maxElementCount * 4)
	T _data[maxElementCount];
	i32 _index[_indexCount];
	u32 _dataCount = 0;

	typedef u32 (*FUNC_HASH)(const T& element);
	FUNC_HASH _funcHash = nullptr;

	static u32 defaultFuncHash(const T& element) {
		return lsk_hash32_fnv1a(&element, sizeof(T));
	}

	explicit lsk_Set(FUNC_HASH funcHash_ = nullptr) {
		memset(_index, -1, sizeof(_index)); // doesnt work
		if(funcHash_) {
			_funcHash = funcHash_;
		}
		else {
			_funcHash = defaultFuncHash;
		}
	}

	i32 add(T element) {
		assert(_dataCount <= maxElementCount);
		u32 h = _funcHash(element);
		u32 emp = h % _indexCount;
		if(_index[emp] != -1) { // collision
			return -1;
		}
		_data[_dataCount] = element;
		_index[emp] = _dataCount;
		++_dataCount;
		return emp;
	}

	void remove(u32 id) {
		assert(id < _indexCount);
		assert(_index[id] != -1);

		if(_index[id] != (i32)_dataCount-1) { // not pointing to the last one
			// move last element to this location
			u32 dataId = _index[id];
			memmove(_data+dataId, _data+(_dataCount-1), sizeof(T));
			// find what pointed to last data element
			// and make it point to the new location
			// TODO: there has to be a better way to do this
			for(u32 i = 0; i < _indexCount; ++i) {
				if(_index[i] == (i32)_dataCount-1) {
					_index[i] = dataId;
					break;
				}
			}
		}

		_index[id] = -1;
		--_dataCount;
	}

	void clear() {
		memset(_index, -1, sizeof(_index));
		_dataCount = 0;
	}

	i32 search(const T& element) {
		u32 h = _funcHash(element);
		u32 emp = h % _indexCount;
		if(_index[emp] != -1) {
			return emp;
		}
		return -1;
	}

	inline const T& at(u32 id) const {
		assert(id < _indexCount);
		return _data[_index[id]];
	}

	inline T& get(u32 id) const {
		assert(id < _indexCount);
		return _data[_index[id]];
	}

	inline u32 count() const {
		return _dataCount;
	}

	inline T* data() {
		return _data;
	}
};

/**
 * Basic dynamic array
 * - grows x2
 * - remove invalidates last pointer
 * - push invalidates all pointers on grow
 * - frees on destruct
 */
template<typename T>
struct lsk_DArray
{
	lsk_IAllocator* _pAlloc = &AllocDefault;
	T* _data = nullptr;
	u32 _capacity = 0;
	u32 _count = 0;
	lsk_Block _block = NULL_BLOCK;

	lsk_DArray() = default;

	explicit lsk_DArray(u32 capacity, lsk_IAllocator* pAlloc = &AllocDefault) {
		init(capacity, pAlloc);
	}

	void init(u32 capacity, lsk_IAllocator* pAlloc = &AllocDefault) {
		if(capacity == 0) capacity = 8;
		assert(_data == nullptr && capacity > 0 && pAlloc);
		_capacity = capacity;
		_count = 0;
		_pAlloc = pAlloc;
		_block = _pAlloc->allocate(_capacity * sizeof(T), alignof(T));
		assert_msg(_block.ptr, "Out of memory");
		_data = (T*)_block.ptr;
	}

	// deep copy
	lsk_DArray(const lsk_DArray& other) {
		init(other._capacity, other._pAlloc);
		_count = other._count;
		memmove(_data, other._data, _count * sizeof(T));
	}

	~lsk_DArray() {
		destroy();
	}

	inline void destroy() {
		if(_pAlloc) {
			for(u32 i = 0; i < _count; ++i) {
				_data[i].~T();
			}
			_pAlloc->deallocate(_block);
			_pAlloc = nullptr;
		}
	}

	void reserve(u32 newCapacity) {
		if(newCapacity <= _capacity) return;
		_capacity = newCapacity;
		_block = _pAlloc->reallocate(_block, _capacity * sizeof(T), alignof(T));
		assert_msg(_block.ptr, "Out of memory");
		_data = (T*)_block.ptr;
	}

	inline T& push(const T& elt) {
		assert_msg(_capacity > 0, "Array not initialized"); // init array first
		if(_count >= _capacity) {
			reserve(_capacity * 2);
		}

		u32 did = _count++;
		new(_data + did) T((T&&)elt);
		return _data[did];
	}

	inline void remove(u32 id) {
		assert_msg(id < _count, "Element out of range");
		_data[id] = _data[_count-1];
		--_count;
	}

	inline void remove(const T& elt) {
		const T* eltPtr = &elt;
		assert_msg(eltPtr >= _data && eltPtr < _data + _count, "Element out of range");

		u32 id = eltPtr - _data;
		_data[id].~T();
		_data[id] = _data[_count-1];
		--_count;
	}

	inline void clear() {
		for(u32 i = 0; i < _count; ++i) {
			_data[i].~T();
		}
		_count = 0;
	}

	inline lsk_Iter<T> begin() const {
		return lsk_Iter<T>(_data);
	}

	inline lsk_Iter<T> end() const {
		return lsk_Iter<T>(&_data[_count]);
	}

	inline u32 capacity() const {
		return _capacity;
	}

	inline u32 count() const {
		return _count;
	}

	inline T* data() {
		return _data;
	}

	inline T* data() const {
		return _data;
	}

	inline T& operator[](u32 index) {
		assert(index < _count);
		return _data[index];
	}

	inline const T& operator[](u32 index) const {
		assert(index < _count);
		return _data[index];
	}
};

template<typename T>
struct lsk_DSparseArray;

template<typename T>
struct Ref
{
	u32 _id = 0;
	lsk_DSparseArray<T>* _from = nullptr;

	Ref() = default;
	Ref(u32 id, lsk_DSparseArray<T>* from):
		_id(id), _from(from) {}

	inline T& get() {
		assert(_from);
		return _from->get(_id);
	}

	inline T* operator->() {
		assert(_from);
		return &_from->get(_id);
	}

	inline void clear() {
		_id = 0;
		_from = nullptr;
	}

	inline bool valid() {
		return _from ? true : false;
	}
};

/**
 * Dynamic sparse array
 * - Grows dynamically
 * - Packed data
 * - No pointer invalidation (kind of)
 * - O(1) push (when not expanding), remove, iteration
 */
template<typename T>
struct lsk_DSparseArray
{
	struct FreeList {
		FreeList* next = nullptr;
	};

	lsk_IAllocator* _pAllocator = &AllocDefault;
	T* _data = nullptr;
	i64* _refId = nullptr;
	i64** _refIdPtr = nullptr;
	lsk_Block _dataBlock, _refBlock, _refPtrBlock;
	u32 _capacity = 0;
	u32 _count = 0;
	f32 _growth = 2.f;
	FreeList* _freeListTop = nullptr;
	// FreeList is saved in-place, where _ref is empty

	lsk_DSparseArray() = default;
	lsk_DSparseArray(u32 capacity, lsk_IAllocator* pAllocator = &AllocDefault) {
		init(capacity, pAllocator);
	}
	~lsk_DSparseArray() {
		destroy();
	}

	lsk_DSparseArray(const lsk_DSparseArray& other) = delete;
	lsk_DSparseArray& operator=(const lsk_DSparseArray& other) = delete;

	void init(u32 capacity, lsk_IAllocator* pAllocator = &AllocDefault) {
		assert(_capacity == 0 && _data == nullptr && capacity > 0 && pAllocator);
		_pAllocator = pAllocator;
		_dataBlock = _pAllocator->allocate(capacity * sizeof(T), alignof(T));
		assert_msg(_dataBlock.ptr, "lsk_DSparseArray::init() out of memory");
		_data = (T*)_dataBlock.ptr;
		_refBlock = _pAllocator->allocate(capacity * sizeof(i64), alignof(i64));
		assert_msg(_refBlock.ptr, "lsk_DSparseArray::init() out of memory");
		_refId = (i64*)_refBlock.ptr;
		_refPtrBlock = _pAllocator->allocate(capacity * sizeof(i64*), alignof(i64*));
		assert_msg(_refPtrBlock.ptr, "lsk_DSparseArray::init() out of memory");
		_refIdPtr = (i64**)_refPtrBlock.ptr;

		_capacity = capacity;
		_count = 0;


		_freeListTop = nullptr;
		for(u32 i = 0; i < _capacity; ++i) {
			FreeList* cur = (FreeList*)&_refId[i];
			cur->next = _freeListTop;
			_freeListTop = cur;
		}
	}

	inline void destroy() {
		if(!_pAllocator) return;
		for(u32 i = 0; i < _count; ++i) {
			_data[i].~T();
		}
		_pAllocator->deallocate(_dataBlock);
		_dataBlock = NULL_BLOCK;
		_pAllocator->deallocate(_refBlock);
		_refBlock = NULL_BLOCK;
		_pAllocator->deallocate(_refPtrBlock);
		_refPtrBlock = NULL_BLOCK;
		_pAllocator = nullptr;
	}

	void clear() {
		for(u32 i = 0; i < _count; ++i) {
			_data[i].~T();
		}
		_count = 0;

		_freeListTop = nullptr;
		for(u32 i = 0; i < _capacity; ++i) {
			FreeList* cur = (FreeList*)&_refId[i];
			cur->next = _freeListTop;
			_freeListTop = cur;
		}
	}

	void reserve(u32 newCapacity) {
		if(newCapacity <= _capacity) return;

		// - null ref ids
		FreeList* cur = _freeListTop;
		while(cur) {
			i64& ref = *(i64*)cur;
			ref = -1;
			cur = cur->next;
		}

		_dataBlock = _pAllocator->reallocate(_dataBlock, newCapacity * sizeof(T), alignof(T));
		assert_msg(_dataBlock.ptr, "lsk_DSparseArray::push() out of memory");
		_refBlock = _pAllocator->reallocate(_refBlock, newCapacity * sizeof(i64), alignof(i64));
		assert_msg(_refBlock.ptr, "lsk_DSparseArray::push() out of memory");
		_refPtrBlock = _pAllocator->reallocate(_refPtrBlock, newCapacity * sizeof(i64*), alignof(i64*));
		assert_msg(_refPtrBlock.ptr, "lsk_DSparseArray::push() out of memory");

		intptr_t oldRef = (intptr_t)_refId;
		_data = (T*)_dataBlock.ptr;
		_refId = (i64*)_refBlock.ptr;
		_refIdPtr = (i64**)_refPtrBlock.ptr;

		// fix pointers offsets
		for(u32 i = 0; i < _count; ++i) {
			_refIdPtr[i] = (i64*)((intptr_t)_refIdPtr[i] - oldRef + (intptr_t)_refId);
		}
		// make refIds -1
		for(u32 i = _capacity; i < newCapacity; ++i) {
			_refId[i] = -1;
		}

		_capacity = newCapacity;

		// remake freelist
		_freeListTop = nullptr;
		for(u32 i = 0; i < _capacity; ++i) {
			if(_refId[i] == -1) {
				FreeList* cur = (FreeList*)&_refId[i];
				cur->next = _freeListTop;
				_freeListTop = cur;
			}
		}
	}

	Ref<T> push(const T& elt) {
		assert_msg(_capacity > 0, "Array not initialized"); // init array first
		// expand
		if(_count >= _capacity) {
			reserve(_capacity * _growth);
		}

		u32 did = _count++;
		// _data is not initialized (constructed), assignement operator doesn't copy vtable
		// so use placement new and tries to move it
		new(_data + did) T((T&&)elt);

		// should never happen
		assert_msg(_freeListTop, "lsk_DSparseArray::push() couldn't find a free redirect slot");

		// pop from freelist
		u32 freeSlotId = ((intptr_t)_freeListTop - (intptr_t)_refId) / sizeof(i64);
		_freeListTop = _freeListTop->next;
		_refId[freeSlotId] = did;
		_refIdPtr[did] = &_refId[freeSlotId];
		return Ref<T>(freeSlotId, this);
	}

	void remove(const T& elt) {
		// shrink
		// TODO: shrink

		const T* eltPtr = &elt;
		assert_msg((char*)eltPtr >= (char*)_data && (char*)eltPtr < ((char*)_data + _count * sizeof(T)),
				   "lsk_DSparseArray::remove() ptr out of bounds");
		u32 did = eltPtr - _data;

		i64& oldref = *_refIdPtr[did];

		// swap to last
		_data[did].~T();
		_data[did] = _data[_count-1];
		_refIdPtr[did] = _refIdPtr[_count-1];
		*_refIdPtr[did] = did;
		--_count;

		// push to free list
		oldref = -1;
		FreeList* newNext = (FreeList*)&oldref;
		newNext->next = _freeListTop;
		_freeListTop = newNext;
	}

	inline void remove(Ref<T>& ref) {
		assert(ref._id < _capacity && _refId[ref._id] >= 0 && _refId[ref._id] < _count);
		remove(get(ref._id));
		ref.clear();
	}

	inline lsk_Iter<T> begin() const {
		return lsk_Iter<T>(_data);
	}

	inline lsk_Iter<T> end() const {
		return lsk_Iter<T>(&_data[_count]);
	}

	inline u32 capacity() const {
		return _capacity;
	}

	inline u32 count() const {
		return _count;
	}

	inline T* data() {
		return _data;
	}

	inline T* data() const {
		return _data;
	}

	inline T& data(u32 did) {
		assert(did < _count);
		return _data[did];
	}

	inline const T& data(u32 did) const {
		assert(did < _count);
		return _data[did];
	}

	inline T& get(u32 refIndex) {
		assert(refIndex < _capacity && _refId[refIndex] >= 0 && _refId[refIndex] < _count);
		return _data[_refId[refIndex]];
	}

	inline const T& get(u32 refIndex) const {
		assert(refIndex < _capacity && _refId[refIndex] >= 0 && _refId[refIndex] < _count);
		return _data[_refId[refIndex]];
	}

	inline Ref<T> getRef(const T& elt) {
		const T* eltPtr = &elt;
		assert_msg((char*)eltPtr >= (char*)_data && (char*)eltPtr < ((char*)_data + _count * sizeof(T)),
				   "lsk_DSparseArray::getRef() ptr out of bounds");
		u32 did = eltPtr - _data;
		Ref<T> ref;
		ref._id = (u32)(_refIdPtr[did] - _refId);
		ref._from = this;
		return ref;
	}
};
