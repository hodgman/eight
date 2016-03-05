#pragma once

#include <cmath>
#include <float.h>
#if defined(eiBUILD_WINDOWS)
# include "intrin.h"
# include "emmintrin.h"
#pragma intrinsic(_BitScanReverse)
#ifdef eiBUILD_X64
#pragma intrinsic(_BitScanReverse64)
#endif
#elif defined(eiBUILD_XBONE)
# include "intrin.h"
# include "nmmintrin.h"
# include "immintrin.h"
#elif defined(eiBUILD_PS4)
# include "x86intrin.h"
#endif

typedef __m128  m128;
typedef __m128i m128i;

inline m128i SetZero_m128i() { return _mm_setzero_si128(); }
inline m128i SetOne_m128i() { return _mm_set_epi32(0,0,0,1); }
inline m128i And_m128i(m128i a, m128i b) { return _mm_and_si128(a, b); }
inline m128i Or_m128i(m128i a, m128i b) { return _mm_or_si128(a, b); }
template<int bytes> inline m128i ShiftLeftBytes_m128i(m128i a) { return _mm_slli_si128(a, bytes); }

template<bool high> struct Helper_ShiftLeft_m128i {
	template<int bits> static inline m128i go(m128i a)
	{
		m128i v1;
		v1 = _mm_slli_si128(a, 8);
		v1 = _mm_slli_epi64(v1, bits - 64);
		return v1;
	}
};
template<> struct Helper_ShiftLeft_m128i<false> {
	template<int bits> static inline m128i go(m128i a)
	{
		m128i v1, v2;
		v1 = _mm_slli_epi64(a, bits);
		v2 = _mm_slli_si128(a, 8);
		v2 = _mm_srli_epi64(v2, 64 - bits);
		v1 = _mm_or_si128(v1, v2);
		return v1;
	}
};
template<int bits> inline m128i ShiftLeft_m128i(m128i a)
{
	return Helper_ShiftLeft_m128i<bits >= 64>::go<bits>(a);
}


template<class T> T Saturate( T f )
{
	return f < 0 ? 0 : (f > 1 ? 1 : f);
}
template<class T> T Clamp( T f, T min, T max )
{
	return f < min ? min : (f > max ? max : f);
}
template<class T> T SaturateNanSafe( T f )
{
#if defined(eiBUILD_WINDOWS)
	f = _isnan(f) ? (T)0 : f;
#else
	f = std::isnan(f) ? (T)0 : f;
#endif
	return f < 0 ? 0 : (f > 1 ? 1 : f);
}