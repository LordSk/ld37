#pragma once
#include <math.h> // c math
#include "lsk_types.h"
#ifdef LSK_MATH_SIMD_IMPL
	#include <intrin.h>
#endif

typedef float f32;

#ifndef LSK_PI
	#define LSK_PI (3.14159265359)
	#define LSK_2PI (6.28318530718)
	#define LSK_HALFPI (1.57079632679)
#endif

inline f32 lsk_radians(f32 deg)
{
	return deg * LSK_PI / 180.f;
}

inline f32 lsk_wrapAngle(f32 angle)
{
	if(angle > LSK_2PI) {
		angle -= (i32)(angle / LSK_2PI) * LSK_2PI;
	}
	else if(angle < LSK_2PI) {
		angle += (i32)(-angle / LSK_2PI) * LSK_2PI;
	}
	return angle;
}

// TODO: inverse a1 and a2?
inline f32 lsk_lerpAngle(f32 a1, f32 a2, f32 alpha)
{
	f32 angle1 = lsk_wrapAngle(a1);
	f32 angle2 = lsk_wrapAngle(a2);
	f32 angleDiff = angle1 - angle2;
	angleDiff = angleDiff < 0 ? -angleDiff : angleDiff; // abs

	if(angleDiff > LSK_PI) {
		if(angle2 > angle1) {
			angle1 += LSK_2PI;
		}
		else{
			angle2 += LSK_2PI;
		}
	}

	return angle1 * alpha + angle2 * (1.f - alpha);
}

//
// Vector 2 type
//
union alignas(16) lsk_Vec2
{
	struct { f32 x, y, unused[2]; };
	f32 data[4];
#ifdef LSK_MATH_SIMD_IMPL
	__m128 wide;
#endif

	lsk_Vec2() = default;

	constexpr lsk_Vec2(f32 x_, f32 y_)
	: data{x_, y_, 0, 0} {
	}

	constexpr explicit lsk_Vec2(f32 one)
	: data{one, one, 0, 0} {
	}
};

inline lsk_Vec2 lsk_Vec2Minus(const lsk_Vec2& v)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec2 r;
	r.wide = _mm_sub_ps(_mm_set1_ps(0.f), v.wide);
	return r;
#else
	return lsk_Vec2{
		-v.x,
		-v.y
	};
#endif
}

inline lsk_Vec2 lsk_Vec2Add(const lsk_Vec2& v1, const lsk_Vec2& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec2 r;
	r.wide = _mm_add_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec2{
		v1.x + v2.x,
		v1.y + v2.y
	};
#endif
}

inline lsk_Vec2* lsk_Vec2Add(lsk_Vec2* v, const lsk_Vec2& other)
{
	v->x += other.x;
	v->y += other.y;
	return v;
}

inline lsk_Vec2 lsk_Vec2Sub(const lsk_Vec2& v1, const lsk_Vec2& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec2 r;
	r.wide = _mm_sub_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec2{
		v1.x - v2.x,
		v1.y - v2.y
	};
#endif
}

inline lsk_Vec2* lsk_Vec2Sub(lsk_Vec2* v, const lsk_Vec2& other)
{
	v->x -= other.x;
	v->y -= other.y;
	return v;
}

inline lsk_Vec2 lsk_Vec2Mul(const lsk_Vec2& v1, const lsk_Vec2& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec2 r;
	r.wide = _mm_mul_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec2{
		v1.x * v2.x,
		v1.y * v2.y
	};
#endif
}

inline lsk_Vec2* lsk_Vec2Mul(lsk_Vec2* v, const lsk_Vec2& other)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, other.wide);
#else
	v->x *= other.x;
	v->y *= other.y;
#endif
	return v;
}

inline lsk_Vec2 lsk_Vec2Div(const lsk_Vec2& v1, const lsk_Vec2& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec2 r;
	r.wide = _mm_div_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec2{
		v1.x / v2.x,
		v1.y / v2.y
	};
#endif
}

inline lsk_Vec2* lsk_Vec2Div(lsk_Vec2* v, const lsk_Vec2& other)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, other.wide);
#else
	v->x /= other.x;
	v->y /= other.y;
#endif
	return v;
}

// scalar
inline lsk_Vec2 lsk_Vec2Add(const lsk_Vec2& v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec2 r;
	r.wide = _mm_add_ps(v.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec2{
		v.x + scalar,
		v.y + scalar
	};
#endif
}

inline lsk_Vec2* lsk_Vec2Add(lsk_Vec2* v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_add_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x += scalar;
	v->y += scalar;
#endif
	return v;
}


inline lsk_Vec2 lsk_Vec2Sub(const lsk_Vec2& v1, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec2 r;
	r.wide = _mm_sub_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec2{
		v1.x - scalar,
		v1.y - scalar
	};
#endif
}

inline lsk_Vec2* lsk_Vec2Sub(lsk_Vec2* v, f32 scalar)
{
	v->x -= scalar;
	v->y -= scalar;
	return v;
}

inline lsk_Vec2 lsk_Vec2Mul(const lsk_Vec2& v1, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec2 r;
	r.wide = _mm_mul_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec2{
		v1.x * scalar,
		v1.y * scalar
	};
#endif
}

inline lsk_Vec2* lsk_Vec2Mul(lsk_Vec2* v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x *= scalar;
	v->y *= scalar;
#endif
	return v;
}

inline lsk_Vec2 lsk_Vec2Div(const lsk_Vec2& v1, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec2 r;
	r.wide = _mm_div_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec2{
		v1.x / scalar,
		v1.y / scalar
	};
#endif
}

inline lsk_Vec2* lsk_Vec2Div(lsk_Vec2* v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x /= scalar;
	v->y /= scalar;
#endif
	return v;
}

inline f32 lsk_dot(const lsk_Vec2& v1, const lsk_Vec2& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v1.wide, v2.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	return ((f32*)&r)[0];
#else
	return v1.x*v2.x + v1.y*v2.y;
#endif
}

inline f32 lsk_lengthSq(const lsk_Vec2& v)
{
	return lsk_dot(v , v);
}

inline f32 lsk_length(const lsk_Vec2& v)
{
#ifdef LSK_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v.wide, v.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	r = _mm_sqrt_ps(r);
	return ((f32*)&r)[0];
#else
	return sqrtf(lsk_lengthSq(v));
#endif
}

inline f32 lsk_distance(const lsk_Vec2& v1, const lsk_Vec2& v2)
{
	return lsk_length(lsk_Vec2Sub(v1, v2));
}

inline lsk_Vec2 lsk_normalize(lsk_Vec2 v)
{
#ifdef LSK_MATH_SIMD_IMPL
	__m128 len = _mm_mul_ps(v.wide, v.wide);
	len = _mm_hadd_ps(len, len);
	len = _mm_hadd_ps(len, len);
	len = _mm_sqrt_ps(len);
	v.wide = _mm_div_ps(v.wide, len);
	return v;
#else
	f32 scale = 1.0f / lsk_length(v);
	return lsk_Vec2Mul(v, scale);
#endif
}

#ifdef LSK_MATH_OPERATORS
inline lsk_Vec2 operator-(const lsk_Vec2& v) {
	return lsk_Vec2Minus(v);
}

inline lsk_Vec2 operator+(const lsk_Vec2& v1, const lsk_Vec2& v2) {
	return lsk_Vec2Add(v1, v2);
}

inline lsk_Vec2& operator+=(lsk_Vec2& v, const lsk_Vec2& other) {
	return *lsk_Vec2Add(&v, other);
}

inline lsk_Vec2 operator-(const lsk_Vec2& v1, const lsk_Vec2& v2) {
	return lsk_Vec2Sub(v1, v2);
}

inline lsk_Vec2& operator-=(lsk_Vec2& v, const lsk_Vec2& other) {
	return *lsk_Vec2Sub(&v, other);
}

inline lsk_Vec2 operator*(const lsk_Vec2& v1, const lsk_Vec2& v2) {
	return lsk_Vec2Mul(v1, v2);
}

inline lsk_Vec2& operator*=(lsk_Vec2& v, const lsk_Vec2& other) {
	return *lsk_Vec2Mul(&v, other);
}

inline lsk_Vec2 operator/(const lsk_Vec2& v1, const lsk_Vec2& v2) {
	return lsk_Vec2Div(v1, v2);
}

inline lsk_Vec2& operator/=(lsk_Vec2& v, const lsk_Vec2& other) {
	return *lsk_Vec2Div(&v, other);
}

// scalar
inline lsk_Vec2 operator+(const lsk_Vec2& v, f32 scalar) {
	return lsk_Vec2Add(v, scalar);
}

inline lsk_Vec2& operator+=(lsk_Vec2& v, f32 scalar) {
	return *lsk_Vec2Add(&v, scalar);
}

inline lsk_Vec2 operator-(const lsk_Vec2& v1, f32 scalar) {
	return lsk_Vec2Sub(v1, scalar);
}

inline lsk_Vec2& operator-=(lsk_Vec2& v, f32 scalar) {
	return *lsk_Vec2Sub(&v, scalar);
}

inline lsk_Vec2 operator*(const lsk_Vec2& v1, f32 scalar) {
	return lsk_Vec2Mul(v1, scalar);
}

inline lsk_Vec2& operator*=(lsk_Vec2& v, f32 scalar) {
	return *lsk_Vec2Mul(&v, scalar);
}

inline lsk_Vec2 operator/(const lsk_Vec2& v1, f32 scalar) {
	return lsk_Vec2Div(v1, scalar);
}

inline lsk_Vec2& operator/=(lsk_Vec2& v, f32 scalar) {
	return *lsk_Vec2Div(&v, scalar);
}
#endif


//
// Vector 3
//
union alignas(16) lsk_Vec3
{
	struct { f32 x, y, z, unused; };
	f32 data[4];
#ifdef LSK_MATH_SIMD_IMPL
	__m128 wide;
#endif

	lsk_Vec3() = default;

	constexpr lsk_Vec3(f32 x_, f32 y_, f32 z_)
	: data{x_, y_, z_, 0} {
	}

	constexpr explicit lsk_Vec3(f32 one)
	: data{one, one, one, 0} {
	}

	constexpr lsk_Vec3(const lsk_Vec2& v2, f32 z_)
	: data{v2.x, v2.y, z_, 0} {
	}
};

inline lsk_Vec3 lsk_Vec3Minus(const lsk_Vec3& v)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec3 r;
	r.wide = _mm_sub_ps(_mm_set1_ps(0.f), v.wide);
	return r;
#else
	return lsk_Vec3{
		-v.x,
		-v.y,
		-v.z
	};
#endif
}

inline lsk_Vec3 lsk_Vec3Add(const lsk_Vec3& v1, const lsk_Vec3& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec3 r;
	r.wide = _mm_add_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec3{
		v1.x + v2.x,
		v1.y + v2.y,
		v1.z + v2.z
	};
#endif
}

inline lsk_Vec3* lsk_Vec3Add(lsk_Vec3* v, const lsk_Vec3& other)
{
	v->x += other.x;
	v->y += other.y;
	v->z += other.z;
	return v;
}

inline lsk_Vec3 lsk_Vec3Sub(const lsk_Vec3& v1, const lsk_Vec3& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec3 r;
	r.wide = _mm_sub_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec3{
		v1.x - v2.x,
		v1.y - v2.y,
		v1.z - v2.z
	};
#endif
}

inline lsk_Vec3* lsk_Vec3Sub(lsk_Vec3* v, const lsk_Vec3& other)
{
	v->x -= other.x;
	v->y -= other.y;
	v->z -= other.z;
	return v;
}

inline lsk_Vec3 lsk_Vec3Mul(const lsk_Vec3& v1, const lsk_Vec3& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec3 r;
	r.wide = _mm_mul_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec3{
		v1.x * v2.x,
		v1.y * v2.y,
		v1.z * v2.z
	};
#endif
}

inline lsk_Vec3* lsk_Vec3Mul(lsk_Vec3* v, const lsk_Vec3& other)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, other.wide);
#else
	v->x *= other.x;
	v->y *= other.y;
	v->z *= other.z;
#endif
	return v;
}

inline lsk_Vec3 lsk_Vec3Div(const lsk_Vec3& v1, const lsk_Vec3& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec3 r;
	r.wide = _mm_div_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec3{
		v1.x / v2.x,
		v1.y / v2.y,
		v1.z / v2.z
	};
#endif
}

inline lsk_Vec3* lsk_Vec3Div(lsk_Vec3* v, const lsk_Vec3& other)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, other.wide);
#else
	v->x /= other.x;
	v->y /= other.y;
	v->z /= other.z;
#endif
	return v;
}

// scalar
inline lsk_Vec3 lsk_Vec3Add(const lsk_Vec3& v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec3 r;
	r.wide = _mm_add_ps(v.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec3{
		v.x + scalar,
		v.y + scalar,
		v.z + scalar
	};
#endif
}

inline lsk_Vec3* lsk_Vec3Add(lsk_Vec3* v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_add_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x += scalar;
	v->y += scalar;
	v->z += scalar;
#endif
	return v;
}


inline lsk_Vec3 lsk_Vec3Sub(const lsk_Vec3& v1, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec3 r;
	r.wide = _mm_sub_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec3{
		v1.x - scalar,
		v1.y - scalar,
		v1.z - scalar
	};
#endif
}

inline lsk_Vec3* lsk_Vec3Sub(lsk_Vec3* v, f32 scalar)
{
	v->x -= scalar;
	v->y -= scalar;
	v->z -= scalar;
	return v;
}

inline lsk_Vec3 lsk_Vec3Mul(const lsk_Vec3& v1, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec3 r;
	r.wide = _mm_mul_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec3{
		v1.x * scalar,
		v1.y * scalar,
		v1.z * scalar
	};
#endif
}

inline lsk_Vec3* lsk_Vec3Mul(lsk_Vec3* v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x *= scalar;
	v->y *= scalar;
	v->z *= scalar;
#endif
	return v;
}

inline lsk_Vec3 lsk_Vec3Div(const lsk_Vec3& v1, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec3 r;
	r.wide = _mm_div_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec3{
		v1.x / scalar,
		v1.y / scalar,
		v1.z / scalar
	};
#endif
}

inline lsk_Vec3* lsk_Vec3Div(lsk_Vec3* v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x /= scalar;
	v->y /= scalar;
	v->z /= scalar;
#endif
	return v;
}

inline lsk_Vec3 lsk_cross(const lsk_Vec3& v1, const lsk_Vec3& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	__m128 v1s0 = _mm_shuffle_ps(v1.wide, v1.wide, _MM_SHUFFLE(0, 0, 2, 1));
	__m128 v2s0 = _mm_shuffle_ps(v2.wide, v2.wide, _MM_SHUFFLE(0, 1, 0, 2));
	__m128 v1s1 = _mm_shuffle_ps(v1.wide, v1.wide, _MM_SHUFFLE(0, 1, 0, 2));
	__m128 v2s1 = _mm_shuffle_ps(v2.wide, v2.wide, _MM_SHUFFLE(0, 0, 2, 1));
	lsk_Vec3 r;
	r.wide = _mm_sub_ps(_mm_mul_ps(v1s0, v2s0), _mm_mul_ps(v1s1, v2s1));
	return r;
#else
	return lsk_Vec3{
		(v1.y * v2.z) - (v1.z * v2.y),
		(v1.z * v2.x) - (v1.x * v2.z),
		(v1.x * v2.y) - (v1.y * v2.x)
	};
#endif
}

inline f32 lsk_dot(const lsk_Vec3& v1, const lsk_Vec3& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v1.wide, v2.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	return ((f32*)&r)[0];
#else
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
#endif
}

inline f32 lsk_lengthSq(const lsk_Vec3& v)
{
	return lsk_dot(v , v);
}

inline f32 lsk_length(const lsk_Vec3& v)
{
#ifdef LSK_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v.wide, v.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	r = _mm_sqrt_ps(r);
	return ((f32*)&r)[0];
#else
	return sqrtf(lsk_lengthSq(v));
#endif
}

inline f32 lsk_distance(const lsk_Vec3& v1, const lsk_Vec3& v2)
{
	return lsk_length(lsk_Vec3Sub(v1, v2));
}

inline lsk_Vec3 lsk_normalize(lsk_Vec3 v)
{
#ifdef LSK_MATH_SIMD_IMPL
	__m128 len = _mm_mul_ps(v.wide, v.wide);
	len = _mm_hadd_ps(len, len);
	len = _mm_hadd_ps(len, len);
	len = _mm_sqrt_ps(len);
	v.wide = _mm_div_ps(v.wide, len);
	return v;
#else
	f32 scale = 1.0f / lsk_length(v);
	return lsk_Vec3Mul(v, scale);
#endif
}

#ifdef LSK_MATH_OPERATORS
inline lsk_Vec3 operator-(const lsk_Vec3& v) {
	return lsk_Vec3Minus(v);
}

inline lsk_Vec3 operator+(const lsk_Vec3& v1, const lsk_Vec3& v2) {
	return lsk_Vec3Add(v1, v2);
}

inline lsk_Vec3& operator+=(lsk_Vec3& v, const lsk_Vec3& other) {
	return *lsk_Vec3Add(&v, other);
}

inline lsk_Vec3 operator-(const lsk_Vec3& v1, const lsk_Vec3& v2) {
	return lsk_Vec3Sub(v1, v2);
}

inline lsk_Vec3& operator-=(lsk_Vec3& v, const lsk_Vec3& other) {
	return *lsk_Vec3Sub(&v, other);
}

inline lsk_Vec3 operator*(const lsk_Vec3& v1, const lsk_Vec3& v2) {
	return lsk_Vec3Mul(v1, v2);
}

inline lsk_Vec3& operator*=(lsk_Vec3& v, const lsk_Vec3& other) {
	return *lsk_Vec3Mul(&v, other);
}

inline lsk_Vec3 operator/(const lsk_Vec3& v1, const lsk_Vec3& v2) {
	return lsk_Vec3Div(v1, v2);
}

inline lsk_Vec3& operator/=(lsk_Vec3& v, const lsk_Vec3& other) {
	return *lsk_Vec3Div(&v, other);
}

// scalar
inline lsk_Vec3 operator+(const lsk_Vec3& v, f32 scalar) {
	return lsk_Vec3Add(v, scalar);
}

inline lsk_Vec3& operator+=(lsk_Vec3& v, f32 scalar) {
	return *lsk_Vec3Add(&v, scalar);
}

inline lsk_Vec3 operator-(const lsk_Vec3& v1, f32 scalar) {
	return lsk_Vec3Sub(v1, scalar);
}

inline lsk_Vec3& operator-=(lsk_Vec3& v, f32 scalar) {
	return *lsk_Vec3Sub(&v, scalar);
}

inline lsk_Vec3 operator*(const lsk_Vec3& v1, f32 scalar) {
	return lsk_Vec3Mul(v1, scalar);
}

inline lsk_Vec3& operator*=(lsk_Vec3& v, f32 scalar) {
	return *lsk_Vec3Mul(&v, scalar);
}

inline lsk_Vec3 operator/(const lsk_Vec3& v1, f32 scalar) {
	return lsk_Vec3Div(v1, scalar);
}

inline lsk_Vec3& operator/=(lsk_Vec3& v, f32 scalar) {
	return *lsk_Vec3Div(&v, scalar);
}
#endif



//
// Vector 4
//
union alignas(16) lsk_Vec4
{
	struct { f32 x, y, z, w; };
	struct { f32 r, g, b, a; };
	f32 data[4];
#ifdef LSK_MATH_SIMD_IMPL
	__m128 wide;
#endif

	lsk_Vec4() = default;

	constexpr lsk_Vec4(f32 x_, f32 y_, f32 z_, f32 w_)
	: data{x_, y_, z_, w_} {
	}

	constexpr explicit lsk_Vec4(f32 one)
	: data{one, one, one, one} {
	}
};

inline lsk_Vec4 lsk_Vec4Minus(const lsk_Vec4& v)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec4 r;
	r.wide = _mm_sub_ps(_mm_set1_ps(0.f), v.wide);
	return r;
#else
	return lsk_Vec4{
		-v.x,
		-v.y,
		-v.z,
		-v.w
	};
#endif
}

inline lsk_Vec4 lsk_Vec4Add(const lsk_Vec4& v1, const lsk_Vec4& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec4 r;
	r.wide = _mm_add_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec4{
		v1.x + v2.x,
		v1.y + v2.y,
		v1.z + v2.z,
		v1.w + v2.w
	};
#endif
}

inline lsk_Vec4* lsk_Vec4Add(lsk_Vec4* v, const lsk_Vec4& other)
{
	v->x += other.x;
	v->y += other.y;
	v->z += other.z;
	v->w += other.w;
	return v;
}

inline lsk_Vec4 lsk_Vec4Sub(const lsk_Vec4& v1, const lsk_Vec4& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec4 r;
	r.wide = _mm_sub_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec4{
		v1.x - v2.x,
		v1.y - v2.y,
		v1.z - v2.z,
		v1.w - v2.w
	};
#endif
}

inline lsk_Vec4* lsk_Vec4Sub(lsk_Vec4* v, const lsk_Vec4& other)
{
	v->x -= other.x;
	v->y -= other.y;
	v->z -= other.z;
	v->w -= other.w;
	return v;
}

inline lsk_Vec4 lsk_Vec4Mul(const lsk_Vec4& v1, const lsk_Vec4& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec4 r;
	r.wide = _mm_mul_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec4{
		v1.x * v2.x,
		v1.y * v2.y,
		v1.z * v2.z,
		v1.w * v2.w
	};
#endif
}

inline lsk_Vec4* lsk_Vec4Mul(lsk_Vec4* v, const lsk_Vec4& other)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, other.wide);
#else
	v->x *= other.x;
	v->y *= other.y;
	v->z *= other.z;
	v->w *= other.w;
#endif
	return v;
}

inline lsk_Vec4 lsk_Vec4Div(const lsk_Vec4& v1, const lsk_Vec4& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec4 r;
	r.wide = _mm_div_ps(v1.wide, v2.wide);
	return r;
#else
	return lsk_Vec4{
		v1.x / v2.x,
		v1.y / v2.y,
		v1.z / v2.z,
		v1.w / v2.w
	};
#endif
}

inline lsk_Vec4* lsk_Vec4Div(lsk_Vec4* v, const lsk_Vec4& other)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, other.wide);
#else
	v->x /= other.x;
	v->y /= other.y;
	v->z /= other.z;
	v->w /= other.w;
#endif
	return v;
}

// scalar
inline lsk_Vec4 lsk_Vec4Add(const lsk_Vec4& v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec4 r;
	r.wide = _mm_add_ps(v.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec4{
		v.x + scalar,
		v.y + scalar,
		v.z + scalar,
		v.w + scalar
	};
#endif
}

inline lsk_Vec4* lsk_Vec4Add(lsk_Vec4* v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_add_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x += scalar;
	v->y += scalar;
	v->z += scalar;
	v->w += scalar;
#endif
	return v;
}


inline lsk_Vec4 lsk_Vec4Sub(const lsk_Vec4& v1, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec4 r;
	r.wide = _mm_sub_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec4{
		v1.x - scalar,
		v1.y - scalar,
		v1.z - scalar,
		v1.w - scalar
	};
#endif
}

inline lsk_Vec4* lsk_Vec4Sub(lsk_Vec4* v, f32 scalar)
{
	v->x -= scalar;
	v->y -= scalar;
	v->z -= scalar;
	v->w -= scalar;
	return v;
}

inline lsk_Vec4 lsk_Vec4Mul(const lsk_Vec4& v1, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec4 r;
	r.wide = _mm_mul_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec4{
		v1.x * scalar,
		v1.y * scalar,
		v1.z * scalar,
		v1.w * scalar
	};
#endif
}

inline lsk_Vec4* lsk_Vec4Mul(lsk_Vec4* v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_mul_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x *= scalar;
	v->y *= scalar;
	v->z *= scalar;
	v->w *= scalar;
#endif
	return v;
}

inline lsk_Vec4 lsk_Vec4Div(const lsk_Vec4& v1, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Vec4 r;
	r.wide = _mm_div_ps(v1.wide, _mm_set1_ps(scalar));
	return r;
#else
	return lsk_Vec4{
		v1.x / scalar,
		v1.y / scalar,
		v1.z / scalar,
		v1.w / scalar
	};
#endif
}

inline lsk_Vec4* lsk_Vec4Div(lsk_Vec4* v, f32 scalar)
{
#ifdef LSK_MATH_SIMD_IMPL
	v->wide = _mm_div_ps(v->wide, _mm_set1_ps(scalar));
#else
	v->x /= scalar;
	v->y /= scalar;
	v->z /= scalar;
	v->w /= scalar;
#endif
	return v;
}

inline f32 lsk_dot(const lsk_Vec4& v1, const lsk_Vec4& v2)
{
#ifdef LSK_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v1.wide, v2.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	return ((f32*)&r)[0];
#else
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
#endif
}

inline f32 lsk_lengthSq(const lsk_Vec4& v)
{
	return lsk_dot(v , v);
}

inline f32 lsk_length(const lsk_Vec4& v)
{
#ifdef LSK_MATH_SIMD_IMPL
	__m128 r = _mm_mul_ps(v.wide, v.wide);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	r = _mm_sqrt_ps(r);
	return ((f32*)&r)[0];
#else
	return sqrtf(lsk_lengthSq(v));
#endif
}

inline lsk_Vec4 lsk_normalize(lsk_Vec4 v)
{
#ifdef LSK_MATH_SIMD_IMPL
	__m128 len = _mm_mul_ps(v.wide, v.wide);
	len = _mm_hadd_ps(len, len);
	len = _mm_hadd_ps(len, len);
	len = _mm_sqrt_ps(len);
	v.wide = _mm_div_ps(v.wide, len);
	return v;
#else
	f32 scale = 1.0f / lsk_length(v);
	return lsk_Vec4Mul(v, scale);
#endif
}

#ifdef LSK_MATH_OPERATORS
inline lsk_Vec4 operator-(const lsk_Vec4& v) {
	return lsk_Vec4Minus(v);
}

inline lsk_Vec4 operator+(const lsk_Vec4& v1, const lsk_Vec4& v2) {
	return lsk_Vec4Add(v1, v2);
}

inline lsk_Vec4& operator+=(lsk_Vec4& v, const lsk_Vec4& other) {
	return *lsk_Vec4Add(&v, other);
}

inline lsk_Vec4 operator-(const lsk_Vec4& v1, const lsk_Vec4& v2) {
	return lsk_Vec4Sub(v1, v2);
}

inline lsk_Vec4& operator-=(lsk_Vec4& v, const lsk_Vec4& other) {
	return *lsk_Vec4Sub(&v, other);
}

inline lsk_Vec4 operator*(const lsk_Vec4& v1, const lsk_Vec4& v2) {
	return lsk_Vec4Mul(v1, v2);
}

inline lsk_Vec4& operator*=(lsk_Vec4& v, const lsk_Vec4& other) {
	return *lsk_Vec4Mul(&v, other);
}

inline lsk_Vec4 operator/(const lsk_Vec4& v1, const lsk_Vec4& v2) {
	return lsk_Vec4Div(v1, v2);
}

inline lsk_Vec4& operator/=(lsk_Vec4& v, const lsk_Vec4& other) {
	return *lsk_Vec4Div(&v, other);
}

// scalar
inline lsk_Vec4 operator+(const lsk_Vec4& v, f32 scalar) {
	return lsk_Vec4Add(v, scalar);
}

inline lsk_Vec4& operator+=(lsk_Vec4& v, f32 scalar) {
	return *lsk_Vec4Add(&v, scalar);
}

inline lsk_Vec4 operator-(const lsk_Vec4& v1, f32 scalar) {
	return lsk_Vec4Sub(v1, scalar);
}

inline lsk_Vec4& operator-=(lsk_Vec4& v, f32 scalar) {
	return *lsk_Vec4Sub(&v, scalar);
}

inline lsk_Vec4 operator*(const lsk_Vec4& v1, f32 scalar) {
	return lsk_Vec4Mul(v1, scalar);
}

inline lsk_Vec4& operator*=(lsk_Vec4& v, f32 scalar) {
	return *lsk_Vec4Mul(&v, scalar);
}

inline lsk_Vec4 operator/(const lsk_Vec4& v1, f32 scalar) {
	return lsk_Vec4Div(v1, scalar);
}

inline lsk_Vec4& operator/=(lsk_Vec4& v, f32 scalar) {
	return *lsk_Vec4Div(&v, scalar);
}
#endif

// matrix

//
// Matrix 4x4
//
union alignas(16) lsk_Mat4
{
	f32 data[16];
#ifdef LSK_MATH_SIMD_IMPL
	__m128 wide[4];
#endif
};

inline lsk_Mat4 lsk_Mat4Identity()
{
	return lsk_Mat4{
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

inline lsk_Mat4 lsk_Mat4Mul(const lsk_Mat4& m1, const lsk_Mat4& m2)
{
#ifdef LSK_MATH_SIMD_IMPL
	lsk_Mat4 m;
	m.wide[0] = _mm_add_ps(
	_mm_add_ps(_mm_mul_ps(m1.wide[0], _mm_set1_ps(m2.data[0])),
	_mm_mul_ps(m1.wide[1], _mm_set1_ps(m2.data[1]))),
	_mm_add_ps(_mm_mul_ps(m1.wide[2], _mm_set1_ps(m2.data[2])),
	_mm_mul_ps(m1.wide[3], _mm_set1_ps(m2.data[3])))
	);
	m.wide[1] = _mm_add_ps(
	_mm_add_ps(_mm_mul_ps(m1.wide[0], _mm_set1_ps(m2.data[4])),
	_mm_mul_ps(m1.wide[1], _mm_set1_ps(m2.data[5]))),
	_mm_add_ps(_mm_mul_ps(m1.wide[2], _mm_set1_ps(m2.data[6])),
	_mm_mul_ps(m1.wide[3], _mm_set1_ps(m2.data[7])))
	);
	m.wide[2] = _mm_add_ps(
	_mm_add_ps(_mm_mul_ps(m1.wide[0], _mm_set1_ps(m2.data[8])),
	_mm_mul_ps(m1.wide[1], _mm_set1_ps(m2.data[9]))),
	_mm_add_ps(_mm_mul_ps(m1.wide[2], _mm_set1_ps(m2.data[10])),
	_mm_mul_ps(m1.wide[3], _mm_set1_ps(m2.data[11])))
	);
	m.wide[3] = _mm_add_ps(
	_mm_add_ps(_mm_mul_ps(m1.wide[0], _mm_set1_ps(m2.data[12])),
	_mm_mul_ps(m1.wide[1], _mm_set1_ps(m2.data[13]))),
	_mm_add_ps(_mm_mul_ps(m1.wide[2], _mm_set1_ps(m2.data[14])),
	_mm_mul_ps(m1.wide[3], _mm_set1_ps(m2.data[15])))
	);
	return m;
#else
	lsk_Mat4 m;
	f32* md = m.data;
	const f32* md1 = m1.data;
	const f32* md2 = m2.data;

	md[0] = md1[0] * md2[0] + md1[4] * md2[1] + md1[8] * md2[2] + md1[12] * md2[3];
	md[1] = md1[1] * md2[0] + md1[5] * md2[1] + md1[9] * md2[2] + md1[13] * md2[3];
	md[2] = md1[2] * md2[0] + md1[6] * md2[1] + md1[10] * md2[2] + md1[14] * md2[3];
	md[3] = md1[3] * md2[0] + md1[7] * md2[1] + md1[11] * md2[2] + md1[15] * md2[3];

	md[4] = md1[0] * md2[4] + md1[4] * md2[5] + md1[8] * md2[6] + md1[12] * md2[7];
	md[5] = md1[1] * md2[4] + md1[5] * md2[5] + md1[9] * md2[6] + md1[13] * md2[7];
	md[6] = md1[2] * md2[4] + md1[6] * md2[5] + md1[10] * md2[6] + md1[14] * md2[7];
	md[7] = md1[3] * md2[4] + md1[7] * md2[5] + md1[11] * md2[6] + md1[15] * md2[7];

	md[8] = md1[0] * md2[8] + md1[4] * md2[9] + md1[8] * md2[10] + md1[12] * md2[11];
	md[9] = md1[1] * md2[8] + md1[5] * md2[9] + md1[9] * md2[10] + md1[13] * md2[11];
	md[10] = md1[2] * md2[8] + md1[6] * md2[9] + md1[10] * md2[10] + md1[14] * md2[11];
	md[11] = md1[3] * md2[8] + md1[7] * md2[9] + md1[11] * md2[10] + md1[15] * md2[11];

	md[12] = md1[0] * md2[12] + md1[4] * md2[13] + md1[8] * md2[14] + md1[12] * md2[15];
	md[13] = md1[1] * md2[12] + md1[5] * md2[13] + md1[9] * md2[14] + md1[13] * md2[15];
	md[14] = md1[2] * md2[12] + md1[6] * md2[13] + md1[10] * md2[14] + md1[14] * md2[15];
	md[15] = md1[3] * md2[12] + md1[7] * md2[13] + md1[11] * md2[14] + md1[15] * md2[15];

	return m;
#endif
}

inline lsk_Mat4 lsk_Mat4Inv(lsk_Mat4 mat) {
	lsk_Mat4 tmp;
	f32* td = tmp.data;
	const f32* md = mat.data;

	td[0] = md[5]  * md[10] * md[15] -
			md[5]  * md[11] * md[14] -
			md[9]  * md[6]  * md[15] +
			md[9]  * md[7]  * md[14] +
			md[13] * md[6]  * md[11] -
			md[13] * md[7]  * md[10];

	td[4] = -md[4]  * md[10] * md[15] +
			md[4]  * md[11] * md[14] +
			md[8]  * md[6]  * md[15] -
			md[8]  * md[7]  * md[14] -
			md[12] * md[6]  * md[11] +
			md[12] * md[7]  * md[10];

	td[8] = md[4]  * md[9] * md[15] -
			md[4]  * md[11] * md[13] -
			md[8]  * md[5] * md[15] +
			md[8]  * md[7] * md[13] +
			md[12] * md[5] * md[11] -
			md[12] * md[7] * md[9];

	td[12] = -md[4]  * md[9] * md[14] +
			md[4]  * md[10] * md[13] +
			md[8]  * md[5] * md[14] -
			md[8]  * md[6] * md[13] -
			md[12] * md[5] * md[10] +
			md[12] * md[6] * md[9];

	td[1] = -md[1]  * md[10] * md[15] +
			md[1]  * md[11] * md[14] +
			md[9]  * md[2] * md[15] -
			md[9]  * md[3] * md[14] -
			md[13] * md[2] * md[11] +
			md[13] * md[3] * md[10];

	td[5] = md[0]  * md[10] * md[15] -
			md[0]  * md[11] * md[14] -
			md[8]  * md[2] * md[15] +
			md[8]  * md[3] * md[14] +
			md[12] * md[2] * md[11] -
			md[12] * md[3] * md[10];

	td[9] = -md[0]  * md[9] * md[15] +
			md[0]  * md[11] * md[13] +
			md[8]  * md[1] * md[15] -
			md[8]  * md[3] * md[13] -
			md[12] * md[1] * md[11] +
			md[12] * md[3] * md[9];

	td[13] = md[0]  * md[9] * md[14] -
			md[0]  * md[10] * md[13] -
			md[8]  * md[1] * md[14] +
			md[8]  * md[2] * md[13] +
			md[12] * md[1] * md[10] -
			md[12] * md[2] * md[9];

	td[2] = md[1]  * md[6] * md[15] -
			md[1]  * md[7] * md[14] -
			md[5]  * md[2] * md[15] +
			md[5]  * md[3] * md[14] +
			md[13] * md[2] * md[7] -
			md[13] * md[3] * md[6];

	td[6] = -md[0]  * md[6] * md[15] +
			md[0]  * md[7] * md[14] +
			md[4]  * md[2] * md[15] -
			md[4]  * md[3] * md[14] -
			md[12] * md[2] * md[7] +
			md[12] * md[3] * md[6];

	td[10] = md[0]  * md[5] * md[15] -
			md[0]  * md[7] * md[13] -
			md[4]  * md[1] * md[15] +
			md[4]  * md[3] * md[13] +
			md[12] * md[1] * md[7] -
			md[12] * md[3] * md[5];

	td[14] = -md[0]  * md[5] * md[14] +
			md[0]  * md[6] * md[13] +
			md[4]  * md[1] * md[14] -
			md[4]  * md[2] * md[13] -
			md[12] * md[1] * md[6] +
			md[12] * md[2] * md[5];

	td[3] = -md[1] * md[6] * md[11] +
			md[1] * md[7] * md[10] +
			md[5] * md[2] * md[11] -
			md[5] * md[3] * md[10] -
			md[9] * md[2] * md[7] +
			md[9] * md[3] * md[6];

	td[7] = md[0] * md[6] * md[11] -
			md[0] * md[7] * md[10] -
			md[4] * md[2] * md[11] +
			md[4] * md[3] * md[10] +
			md[8] * md[2] * md[7] -
			md[8] * md[3] * md[6];

	td[11] = -md[0] * md[5] * md[11] +
			md[0] * md[7] * md[9] +
			md[4] * md[1] * md[11] -
			md[4] * md[3] * md[9] -
			md[8] * md[1] * md[7] +
			md[8] * md[3] * md[5];

	td[15] = md[0] * md[5] * md[10] -
			md[0] * md[6] * md[9] -
			md[4] * md[1] * md[10] +
			md[4] * md[2] * md[9] +
			md[8] * md[1] * md[6] -
			md[8] * md[2] * md[5];

	f32 det = md[0] * td[0] + md[1] * td[4] + md[2] * td[8] +
			md[3] * td[12];

	if(det == 0) {
		return lsk_Mat4Identity();
	}

	det = 1.f / det;

	lsk_Mat4 out;
	for(u32 i = 0; i < 16; ++i) {
		out.data[i] = td[i] * det;
	}

	return out;
}

inline lsk_Vec3 lsk_Vec3MulMat4(lsk_Vec3 vec, lsk_Mat4 mat) {
	lsk_Vec3 o;
	f32* md = mat.data;
	o.x = vec.x * md[0] + vec.y * md[4] + vec.z * md[8] + md[12];
	o.y = vec.x * md[1] + vec.y * md[5] + vec.z * md[9] + md[13];
	o.z = vec.x * md[2] + vec.y * md[6] + vec.z * md[10] + md[14];
	return o;
}

inline lsk_Mat4 lsk_Mat4Translate(const lsk_Vec3& t)
{
	return lsk_Mat4{
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		t.x, t.y, t.z, 1.f
	};
}

inline lsk_Mat4 lsk_Mat4Scale(const lsk_Vec3& s)
{
	return lsk_Mat4{
		s.x, 0.f, 0.f, 0.f,
		0.f, s.y, 0.f, 0.f,
		0.f, 0.f, s.z, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

inline lsk_Mat4 lsk_Mat4RotateAxisX(f32 angle)
{
	return lsk_Mat4{
		1.f, 0.f, 0.f, 0.f,
		0.f, cosf(angle), -sinf(angle), 0.f,
		0.f, sinf(angle), cosf(angle), 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

inline lsk_Mat4 lsk_Mat4RotateAxisY(f32 angle)
{
	return lsk_Mat4{
		cosf(angle), 0.f, sinf(angle), 0.f,
		0.f, 1.f, 0.f, 0.f,
		-sinf(angle), 0.f, cosf(angle), 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

inline lsk_Mat4 lsk_Mat4RotateAxisZ(f32 angle)
{
	return lsk_Mat4{
		cosf(angle), -sinf(angle), 0.f, 0.f,
		sinf(angle), cosf(angle), 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

inline lsk_Mat4 lsk_Mat4Perspective(f32 fov, f32 aspect, f32 nearPlane, f32 farPlane)
{
	fov *= 0.5f;
	f32 d = farPlane - nearPlane;
	f32 cosFov = cosf(fov);
	f32 sinFov = sinf(fov);

	if(d == 0 || sinFov == 0 || aspect == 0) {
		return lsk_Mat4Identity();
	}

	f32 cotangent = cosFov / sinFov;

	lsk_Mat4 m = lsk_Mat4Identity();
	m.data[0] = cotangent / aspect;
	m.data[5] = cotangent;
	m.data[10] = -(farPlane + nearPlane) / d;
	m.data[11] = -1.f;
	m.data[14] = -2.f * nearPlane * farPlane / d;
	m.data[15] = 0.f;
	return m;
}

inline lsk_Mat4 lsk_Mat4Orthographic(f32 left, f32 right, f32 bottom, f32 top,
									 f32 nearPlane, f32 farPlane)
{
	lsk_Mat4 m = lsk_Mat4Identity();
	m.data[0] = 2.f / (right - left);
	m.data[5] = 2.f / (top - bottom);
	m.data[10] = -2.f / (farPlane - nearPlane);
	m.data[12] = -((right + left) / (right - left));
	m.data[13] = -((top + bottom) / (top - bottom));
	m.data[14] = -((farPlane + nearPlane) / (farPlane - nearPlane));
	return m;
}

inline lsk_Mat4 lsk_Mat4LookAt(const lsk_Vec3& eye, const lsk_Vec3& center, const lsk_Vec3& up)
{
	lsk_Vec3 dir = lsk_normalize(lsk_Vec3Sub(center, eye));
	lsk_Vec3 c0 = lsk_normalize(lsk_cross(dir, up));
	lsk_Vec3 c1 = lsk_normalize(lsk_cross(c0, dir));

	return lsk_Mat4{
		c0.x,
		c1.x,
		-dir.x,
		0.f,

		c0.y,
		c1.y,
		-dir.y,
		0.f,

		c0.z,
		c1.z,
		-dir.z,
		0.f,

		-lsk_dot(c0, eye),
		-lsk_dot(c1, eye),
		lsk_dot(dir, eye),
		1.f
	};
}

#ifdef LSK_MATH_OPERATORS
inline lsk_Mat4 operator*(const lsk_Mat4& m1, const lsk_Mat4& m2) {
	return lsk_Mat4Mul(m1, m2);
}

inline lsk_Vec3 operator*(const lsk_Mat4& m, const lsk_Vec3& v) {
	return lsk_Vec3MulMat4(v, m);
}
#endif

union alignas(16) lsk_Quat
{
	struct { f32 x, y, z, w; };
	f32 data[4];
#ifdef LSK_MATH_SIMD_IMPL
	__m128 wide;
#endif

	constexpr lsk_Quat(): data{0.f, 0.f, 0.f, 1.f}{}

	constexpr lsk_Quat(f32 x_, f32 y_, f32 z_, f32 w_)
	: data{x_, y_, z_, w_} {
	}
};

inline lsk_Quat lsk_normalize(lsk_Quat quat) {
	f32 lengthInv = 1.f / sqrtf(quat.x * quat.x + quat.y * quat.y + quat.z * quat.z + quat.w * quat.w);
	quat.x *= lengthInv;
	quat.y *= lengthInv;
	quat.z *= lengthInv;
	quat.w *= lengthInv;
	return quat;
}

inline bool lsk_QuatIsNull(const lsk_Quat& quat) {
	return (quat.x == 0 && quat.y == 0 && quat.z && quat.w == 1.f);
}

inline lsk_Quat lsk_QuatConjugate(lsk_Quat q) {
	return {-q.x, -q.y, -q.z, q.w};
}

inline lsk_Quat lsk_QuatAxisRotation(lsk_Vec3 axis, f32 angle) {
	lsk_Quat quat;
	f32 sa = sinf(angle * 0.5f);
	quat.x = axis.x * sa;
	quat.y = axis.y * sa;
	quat.z = axis.z * sa;
	quat.w = cosf(angle * 0.5f);
	quat = lsk_normalize(quat);
	return quat;
}

inline lsk_Quat lsk_QuatGetRotVec3(lsk_Vec3 v1, lsk_Vec3 v2) {
	lsk_Quat quat;
	lsk_Vec3 c = lsk_cross(v1, v2);
	quat.x = c.x;
	quat.y = c.y;
	quat.z = c.z;
	quat.w = sqrtf(lsk_lengthSq(v1) * lsk_lengthSq(v2)) + lsk_dot(v1, v2);
	return lsk_normalize(quat);
}

inline lsk_Mat4 lsk_QuatMatrix(const lsk_Quat& quat) {
	f32 xx = quat.x * quat.x;
	f32 xy = quat.x * quat.y;
	f32 xz = quat.x * quat.z;
	f32 xw = quat.x * quat.w;

	f32 yy = quat.y * quat.y;
	f32 yz = quat.y * quat.z;
	f32 yw = quat.y * quat.w;

	f32 zz = quat.z * quat.z;
	f32 zw = quat.z * quat.w;

	return lsk_Mat4{
		1.f - 2.f * (yy + zz),
		2.f * (xy + zw),
		2.f * (xz - yw),
		0,

		2.f * (xy - zw),
		1.f - 2.f * (xx + zz),
		2.f * (yz + xw),
		0,

		2.f * (xz + yw),
		2.f * (yz - xw),
		1.f - 2.f * (xx + yy),
		0,

		0,
		0,
		0,
		1,
	};
}

inline lsk_Quat lsk_QuatMul(lsk_Quat qa, lsk_Quat qb) {
	// Grassman product (equivalent methods)
#if 0
	lsk_Vec3 qav{qa.x, qa.y, qa.z};
	lsk_Vec3 qbv{qb.x, qb.y, qb.z};
	lsk_Vec3 qv = lsk_Vec3Add(lsk_Vec3Add(lsk_Vec3Mul(qbv, qa.w), lsk_Vec3Mul(qav, qb.w)),
							  lsk_cross(qav, qbv));
	return {qv.x, qv.y, qv.z, qa.w * qb.w - lsk_dot(qav, qbv)};
#endif
	lsk_Quat quat;
	quat.x = qa.w * qb.x + qa.x * qb.w + qa.y * qb.z - qa.z * qb.y;
	quat.y = qa.w * qb.y + qa.y * qb.w + qa.z * qb.x - qa.x * qb.z;
	quat.z = qa.w * qb.z + qa.z * qb.w + qa.x * qb.y - qa.y * qb.x;
	quat.w = qa.w * qb.w - qa.x * qb.x - qa.y * qb.y - qa.z * qb.z;
	return quat;
}

#ifdef LSK_MATH_OPERATORS
inline lsk_Quat operator*(const lsk_Quat& q1, const lsk_Quat& q2) {
	return lsk_QuatMul(q1, q2);
}

inline lsk_Quat& operator*=(lsk_Quat& q, const lsk_Quat& other) {
	return q = lsk_QuatMul(q, other);
}
#endif

inline lsk_Vec3 lsk_QuatRotateVec3(lsk_Vec3 vec, lsk_Quat quat) {
	lsk_Quat qvec = {vec.x, vec.y, vec.z, 0};
	qvec = lsk_QuatMul(lsk_QuatMul(quat, qvec), lsk_QuatConjugate(quat));
	return {qvec.x, qvec.y, qvec.z};
}


struct lsk_AABB2
{
	lsk_Vec2 min, max;
};

struct lsk_AABB3
{
	lsk_Vec3 min, max;
};

struct lsk_Sphere
{
	lsk_Vec3 center = {0, 0, 0};
	f32 radius = 0;
};
