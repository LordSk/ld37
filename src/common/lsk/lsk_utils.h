#pragma once
#include "lsk_types.h"

template<typename T>
inline void lsk_swap(T& a, T& b) {
	T temp = a;
	a = b;
	b = temp;
}

template<typename T>
inline T lsk_abs(T value) {
	if(value < (T)0) {
		return -value;
	}
	return value;
}

#define lsk_min(a, b) ((a) < (b) ? (a) : (b))
#define lsk_max(a, b) ((a) > (b) ? (a) : (b))

template<typename T>
inline T lsk_clamp(T value, T min, T max) {
	if(value < min) {
		return min;
	}
	if(value > max) {
		return max;
	}
	return value;
}

inline f32 lsk_pow(f32 value, u32 p) {
	f32 r = 1.f;
	for(u32 i = 0; i < p; ++i) {
		r *= value;
	}
	return r;
}

inline f64 lsk_pow(f64 value, u32 p) {
	f64 r = 1.0;
	for(u32 i = 0; i < p; ++i) {
		r *= value;
	}
	return r;
}

template<typename T>
inline i32 lsk_sign(T value) {
	return value < T(0) ? -1: 1;
}

template<typename T>
inline void lsk_arrayFillIncr(T* arr, u32 count, i32 start, i32 incr) {
	for(u32 i = 0; i < count; ++i) {
		arr[i] = start + i32(i)*incr;
	}
}

inline f32 lsk_frac(f32 f) {
	return f - (i32)f;
}

inline f64 lsk_frac(f64 f) {
	return f - (i64)f;
}

inline i32 lsk_floor(f32 f) {
	if(f > 0) {
		return (i32)f;
	}
	return (i32)f - 1;
}

inline i64 lsk_floor(f64 f) {
	if(f > 0) {
		return (i64)f;
	}
	return (i64)f - 1;
}

inline i64 lsk_ceil(f64 f) {
	if((f - (i64)f) > 0.0) {
		return (i64)f + 1;
	}
	return (i64)f;
}

template<typename T>
inline T lsk_inBounds(T value, T min, T max) {
	return (value >= min && value <= max);
}

inline i32 lsk_log10(i64 n) {
	n = lsk_abs(n);
	if(n >= 1000000000) {
		return 9;
	}
	if(n >= 100000000) {
		return 8;
	}
	if(n >= 10000000) {
		return 7;
	}
	if(n >= 1000000) {
		return 6;
	}
	if(n >= 100000) {
		return 5;
	}
	if(n >= 10000) {
		return 4;
	}
	if(n >= 1000) {
		return 3;
	}
	if(n >= 100) {
		return 2;
	}
	if(n >= 10) {
		return 1;
	}
	return 0;
}

#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261

/**
 * @brief 32bit FNV-1a hash
 * @param data
 * @param dataSize
 * @return hash
 */
inline
u32 lsk_hash32_fnv1a(const void* data, u32 dataSize) {
	u32 hash = FNV_OFFSET_BASIS;
	for(u32 i = 0; i < dataSize; ++i) {
		hash = (hash ^ ((const char*)data)[i]) * FNV_PRIME;
	}
	return hash;
}

// compile-time utils
template<typename T, i32 count>
constexpr i32 lsk_const_arrayCount(const T(&)[count]) {
	return count;
}

template<i32 Len>
constexpr i32 lsk_const_strLen(const char(&str)[Len]) {
	return Len-1;
}

constexpr i32 lsk_const_strLen_(const char* str, u32 i) {
	return str[i] == '\0' ? i : lsk_const_strLen_(str, i + 1);
}

constexpr i32 lsk_const_strLen(const char* str) {
	return str ? lsk_const_strLen_(str, 0) : 0;
}

constexpr
u32 lsk_const_hash32_fnv1a(const void* data, u32 dataSize, u32 hash, u32 i) {
	return i < dataSize ?
		lsk_const_hash32_fnv1a(data, dataSize, (hash ^ ((const char*)data)[i]) * FNV_PRIME, i+1):
		hash;
}

#define lsk_const_strHash32_fnv1a(str)\
	lsk_const_hash32_fnv1a(str, lsk_const_strLen(str), FNV_OFFSET_BASIS, 0)

// TODO: move this out?
#include <chrono>
#include <intrin.h>

typedef std::chrono::time_point<std::chrono::high_resolution_clock> timept;
#define timeNow() std::chrono::high_resolution_clock::now()
#define timeDuration(delta) std::chrono::duration<f64>(delta).count()
#define timeDurSince(timePoint) timeDuration(timeNow() - timePoint)

inline u64 lsk_countCycles() {
	return __rdtsc();
}

u32 lsk_rand();
f64 lsk_randf();
