//------------------------------------------------------------------------------
#pragma once
#include <math.h>
#include <eight/core/align.h>
#include <eight/core/types.h>
#include <eight/core/math/arithmetic.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T>
struct eiALIGN(16) TVec4
{
	static TVec4 Create(T x, T y, T z, T w) { TVec4 t = {{x,y,z,w}}; return t; }
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

template<class T, int N>
struct TVecN
{
	T elements[N];
	T& operator[]( uint i )       { return elements[i]; }
	const T& operator[]( uint i ) const { return elements[i]; }
/*	const TVec4 operator *( float s ) const
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
	}*/
};


typedef TVecN<float, 2> Vec2;
typedef TVecN<float, 3> Vec3;
typedef TVecN<bool, 3> Vec3b;

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
inline float Dot3D(const Vec4& a, const Vec4& b)
{
	Vec4 mul = a * b;
	return mul[0] + mul[1] + mul[2];
}
inline Vec4 Cross3D(const Vec4& a, const Vec4& b)
{
	Vec4 c;
	c[0] = a[1]*b[2] - a[2]*b[1];
	c[1] = a[2]*b[0] - a[0]*b[2];
	c[2] = a[0]*b[1] - a[1]*b[0];
	c[3] = 0;
	return c;
}
inline float PlaneDotPoint( const Vec4& plane, const Vec4& point )
{
	Vec4 p = point;
	p[3] = 1;
	return Dot(plane, p);
}
inline Vec4 ToPlane( const Vec4& pos, const Vec4& normal )
{
	Vec4 plane = normal;
	plane[3] = -Dot3D( pos, normal );
	return plane;
};

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

inline Vec4 Lerp3D(const Vec4& a, const Vec4& b, float f)
{
	Vec4 result = {{
		Lerp(a[0], b[0], f),
		Lerp(a[1], b[1], f),
		Lerp(a[2], b[2], f),
		0,
	}};
	return result;
}

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
