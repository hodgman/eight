//------------------------------------------------------------------------------
#pragma once
#include <math.h>
#include <eight/core/align.h>
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T>
struct eiALIGN(16) TVec4
{
	T elements[4];
	      T& operator[]( uint i )       { return elements[i]; }
	const T& operator[]( uint i ) const { return elements[i]; }
	const TVec4 operator *( float s ) const
	{
		TVec4 result = {{ elements[0]*s, elements[1]*s, elements[2]*s, elements[3]*s }};
		return result;
	}
	const TVec4 operator *( const TVec4& vec ) const
	{
		Vec4 result = {{ elements[0]*vec[0], elements[1]*vec[1], elements[2]*vec[2], elements[3]*vec[3] }};
		return result;
	}
	const TVec4 operator +( const TVec4& vec ) const
	{
		Vec4 result = {{ elements[0]+vec[0], elements[1]+vec[1], elements[2]+vec[2], elements[3]+vec[3] }};
		return result;
	}
	const TVec4 operator -( const TVec4& vec ) const
	{
		Vec4 result = {{ elements[0]-vec[0], elements[1]-vec[1], elements[2]-vec[2], elements[3]-vec[3] }};
		return result;
	}
	TVec4& operator +=( const TVec4& vec )
	{
		elements[0]+=vec[0]; elements[1]+=vec[1]; elements[2]+=vec[2]; elements[3]+=vec[3];
		return *this;
	}
};

typedef TVec4<float> Vec4;
typedef TVec4<int>   Vec4i;

inline const Vec4 operator -( const Vec4& vec )
{
	return vec * -1.0f;
}
inline const float Length3D( const Vec4& vec )
{
	float x = vec[0];
	float y = vec[1];
	float z = vec[2];
	return sqrtf(x*x+y*y+z*z);
}
inline const Vec4 Normalize3D( const Vec4& vec )
{
	float s = 1.0f/Length3D(vec);
	Vec4 vecScale = {{s,s,s,1}};
	return vec * vecScale;
}
inline const Vec4 NormalizePlane( const Vec4& vec )
{
	float s = 1.0f/Length3D(vec);
	return vec * s;
}
inline float Dot(const Vec4& a, const Vec4& b)
{
	Vec4 mul = a * b;
	return mul[0] + mul[1] + mul[2] + mul[3];
}
inline float PlaneDotPoint( const Vec4& plane, const Vec4& point )
{
	Vec4 p = point;
	p[3] = 1;
	return Dot(plane, p);
}

inline Vec4i operator<( const Vec4& a, const Vec4& b )//_mm_cmplt_ps
{
	Vec4i result = 
	{{
		a[0] < b[0] ? 0xFFFFFFFF : 0,
		a[1] < b[1] ? 0xFFFFFFFF : 0,
		a[2] < b[2] ? 0xFFFFFFFF : 0,
		a[3] < b[3] ? 0xFFFFFFFF : 0,
	}};
	return result;
}
inline Vec4i operator>( const Vec4& a, const Vec4& b )//
{
	Vec4i result = 
	{{
		a[0] > b[0] ? 0xFFFFFFFF : 0,
		a[1] > b[1] ? 0xFFFFFFFF : 0,
		a[2] > b[2] ? 0xFFFFFFFF : 0,
		a[3] > b[3] ? 0xFFFFFFFF : 0,
	}};
	return result;
}
inline Vec4i& operator&=( Vec4i& a, const Vec4i& b )//_mm_and_ps
{
	a[0] &= b[0];
	a[1] &= b[1];
	a[2] &= b[2];
	a[3] &= b[3];
	return a;
}

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
