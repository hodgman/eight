//------------------------------------------------------------------------------
#pragma once
#include <eight/core/math/vector.h>
#include <math.h>
namespace eight {
//------------------------------------------------------------------------------

struct Mat4
{
	Vec4 columns[4];
	const Vec4& operator[]( uint i ) const { return columns[i]; }
	      Vec4& operator[]( uint i )       { return columns[i]; }
	const Vec4 operator *( const Vec4& vec ) const
	{
		return
		columns[0] * vec[0] + 
		columns[1] * vec[1] + 
		columns[2] * vec[2] + 
		columns[3] * vec[3];
	}
	const Mat4 operator *( const Mat4& mat ) const
	{
		Mat4 result = {{
			*this * mat[0],
			*this * mat[1],
			*this * mat[2],
			*this * mat[3]
		}};
		return result;
	}
	static const Mat4 Identity()
	{
		Mat4 result = {{
			{ 1.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f }
		}};
		return result;
	}
};

const static double eiPi = 3.1415926535897932384626433832795;
const static double eiHalfPi = 1.5707963267948966192313216916398;
const static float eiPif = (float)eiPi;
const static float eiHalfPif = (float)eiHalfPi;

template<class T> T DegToRad(T x) { return x * (T)(eiPi / 180.0); }

inline const Mat4 Perspective( float fovy, float aspect, float zNear, float zFar )
{
	float f = tanf( eiHalfPif - fovy*0.5f );
	float invRange = 1.0f / ( zNear - zFar );
	Mat4 result = {
		{
			{ f / aspect, 0.0f, 0.0f, 0.0f },
			{ 0.0f, f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, (zNear+zFar) * invRange, -1.0f },
			{ 0.0f, 0.0f, 2*zNear*zFar * invRange,  0.0f }
		}
	};
	return result;
}

inline const Mat4 Transpose( const Mat4& mat )
{
	Mat4 result = {{
		{ mat[0][0], mat[1][0], mat[2][0], mat[3][0] },
		{ mat[0][1], mat[1][1], mat[2][1], mat[3][1] },
		{ mat[0][2], mat[1][2], mat[2][2], mat[3][2] },
		{ mat[0][3], mat[1][3], mat[2][3], mat[3][3] },
	}};
	return result;
}

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
