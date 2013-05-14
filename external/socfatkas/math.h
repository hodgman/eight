#pragma once

#include <xmmintrin.h>
#include <emmintrin.h>
#include <math.h>

#pragma warning(disable:4201)

struct vec2_t
{
	union
	{
		struct
		{
			float x;
			float y;
		};
		float v[2];
	};	
};

struct Matrix
{
	__m128 r[4];
};

inline __m128 Vector4( float a, float b, float c, float d )
{
	return _mm_setr_ps(a,b,c,d);
}

inline __m128 Vector4( float a )//set all 4 to same value
{
	return _mm_set1_ps(a);
}

#define VecShuffleMask(a0,b0,c0,d0) _MM_SHUFFLE(d0,c0,b0,a0)
#define VecShuffle(a,b,mask) _mm_shuffle_ps((a),(b),mask)
#define VecMad(a,b,c) _mm_add_ps(c,_mm_mul_ps(a,b))
#define VecStore(where,what) _mm_store_ps((float*)(where),(what))
#define VecStoreU(where,what) _mm_storeu_ps((float*)(where),(what))
#define VecLoad(what) _mm_load_ps((const float*)(what))
#define VecLoadU(what) _mm_loadu_ps((const float*)(what))

#define Vector4Int(a,b,c,d) _mm_setr_epi32(a,b,c,d)
#define VecIntShuffle(a,mask) _mm_shuffle_epi32((a),mask)
#define VecIntZero() _mm_setzero_si128()
#define VecIntAnd(a,b) _mm_and_si128(a,b)
#define VecIntOr(a,b) _mm_or_si128(a,b)
#define VecIntAndNot(a,b) _mm_andnot_si128(a,b)
#define VecIntXor(a,b) _mm_xor_si128(a,b)
#define VecIntMul(a,b) _mm_mul_epu32(a,b)
#define VecIntAdd(a,b) _mm_add_epi32(a,b)
#define VecIntSub(a,b) _mm_sub_epi32(a,b)
#define VecIntMax(a,b) _mm_max_epi16(a,b)
#define VecIntMin(a,b) _mm_min_epi16(a,b)
#define VecFloat2Int(a) _mm_cvttps_epi32(a)
#define VecIntLoad(where) _mm_load_si128( (const __m128i*)(where))
#define VecIntStore(where,what) _mm_store_si128( (__m128i*)(where),(what))
#define VecIntSRL64(a,b) _mm_srl_epi64((a),(b))
#define VecIntSRL32(a,b) _mm_srl_epi32((a),(b))
#define VecIntSLL64(a,b) _mm_sll_epi64((a),(b))
#define VecIntSLL32(a,b) _mm_sll_epi32((a),(b))
#define VecIntCmpEqual(a,b) _mm_cmpeq_epi32((a),(b))
#define VecIntUnpackLo(a,b) _mm_unpacklo_epi64((a),(b))

inline __m128 Vector3Dotv( const __m128& a, const __m128& b )
{
	__m128 ab = _mm_mul_ps( a, b );
	__m128 x = VecShuffle( ab, ab, VecShuffleMask( 0, 0, 0, 0 ) );
	__m128 y = VecShuffle( ab, ab, VecShuffleMask( 1, 1, 1, 1 ) );
	__m128 z = VecShuffle( ab, ab, VecShuffleMask( 2, 2, 2, 2 ) );
	return _mm_add_ps( x, _mm_add_ps( y, z ) );
}

inline float Vector3Dot( const __m128& a, const __m128& b )
{
	float f;
	_mm_store_ss( &f, Vector3Dotv( a, b ) );
	return f;
}

inline __m128 Vector3Lengthv( const __m128& v )
{
	return _mm_sqrt_ps( Vector3Dotv( v, v ) );
}

inline float Vector3Length( const __m128& v )
{
	float f;
	_mm_store_ss( &f, Vector3Lengthv( v ) );
	return f;
}

inline __m128 Vector3Cross( const __m128& a, const __m128& b )
{
	__m128 v1, v2;

	v1 = VecShuffle( a, a, VecShuffleMask(1, 2, 0, 3) );
	v2 = _mm_mul_ps( VecShuffle(b, b, VecShuffleMask(2, 0, 1, 3)), v1 );
	v1 = _mm_mul_ps( v1, b );
	v1 = VecShuffle( v1, v1, VecShuffleMask(1, 2, 0, 3) );
	return _mm_sub_ps( v2, v1 );
}

inline __m128 Vector3Normalize( const __m128& a )
{
	return _mm_div_ps( a, Vector3Lengthv( a ) );
}

__forceinline void Vector3TransformCoord4( const __m128* m, const __m128* src, vec2_t* dst )
{
	__m128 src0 = src[0];
	__m128 src1 = src[1];
	__m128 src2 = src[2];
	__m128 src3 = src[3];

	__m128 tmp0 = _mm_unpacklo_ps( src0, src1 ); // x0 x1 y0 y1
	__m128 tmp1 = _mm_unpackhi_ps( src0, src1 ); // z0 z1 w0 w1
	__m128 tmp2 = _mm_unpacklo_ps( src2, src3 ); // x2 x3 y2 y3
	__m128 tmp3 = _mm_unpackhi_ps( src2, src3 ); // z2 z3 w2 w3

	__m128 xxxx = _mm_movelh_ps( tmp0, tmp2 ); // x0 x1 x2 x3
	__m128 yyyy = _mm_movehl_ps( tmp2, tmp0 ); // y0 y1 y2 y3
	__m128 zzzz = _mm_movelh_ps( tmp1, tmp3 ); // z0 z1 z2 z3

	__m128 transformed_x = _mm_add_ps( _mm_add_ps( _mm_add_ps( m[12], _mm_mul_ps( zzzz, m[8] ) ), _mm_mul_ps( yyyy, m[4] ) ), _mm_mul_ps( xxxx, m[0] ) );
	__m128 transformed_y = _mm_add_ps( _mm_add_ps( _mm_add_ps( m[13], _mm_mul_ps( zzzz, m[9] ) ), _mm_mul_ps( yyyy, m[5] ) ), _mm_mul_ps( xxxx, m[1] ) );
	__m128 transformed_w = _mm_add_ps( _mm_add_ps( _mm_add_ps( m[15], _mm_mul_ps( zzzz, m[11] ) ), _mm_mul_ps( yyyy, m[7] ) ), _mm_mul_ps( xxxx, m[3] ) );

	tmp0 = _mm_div_ps( _mm_unpacklo_ps( transformed_x, transformed_y ), VecShuffle( transformed_w, transformed_w, VecShuffleMask( 0, 0, 1, 1 ) ) ); // x0 y0 x1 y1
	tmp1 = _mm_div_ps( _mm_unpackhi_ps( transformed_x, transformed_y ), VecShuffle( transformed_w, transformed_w, VecShuffleMask( 2, 2, 3, 3 ) ) ); // x2 y2 x3 y3
	VecStore( dst, tmp0 );
	VecStore( dst + 2, tmp1 );
}

__forceinline void Vector3TransformCoord4Homogeneous( const __m128* m, const __m128* src, __m128* dst )
{
	__m128 src0 = src[0];
	__m128 src1 = src[1];
	__m128 src2 = src[2];
	__m128 src3 = src[3];

	__m128 tmp0 = _mm_unpacklo_ps( src0, src1 ); // x0 x1 y0 y1
	__m128 tmp1 = _mm_unpackhi_ps( src0, src1 ); // z0 z1 w0 w1
	__m128 tmp2 = _mm_unpacklo_ps( src2, src3 ); // x2 x3 y2 y3
	__m128 tmp3 = _mm_unpackhi_ps( src2, src3 ); // z2 z3 w2 w3

	__m128 xxxx = _mm_movelh_ps( tmp0, tmp2 ); // x0 x1 x2 x3
	__m128 yyyy = _mm_movehl_ps( tmp2, tmp0 ); // y0 y1 y2 y3
	__m128 zzzz = _mm_movelh_ps( tmp1, tmp3 ); // z0 z1 z2 z3

	__m128 transformed_x = _mm_add_ps( _mm_add_ps( _mm_add_ps( m[12], _mm_mul_ps( zzzz, m[8] ) ), _mm_mul_ps( yyyy, m[4] ) ), _mm_mul_ps( xxxx, m[0] ) );
	__m128 transformed_y = _mm_add_ps( _mm_add_ps( _mm_add_ps( m[13], _mm_mul_ps( zzzz, m[9] ) ), _mm_mul_ps( yyyy, m[5] ) ), _mm_mul_ps( xxxx, m[1] ) );
	__m128 transformed_z = _mm_add_ps( _mm_add_ps( _mm_add_ps( m[14], _mm_mul_ps( zzzz, m[10] ) ), _mm_mul_ps( yyyy, m[6] ) ), _mm_mul_ps( xxxx, m[2] ) );
	__m128 transformed_w = _mm_add_ps( _mm_add_ps( _mm_add_ps( m[15], _mm_mul_ps( zzzz, m[11] ) ), _mm_mul_ps( yyyy, m[7] ) ), _mm_mul_ps( xxxx, m[3] ) );

	tmp0 = _mm_unpacklo_ps( transformed_x, transformed_y ); // x0 y0 x1 y1
	tmp1 = _mm_unpackhi_ps( transformed_x, transformed_y ); // x2 y2 x3 y3
	tmp2 = _mm_unpacklo_ps( transformed_z, transformed_w ); // z0 w0 z1 w1
	tmp3 = _mm_unpackhi_ps( transformed_z, transformed_w ); // z2 w2 z3 w3

	dst[0] = _mm_movelh_ps( tmp0, tmp2 );
	dst[1] = _mm_movehl_ps( tmp2, tmp0 );
	dst[2] = _mm_movelh_ps( tmp1, tmp3 );
	dst[3] = _mm_movehl_ps( tmp3, tmp1 );
}

__forceinline __m128 Vector3TransformCoord( const Matrix& m, __m128 vec )
{
	__m128 xxxx = VecShuffle( vec, vec, VecShuffleMask(0,0,0,0) );
	__m128 yyyy = VecShuffle( vec, vec, VecShuffleMask(1,1,1,1) );
	__m128 zzzz = VecShuffle( vec, vec, VecShuffleMask(2,2,2,2) );
	__m128 vvvv = _mm_add_ps ( _mm_add_ps ( m.r[3], _mm_mul_ps ( zzzz, m.r[2] ) ), _mm_add_ps( _mm_mul_ps ( yyyy, m.r[1] ), _mm_mul_ps( xxxx, m.r[0] ) ) );
	__m128 transformed_vvvv = _mm_div_ps( vvvv, VecShuffle( vvvv, vvvv, VecShuffleMask( 3, 3, 3, 3 ) ) );
	return VecShuffle( transformed_vvvv, vvvv, VecShuffleMask( 0, 1, 2, 3 ) );
}

inline Matrix MatrixIdentity()
{
	Matrix m;
	m.r[0] = Vector4( 1, 0, 0, 0 );
	m.r[1] = Vector4( 0, 1, 0, 0 );
	m.r[2] = Vector4( 0, 0, 1, 0 );
	m.r[3] = Vector4( 0, 0, 0, 1 );
	return m;
}

inline Matrix operator * ( const Matrix& a, const Matrix& b )
{
	Matrix m;
	__m128 xxxx, yyyy, zzzz, wwww;
#define MUL_LINE(d,s) \
	xxxx = VecShuffle( s, s, VecShuffleMask(0,0,0,0) ); \
	yyyy = VecShuffle( s, s, VecShuffleMask(1,1,1,1) ); \
	zzzz = VecShuffle( s, s, VecShuffleMask(2,2,2,2) ); \
	wwww = VecShuffle( s, s, VecShuffleMask(3,3,3,3) ); \
	d = _mm_add_ps( _mm_add_ps( _mm_mul_ps( b.r[3], wwww ), _mm_mul_ps( b.r[2], zzzz ) ), _mm_add_ps( _mm_mul_ps( b.r[1], yyyy ), _mm_mul_ps( b.r[0], xxxx ) ) );

	MUL_LINE(m.r[0],a.r[0]);
	MUL_LINE(m.r[1],a.r[1]);
	MUL_LINE(m.r[2],a.r[2]);
	MUL_LINE(m.r[3],a.r[3]);
#undef MUL_LINE
	return m;
}

inline Matrix MatrixOrtho( float w, float h, float znear, float zfar )
{
	Matrix m;
	m.r[0] = Vector4( 2 / w, 0, 0, 0 );
	m.r[1] = Vector4( 0, - 2 / h, 0, 0 );
	m.r[2] = Vector4( 0, 0, 1 / (zfar - znear ), 0 );
	m.r[3] = Vector4( -1, 1, -znear / (zfar - znear), 1 );
	return m;
}

inline Matrix MatrixTranslation( const __m128& v )
{
	Matrix m = MatrixIdentity();
	m.r[3] = v;
	return m;
}

inline Matrix MatrixPerspective( float fov, float aspect, float znear, float zfar )
{
	float h = 1 / tanf(fov / 2);
	float w = h / aspect;

	Matrix m;
	m.r[0] = Vector4( w, 0, 0, 0 );
	m.r[1] = Vector4( 0, h, 0, 0 );
	m.r[2] = Vector4( 0, 0, zfar / (zfar - znear ), 1 );
	m.r[3] = Vector4( 0, 0, znear * zfar / (znear - zfar), 0 );

	return m;
}

inline Matrix MatrixLookAt( const __m128& from, const __m128& at, const __m128& up )
{
	__m128 zaxis, yaxis, xaxis;

	zaxis = Vector3Normalize( _mm_sub_ps( at, from ) );
	xaxis = Vector3Normalize( Vector3Cross( up, zaxis ) );
	yaxis = Vector3Normalize( Vector3Cross( zaxis, xaxis ) );

	__m128 v0 = _mm_unpacklo_ps( xaxis, yaxis ); // x0 x1 y0 y1
	__m128 v1 = _mm_unpacklo_ps( zaxis, _mm_setzero_ps() ); // x2 0 y2 0
	__m128 v2 = _mm_unpackhi_ps( xaxis, yaxis ); // z0 z1 ? ?
	__m128 v3 = _mm_unpackhi_ps( zaxis, _mm_setzero_ps() ); // z2 0 ? 0

	Matrix m;
	m.r[0] = _mm_movelh_ps( v0, v1 );
	m.r[1] = _mm_movehl_ps( v1, v0 );
	m.r[2] = _mm_movelh_ps( v2, v3 );
	m.r[3] = Vector4( -Vector3Dot( xaxis, from ), -Vector3Dot( yaxis, from ), -Vector3Dot( zaxis, from ), 1  );

	return m;
}

inline Matrix MatrixScaling( float scale )
{
	Matrix m = MatrixIdentity();
	m.r[0] = Vector4( scale, 0, 0, 0 );
	m.r[1] = Vector4( 0, scale, 0, 0 );
	m.r[2] = Vector4( 0, 0, scale, 0 );
	return m;
}

inline Matrix MatrixScaling( float scalex, float scaley, float scalez )
{
	Matrix m = MatrixIdentity();
	m.r[0] = Vector4( scalex, 0, 0, 0 );
	m.r[1] = Vector4( 0, scaley, 0, 0 );
	m.r[2] = Vector4( 0, 0, scalez, 0 );
	return m;
}