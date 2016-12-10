#include "lsk_string.h"
#include <stdarg.h>

i32 lsk_strCmp(const char* str1, const char* str2, u32 length)
{
	for(u32 i = 0; i < length; ++i) {
		if(str1[i] != str2[i]) {
			return i;
		}
	}
	return -1;
}

bool lsk_strEq(const char* str1, const char* str2)
{
	i32 length =  lsk_strLen(str1);
	if(length != lsk_strLen(str2)) {
		return false;
	}
	return (memcmp(str1, str2, length) == 0);
}


/*void lsk_toLower(char* dest, const char* src, u32 length)
{
	// TODO: implement this
	for(u32 i = 0; i < length; ++i) {

	}
}*/

u32 lsk_strHash(const char* src, u32 length)
{
	// Jenkins One At A Time Hash
	u32 hash = 0;

	for(u32 i = 0; i < length; i++)
	{
		hash += src[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}


u32 lsk_strHash_fnv1a(const char* src, u32 length)
{
	return lsk_hash32_fnv1a(src, length);
}

bool lsk_isDigit(char c)
{
	return (c >= '0' && c <= '9');
}

i64 lsk_parseI64(const char* str, i32 strLength)
{
	if(strLength < 0) {
		strLength = lsk_strLen(str);
	}

	// −9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
	// 19
	constexpr u32 maxDigits = 19;
	i64 result = 0;
	bool negative = false;
	i32 offset = 0;

	if(str[0] == '-') {
		negative = true;
		offset = 1;
	}

	i32 start = lsk_min((u32)strLength, maxDigits) - 1;

	for(i32 i = start; i >= offset; --i) {
		result += (i64)lsk_pow(10.0, start - i) * (i64)(str[i] - '0');
	}

	if(negative) {
		return -result;
	}

	return result;
}

f64 lsk_parseF64(const char* str, i32 strLength)
{
	if(strLength < 0) {
		strLength = lsk_strLen(str);
	}

	i32 dotPos = -1;
	for(i32 i = 0; i < strLength && dotPos == -1; ++i) {
		if(str[i] == '.') {
			dotPos = i;
		}
	}

	bool negative = false;
	if(str[0] == '-') {
		negative = true;
	}

	if(dotPos == -1) {
		return lsk_parseI64(str, strLength);
	}

	i64 wholePart = lsk_parseI64(str, dotPos);
	i64 fractPart = lsk_parseI64(str + dotPos + 1, strLength - dotPos - 1);
	i32 nc = lsk_log10(fractPart) + 1;

	// fract part first zeroes (0.00xx)
	i32 i = 0;
	while(*(str + dotPos + 1 + i) == '0') ++i;
	nc += i;

	i32 div = lsk_pow(10.f, nc);

	f64 result = wholePart;
	if(negative) {
		result -= (f64)fractPart / div;
	}
	else {
		result += (f64)fractPart / div;
	}

	return result;
}


void lsk_f32ToStr(f32 fnum, char* dest, u32 precision)
{
	f32 absFloating = lsk_abs(fnum);
	i32 wholePart = absFloating;
	i32 index = 0;

	// negative
	if(fnum < 0.f) {
		dest[index++] = '-';
	}

	if(wholePart == 0) {
		dest[index++] = '0';
	}
	else {
		i32 logVal = lsk_log10(wholePart);
		index += logVal;

		// read whole part
		for(i32 i = 0 ; i < logVal + 1 && index > -1; ++i) {
			u32 wt = lsk_pow(10.f, i + 1); // 10 100 1000...
			u32 reminder = wholePart % wt;
			u32 digit = reminder / (wt / 10);
			dest[index--] = digit + 48; // ASCII value
		}

		index += logVal + 2;
	}

	dest[index] = '.';

	f32 fracPart  = absFloating - wholePart;
	f32 curF = fracPart; // cursor

	// read fractional part
	for(u32 i = 0; i < precision; ++i) {
		f32 f10 = curF * 10; // multiply by 10 to extract 1st decimal from curF
		u32 digit = f10; // round to get only decimal (x.yzwxacb -> x)
		dest[index + i + 1] = digit + 48; // ASCII value
		curF = f10 - digit; // advance cursor (x.yzwxacb - x = 0.yzwxacb)
	}

	dest[index + precision + 1] = 0;
}

void lsk_f64ToStr(f64 fnum, char* dest, u32 precision)
{
	f64 absFloating = lsk_abs(fnum);
	i32 wholePart = absFloating;
	i32 index = 0;

	// negative
	if(fnum < 0.0) {
		dest[index++] = '-';
	}

	if(wholePart == 0) {
		dest[index++] = '0';
	}
	else {
		i32 logVal = lsk_log10(wholePart);
		index += logVal;

		// read whole part
		for(i32 i = 0 ; i < logVal + 1 && index > -1; ++i) {
			u32 wt = lsk_pow(10.f, i + 1); // 10 100 1000...
			u32 reminder = wholePart % wt;
			u32 digit = reminder / (wt / 10);
			dest[index--] = digit + 48; // ASCII value
		}

		index += logVal + 2;
	}

	dest[index] = '.';

	f64 fracPart  = absFloating - wholePart;
	f64 curF = fracPart; // cursor

	// read fractional part
	for(u32 i = 0; i < precision; ++i) {
		f64 f10 = curF * 10; // multiply by 10 to extract 1st decimal from curF
		u32 digit = f10; // round to get only decimal (x.yzwxacb -> x)
		dest[index + i + 1] = digit + 48; // ASCII value
		curF = f10 - digit; // advance cursor (x.yzwxacb - x = 0.yzwxacb)
	}

	dest[index + precision + 1] = 0;
}

void lsk_strAppendEx(char* str, u32* pSize, const char* toAppend, u32 length)
{
	memcpy(str + *pSize, toAppend, length);
	*pSize += length;
}

enum {
	FORMAT_TYPE_INAVLID=0,
	FORMAT_TYPE_INTEGER,
	FORMAT_TYPE_UINTEGER,
	FORMAT_TYPE_HEX,
	FORMAT_TYPE_FLOATING,
	FORMAT_TYPE_STRING,
	FORMAT_TYPE_USER_OPTION
};

u32 lsk_strFormat(char* buff, const char* formatStr, ...)
{
	va_list args;
	va_start(args, formatStr);
	u32 len = lsk_vstrFormat(buff, formatStr, args);
	va_end(args);
	return len;
}

u32 lsk_vstrFormat(char* buff, const char* formatStr, va_list args)
{
	u32 paramCount = 0;
	u32 paramPos[256];
	u32 paramLen[256];
	u32 paramType[256];

	const char* cur = formatStr;
	while(*cur != 0) {
		if(*cur == '%') {
			++cur; // skip %
			u32 pid = paramCount++;

			switch(*cur) {
				case 's': {
					paramPos[pid] = cur - formatStr - 1;
					paramLen[pid] = 2;
					paramType[pid] = FORMAT_TYPE_STRING;
					if(cur[1] == '*') {
						++paramLen[pid];
						pid = paramCount++;
						paramType[pid] = FORMAT_TYPE_USER_OPTION;
					}
				} break;

				case 'i': {
					paramPos[pid] = cur - formatStr - 1;
					paramLen[pid] = 2;
					paramType[pid] = FORMAT_TYPE_INTEGER;
				} break;

				case 'u': {
					paramPos[pid] = cur - formatStr - 1;
					paramLen[pid] = 2;
					paramType[pid] = FORMAT_TYPE_UINTEGER;
				} break;

				case 'h': {
					paramPos[pid] = cur - formatStr - 1;
					paramLen[pid] = 2;
					paramType[pid] = FORMAT_TYPE_HEX;
				} break;

				default: {
					--paramCount; // param not recognized
				} break;
			}
		}

		++cur;
	}

	u32 formatLen = cur - formatStr; // format string length
	u32 strLen = 0;

	if(paramCount == 0) {
		lsk_strAppendEx(buff, &strLen, formatStr, formatLen);
	}
	else {
		const char* copyCur = formatStr;

		for(u32 i = 0; i < paramCount; ++i) {
			u32 copyLen = paramPos[i] - (copyCur - formatStr);
			lsk_strAppendEx(buff, &strLen, copyCur, copyLen);
			copyCur += copyLen + paramLen[i];

			switch(paramType[i]) {
				case FORMAT_TYPE_STRING: {
					const char* argStr = va_arg(args, const char*);
					u32 len = 0;
					if(i+1 < paramCount && paramType[i+1] == FORMAT_TYPE_USER_OPTION) {
						len = va_arg(args, u32);
						++i;
					}
					else {
						len = lsk_strLen(argStr);
					}
					lsk_strAppendEx(buff, &strLen, argStr, len);
				} break;

				case FORMAT_TYPE_INTEGER: {
					char numBuff[32];
					i64 argInt = va_arg(args, i64);
					lsk_i64ToStr(argInt, numBuff, 10);
					u32 len = lsk_strLen(numBuff);
					lsk_strAppendEx(buff, &strLen, numBuff, len);
				} break;

				case FORMAT_TYPE_UINTEGER: {
					char numBuff[32];
					i64 argInt = va_arg(args, u64);
					lsk_u64ToStr(argInt, numBuff, 10);
					u32 len = lsk_strLen(numBuff);
					lsk_strAppendEx(buff, &strLen, numBuff, len);
				} break;

				case FORMAT_TYPE_HEX: {
					char numBuff[32];
					i64 argInt = va_arg(args, i64);
					lsk_i64ToStr(argInt, numBuff, 16);
					u32 len = lsk_strLen(numBuff);
					lsk_strAppendEx(buff, &strLen, "0x", 2);
					lsk_strAppendEx(buff, &strLen, numBuff, len);
				} break;

				default: break;
			}
		}

		u32 copyLen = formatLen - (copyCur - formatStr);
		lsk_strAppendEx(buff, &strLen, copyCur, copyLen);
	}

	buff[strLen] = 0;
	return strLen;
}

lsk_DStr::lsk_DStr(const lsk_DStr& other)
{
	_pAlloc = other._pAlloc;
	set(other.c_str(), other.len());
}

lsk_DStr& lsk_DStr::operator=(const lsk_DStr& other)
{
	_pAlloc = other._pAlloc;
	set(other.c_str(), other.len());
	return *this;
}

lsk_DStr::lsk_DStr(lsk_IAllocator* pAlloc)
{
	_pAlloc = pAlloc;
	_data = nullptr;
	_length = 0;
}

lsk_DStr::lsk_DStr(const char* cstr, lsk_IAllocator* pAlloc)
{
	_pAlloc = pAlloc;
	set(cstr);
}

lsk_DStr::~lsk_DStr()
{
	release();
}

void lsk_DStr::_resize(i32 newCapacity)
{
	if(newCapacity <= _localBuffSize-1) {
		// from dynallocated to local
		if(_data != _local() && _data && _dynBlock.ptr) {
			memcpy(_local(), _data, _length);
			_pAlloc->deallocate(_dynBlock);
			_dynBlock = NULL_BLOCK;
		}
		_data = _local();
	}
	else {
		if(!_data || _data == _local()) {
			_dynBlock = _pAlloc->allocate(newCapacity+1);
			_data = (char*)_dynBlock.ptr;
			if(_localBuffSize > 0) {
				memcpy(_data, _local(), _length);
			}
		}
		else if((u64)newCapacity > _dynBlock.size-1) {
			if((u64)newCapacity < _dynBlock.size * 2 - 1) {
				_dynBlock = _pAlloc->reallocate(_dynBlock, _dynBlock.size * 2);
				assert(_dynBlock.ptr);
				_data = (char*)_dynBlock.ptr;
			}
			else {
				_dynBlock = _pAlloc->reallocate(_dynBlock, newCapacity+1);
				assert(_dynBlock.ptr);
				_data = (char*)_dynBlock.ptr;
			}
		}
		else if((u64)newCapacity < _dynBlock.size / 4 - 1) {
			_dynBlock = _pAlloc->reallocate(_dynBlock, _dynBlock.size / 4);
			assert(_dynBlock.ptr);
			_data = (char*)_dynBlock.ptr;
		}
	}
}

void lsk_DStr::release()
{
	if(_dynBlock.ptr) {
		_pAlloc->deallocate(_dynBlock);
	}
	_data = nullptr;
	_length = 0;
	_dynBlock = NULL_BLOCK;
}

void lsk_DStr::set(const char* cstr, i32 len)
{
	if(len == -1) len = lsk_strLen(cstr);
	_resize(len);
	memcpy(_data, cstr, len);
	_length = len;
	_data[_length] = 0;
}

void lsk_DStr::setf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	i32 len = vsnprintf(0, 0, fmt, args);
	va_end(args);

	_resize(len);

	va_start(args, fmt);
	vsprintf(_data, fmt, args);
	va_end(args);

	_length = len;
	_data[_length] = 0;
}

void lsk_DStr::append(const char* cstr, i32 len)
{
	if(len == 0) return;
	if(len == -1) len = lsk_strLen(cstr);

	_resize(_length + len);
	memcpy(_data + _length, cstr, len);
	_length += len;
	_data[_length] = 0;
}

void lsk_DStr::appendf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	i32 len = vsnprintf(0, 0, fmt, args);
	va_end(args);

	_resize(_length + len);

	va_start(args, fmt);
	vsprintf(_data + _length, fmt, args);
	va_end(args);

	_length += len;
	_data[_length] = 0;
}

void lsk_DStr::replace(char searchChar, char replaceChar)
{
	for(i32 i = 0; i < _length; ++i) {
		if(_data[i] == searchChar) {
			_data[i] = replaceChar;
		}
	}
}
