//------------------------------------------------------------------------------
#pragma once
#include <eight/core/debug.h>
#include <eight/core/math/vector.h>
#include <math.h>
namespace eight {
//------------------------------------------------------------------------------

struct eiALIGN(16) Mat4
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
	/*const Mat4 operator *( const Mat4& mat ) const
	{
		Mat4 result = {{
			*this * mat[0],
			*this * mat[1],
			*this * mat[2],
			*this * mat[3]
		}};
		return result;
	}*/
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
	static const Mat4 AxisAngle( const Vec4& axis, float angle );
	static const Mat4 XRotation( float angle );
	static const Mat4 YRotation( float angle );
	static const Mat4 ZRotation( float angle );
	static const Mat4 Translation( const Vec4& t );
	static const Mat4 Scale( float x, float y, float z );
};

inline const Mat4 operator*( const Mat4& lhs, const Mat4& rhs )
{
	Mat4 result = {{
		lhs * rhs[0],
		lhs * rhs[1],
		lhs * rhs[2],
		lhs * rhs[3]
	}};
/*	for (int i=0; i<4; i++)
	{
		for (int j=0; j<4; j++)
		{
			result[i][j] = lhs[i][0] * rhs[0][j] + lhs[i][1] * rhs[1][j] + lhs[i][2] * rhs[2][j] + lhs[i][3] * rhs[3][j];
		}
	}*/
	for (int i=0; i<4; i++)
	{
		for (int j=0; j<4; j++)
		{
			result[j][i] = lhs[0][i] * rhs[j][0]
			             + lhs[1][i] * rhs[j][1]
						 + lhs[2][i] * rhs[j][2]
						 + lhs[3][i] * rhs[j][3];
		}
	}
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

inline const Mat4 Mat4::AxisAngle( const Vec4& axis, float angle )
{
	float s = sinf( angle );
	float c = cosf( angle );
	float i = 1.0f - c;
	float x = axis[0];
	float y = axis[1];
	float z = axis[2];
	Mat4 result = {{
		{ x*x*i + c,   x*y*i + z*s,  z*x*i - y*s, 0 },
		{ x*y*i - z*s, y*y*i + c,    y*z*i + x*s, 0 },
		{ z*x*i + y*s, y*z*i - x*s,  z*z*i + c,   0 },
		{ 0, 0, 0, 1 }
	}};
	return result;
}
inline const Mat4 Mat4::XRotation( float angle )
{
	float s = sinf( angle );
	float c = cosf( angle );
	Mat4 result = {{
		{ 1,  0, 0, 0 },
		{ 0,  c, s, 0 },
		{ 0, -s, c, 0 },
		{ 0,  0, 0, 1 }
	}};
	return result;
}
inline const Mat4 Mat4::YRotation( float angle )
{
	float s = sinf( angle );
	float c = cosf( angle );
	Mat4 result = {{
		{ c, 0, -s, 0 },
		{ 0, 1,  0, 0 },
		{ s, 0,  c, 0 },
		{ 0, 0,  0, 1 }
	}};
	return result;
}
inline const Mat4 Mat4::ZRotation( float angle )
{
	float s = sinf( angle );
	float c = cosf( angle );
	Mat4 result = {{
		{  c, s, 0, 0 },
		{ -s, c, 0, 0 },
		{  0, 0, 1, 0 },
		{  0, 0, 0, 1 }
	}};
	return result;
}
inline const Mat4 Mat4::Translation( const Vec4& trans )
{
	Vec4 t = trans;
	t[3] = 1;
	Mat4 result = {{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		t
	}};
	return result;
}
inline const Mat4 Mat4::Scale( float x, float y, float z )
{
	Mat4 result = {{
		{ x, 0, 0, 0 },
		{ 0, y, 0, 0 },
		{ 0, 0, z, 0 },
		{ 0, 0, 0, 1 },
	}};
	return result;
}

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
			{ f / aspect, 0.0f, 0.0f,                     0.0f },
			{ 0.0f,       f,    0.0f,                     0.0f },
			{ 0.0f,       0.0f, (zNear+zFar) * invRange, -1.0f },
			{ 0.0f,       0.0f, 2*zNear*zFar * invRange,  0.0f }
		}
	};
/*	float f = 1.0f/tanf( fovy*0.5f );
	float invRange = 1.0f / ( zFar - zNear );
	Mat4 result = {
		{
			{ f / aspect, 0.0f, 0.0f,                    0.0f },
			{ 0.0f,       f,    0.0f,                    0.0f },
			{ 0.0f,       0.0f, zFar * invRange,         1.0f },
			{ 0.0f,       0.0f, -zNear*zFar * invRange,  0.0f }
		}
	};*/
	return result;
}

inline const Mat4 FastInverse( const Mat4& mat )
{
	Mat4 result = {{
		{ mat[0][0], mat[1][0], mat[2][0], 0 },
		{ mat[0][1], mat[1][1], mat[2][1], 0 },
		{ mat[0][2], mat[1][2], mat[2][2], 0 },
	}};
	Vec4 col3 = -( result[0] * mat[3][0] + result[1] * mat[3][1] + result[2] * mat[3][2] );
	col3[3] = 1;
	result[3] = col3;
	return result;
}

inline const Mat4 FullInverse(const Mat4& mat)
{
	Mat4 result;
	const float* m = &mat.columns[0].elements[0];
	float* invOut = &result.columns[0].elements[0];
	float inv[16], det;
	int i;

	inv[0] = m[5]  * m[10] * m[15] - 
		m[5]  * m[11] * m[14] - 
		m[9]  * m[6]  * m[15] + 
		m[9]  * m[7]  * m[14] +
		m[13] * m[6]  * m[11] - 
		m[13] * m[7]  * m[10];

	inv[4] = -m[4]  * m[10] * m[15] + 
		m[4]  * m[11] * m[14] + 
		m[8]  * m[6]  * m[15] - 
		m[8]  * m[7]  * m[14] - 
		m[12] * m[6]  * m[11] + 
		m[12] * m[7]  * m[10];

	inv[8] = m[4]  * m[9] * m[15] - 
		m[4]  * m[11] * m[13] - 
		m[8]  * m[5] * m[15] + 
		m[8]  * m[7] * m[13] + 
		m[12] * m[5] * m[11] - 
		m[12] * m[7] * m[9];

	inv[12] = -m[4]  * m[9] * m[14] + 
		m[4]  * m[10] * m[13] +
		m[8]  * m[5] * m[14] - 
		m[8]  * m[6] * m[13] - 
		m[12] * m[5] * m[10] + 
		m[12] * m[6] * m[9];

	inv[1] = -m[1]  * m[10] * m[15] + 
		m[1]  * m[11] * m[14] + 
		m[9]  * m[2] * m[15] - 
		m[9]  * m[3] * m[14] - 
		m[13] * m[2] * m[11] + 
		m[13] * m[3] * m[10];

	inv[5] = m[0]  * m[10] * m[15] - 
		m[0]  * m[11] * m[14] - 
		m[8]  * m[2] * m[15] + 
		m[8]  * m[3] * m[14] + 
		m[12] * m[2] * m[11] - 
		m[12] * m[3] * m[10];

	inv[9] = -m[0]  * m[9] * m[15] + 
		m[0]  * m[11] * m[13] + 
		m[8]  * m[1] * m[15] - 
		m[8]  * m[3] * m[13] - 
		m[12] * m[1] * m[11] + 
		m[12] * m[3] * m[9];

	inv[13] = m[0]  * m[9] * m[14] - 
		m[0]  * m[10] * m[13] - 
		m[8]  * m[1] * m[14] + 
		m[8]  * m[2] * m[13] + 
		m[12] * m[1] * m[10] - 
		m[12] * m[2] * m[9];

	inv[2] = m[1]  * m[6] * m[15] - 
		m[1]  * m[7] * m[14] - 
		m[5]  * m[2] * m[15] + 
		m[5]  * m[3] * m[14] + 
		m[13] * m[2] * m[7] - 
		m[13] * m[3] * m[6];

	inv[6] = -m[0]  * m[6] * m[15] + 
		m[0]  * m[7] * m[14] + 
		m[4]  * m[2] * m[15] - 
		m[4]  * m[3] * m[14] - 
		m[12] * m[2] * m[7] + 
		m[12] * m[3] * m[6];

	inv[10] = m[0]  * m[5] * m[15] - 
		m[0]  * m[7] * m[13] - 
		m[4]  * m[1] * m[15] + 
		m[4]  * m[3] * m[13] + 
		m[12] * m[1] * m[7] - 
		m[12] * m[3] * m[5];

	inv[14] = -m[0]  * m[5] * m[14] + 
		m[0]  * m[6] * m[13] + 
		m[4]  * m[1] * m[14] - 
		m[4]  * m[2] * m[13] - 
		m[12] * m[1] * m[6] + 
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] + 
		m[1] * m[7] * m[10] + 
		m[5] * m[2] * m[11] - 
		m[5] * m[3] * m[10] - 
		m[9] * m[2] * m[7] + 
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] - 
		m[0] * m[7] * m[10] - 
		m[4] * m[2] * m[11] + 
		m[4] * m[3] * m[10] + 
		m[8] * m[2] * m[7] - 
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] + 
		m[0] * m[7] * m[9] + 
		m[4] * m[1] * m[11] - 
		m[4] * m[3] * m[9] - 
		m[8] * m[1] * m[7] + 
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] - 
		m[0] * m[6] * m[9] - 
		m[4] * m[1] * m[10] + 
		m[4] * m[2] * m[9] + 
		m[8] * m[1] * m[6] - 
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	eiASSERT(det != 0);

	det = 1.0f / det;

	for (i = 0; i < 16; i++)
		invOut[i] = inv[i] * det;
	return result;
}

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
