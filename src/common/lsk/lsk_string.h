#pragma once
#include <io.h>
#include "lsk_types.h"
#include "lsk_utils.h"
#include "lsk_allocator.h"

/**
 * @brief Length of a null terminated string
 * @param src
 * @return
 */
inline i32 lsk_strLen(const char* str)
{
	const char* cur = str;
	while(*cur != 0) {
		++cur;
	}
	return (cur - str);
}

/**
 * @brief Compare two strings
 * @param str1
 * @param str2
 * @param length
 * @return -1 if equal, or where the string differ
 */
i32 lsk_strCmp(const char* str1, const char* str2, u32 length);

/**
 * @brief Compare two strings
 * @param str1
 * @param str2
 * @return equality
 */
bool lsk_strEq(const char* str1, const char* str2);

/**
 * @brief Copies src to dest
 * @param src
 * @param dest
 * @param length
 */
inline void lsk_strCopy(char* dest, const char* src, u32 length) {
	memcpy(dest, src, length);
}

/**
 * @brief lsk_toLower
 * @param dest
 * @param src
 * @param length
 */
//void lsk_toLower(char* dest, const char* src, u32 length);

/**
 * @brief Hash a string
 * @param src
 * @param length
 * @return hash
 */
u32 lsk_strHash(const char* src, u32 length);

/**
 * @brief Hash a string using FNV-1a
 * @param src
 * @param length
 * @return
 */
u32 lsk_strHash_fnv1a(const char* src, u32 length);

/**
 * @brief Is character a digit [0-9]
 * @param c
 * @return
 */
bool lsk_isDigit(char c);

/**
 * @brief String to i64
 * @param str
 * @param strLength
 * @return
 */
i64 lsk_parseI64(const char* str, i32 strLength = -1);

/**
 * @brief String to f64
 * @param str
 * @param strLength
 * @return
 */
f64 lsk_parseF64(const char* str, i32 strLength = -1);

/**
 * @brief i64 to string
 * @param number
 * @param dest
 * @param base
 */
template<typename T>
void lsk__integerToStr_imp(T number, char* dest, u32 base = 10)
{
	assert(base >= 2 && base <= 16);
	constexpr const char* baseStr = "0123456789abcdef";

	if(number == 0) {
		dest[0] = '0';
		dest[1] = 0; // null terminate
		return;
	}

	bool negative = false;
	if(number < 0) {
		negative = true;
		number = -number;
	}

	i32 d = 0;
	char rev[64];
	while(number != 0) {
		rev[d++] = baseStr[number%base];
		number /= base;
	}

	// negative sign if base 10
	i32 offset = 0;
	if(negative && base == 10) {
		offset = 1;
		dest[0] = '-';
	}

	// reverse
	for(i32 i = 0; i < d ; ++i) {
		dest[d-i-1 + offset] = rev[i];
	}

	dest[d + offset] = 0; // null terminate
}

#define lsk_i64ToStr(number, dest, base) lsk__integerToStr_imp((i64)number, dest, base)
#define lsk_i32ToStr(number, dest, base) lsk__integerToStr_imp((i32)number, dest, base)
#define lsk_u64ToStr(number, dest, base) lsk__integerToStr_imp((u64)number, dest, base)
#define lsk_u32ToStr(number, dest, base) lsk__integerToStr_imp((u32)number, dest, base)
#define lsk_u16ToStr(number, dest, base) lsk__integerToStr_imp((u16)number, dest, base)
#define lsk_u8ToStr(number, dest, base) lsk__integerToStr_imp((u8)number, dest, base)

/**
 * @brief float to string
 * @param fnum
 * @param dest
 * @param precision
 */
void lsk_f32ToStr(f32 fnum, char* dest, u32 precision = 5);

/**
 * @brief double to string
 * @param fnum
 * @param dest
 * @param precision
 */
void lsk_f64ToStr(f64 fnum, char* dest, u32 precision = 5);

u32 lsk_strFormat(char* buff, const char* formatStr, ...);
u32 lsk_vstrFormat(char* buff, const char* formatStr, va_list args);


struct lsk_DStr
{
	lsk_IAllocator* _pAlloc = nullptr;
	char* _data = nullptr;
	i32 _length = 0;
	lsk_Block _dynBlock = NULL_BLOCK;
	i32 _localBuffSize = -1;

	inline char* _local() const {
		return (char*)((intptr_t)this + sizeof(lsk_DStr));
	}

	// NOTE: deep copy
	lsk_DStr(const lsk_DStr& other);
	lsk_DStr& operator=(const lsk_DStr& other);

	lsk_DStr(): lsk_DStr(&AllocDefault) {}
	explicit lsk_DStr(lsk_IAllocator* pAlloc);
	explicit lsk_DStr(const char* cstr, lsk_IAllocator* pAlloc = &AllocDefault);

	~lsk_DStr();

	/**
	 * @brief Resize capacity, does not change length
	 * @param newCapacity
	 */
	void _resize(i32 newCapacity);

	void release();

	void set(const char* cstr, i32 len = -1);
	void setf(const char* fmt, ...);
	void append(const char* cstr, i32 len = -1);
	void appendf(const char* fmt, ...);

	inline i32 len() const {
		return _length;
	}

	inline const char* c_str() const {
		return _data;
	}

	inline i32 capacity() const {
		if(_data == _local()) return _localBuffSize - 1;
		return _dynBlock.size - 1;
	}

	inline bool equals(const char* cstr, i32 len = -1) const {
		if(len == -1) len = lsk_strLen(cstr);
		return (_length == len && lsk_strCmp(_data, cstr, len));
	}

	inline bool equals(const lsk_DStr& str) const {
		return (_length == str._length && lsk_strCmp(_data, str._data, _length));
	}

	inline bool startsWith(const char* cstr, i32 len = -1) const {
		if(len == -1) len = lsk_strLen(cstr);
		if(_length >= len) {
			return (lsk_strCmp(_data, cstr, len) == -1);
		}
		return false;
	}

	inline bool endsWith(const char* cstr, i32 len = -1) const {
		if(len == -1) len = lsk_strLen(cstr);
		if(_length >= len) {
			return (lsk_strCmp(_data + (_length - len), cstr, len) == -1);
		}
		return false;
	}

	void replace(char searchChar, char replaceChar);

	inline void rstrip(i32 count) {
		if(count >= _length) {
			_length = 0;
		}
		else {
			_length -= count;
		}
		_data[_length] = 0;
	}

	inline lsk_DStr& operator+=(const char* cstr) {
		append(cstr);
		return *this;
	}

	inline lsk_DStr& operator+=(const lsk_DStr& str) {
		append(str.c_str(), str.len());
		return *this;
	}

	inline bool operator==(const lsk_DStr& str) const {
		return equals(str);
	}

	inline bool operator==(const char* cstr) const {
		return equals(cstr);
	}

	inline lsk_DStr& operator=(const char* cstr) {
		set(cstr);
		return *this;
	}
};


#define DECLARE_STRTYPE(Type, Size)\
struct Type: lsk_DStr {\
	char _localBuff[Size];\
	Type(): Type(&AllocDefault) {}\
	explicit Type(lsk_IAllocator* pAlloc):\
		lsk_DStr(pAlloc) { _localBuffSize = Size; }\
	explicit Type(const char* cstr, lsk_IAllocator* pAlloc = &AllocDefault):\
		lsk_DStr(pAlloc) { _localBuffSize = Size; set(cstr); }\
	Type(const lsk_DStr& other): lsk_DStr(other._pAlloc) {_localBuffSize = Size;\
		set(other.c_str(), other.len());}\
	Type(const Type& other): Type(other._pAlloc) {_localBuffSize = Size;\
		set(other.c_str(), other.len());}\
	inline Type& operator=(const char* cstr) { set(cstr); return *this; }\
};

DECLARE_STRTYPE(lsk_DStr16, 16)
DECLARE_STRTYPE(lsk_DStr32, 32)
DECLARE_STRTYPE(lsk_DStr64, 64)
DECLARE_STRTYPE(lsk_DStr128, 128)
DECLARE_STRTYPE(lsk_DStr256, 256)
