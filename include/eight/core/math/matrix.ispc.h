
#include "eight/core/math/types.ispc.h"

inline uniform Mat4 Transpose( uniform Mat4 m )
{
	uniform Vec4 temp0 = { m.c[0][0], m.c[1][0], m.c[0][1], m.c[1][1] };
	uniform Vec4 temp1 = { m.c[2][0], m.c[3][0], m.c[2][1], m.c[3][1] };
	uniform Vec4 temp2 = { m.c[0][2], m.c[1][2], m.c[0][3], m.c[1][3] };
	uniform Vec4 temp3 = { m.c[2][2], m.c[3][2], m.c[2][3], m.c[3][3] };
	uniform Mat4 result = {{ 
		{ temp0.x, temp0.y, temp1.x, temp1.y },
		{ temp0.z, temp0.w, temp1.z, temp1.w },
		{ temp2.x, temp2.y, temp3.x, temp3.y },
		{ temp2.z, temp2.w, temp3.z, temp3.w }
	}};
	return result;
}

inline uniform Mat4 FastInverse( uniform Mat4 mat_ )
{
	uniform Mat4 mat = mat_;
	uniform Vec4 c3 = mat.c[3];
	uniform Vec4 zero = {0,0,0,0};
	mat.c[3] = zero;
	uniform Mat4 result = Transpose(mat);
	uniform Vec4 col3 = -( result.c[0] * c3[0] + result.c[1] * c3[1] + result.c[2] * c3[2] );
	col3[3] = 1;
	result.c[3] = col3;
	return result;
}

inline uniform Mat4 mul( uniform Mat4 lhs, uniform Mat4 rhs )
{
	uniform Mat4 result;
	for (uniform unsigned int j=0; j!=4; j++)
	{
		foreach ( i = 0 ... 4 )
		{
			result.c[j][i] = lhs.c[0][i] * rhs.c[j][0]
			               + lhs.c[1][i] * rhs.c[j][1]
			               + lhs.c[2][i] * rhs.c[j][2]
			               + lhs.c[3][i] * rhs.c[j][3];
		}
	}
	return result;
}