//------------------------------------------------------------------------------
#pragma once
#include <float.h>
namespace eight {
//------------------------------------------------------------------------------
	
template<class T> T min( T a, T b ) { return a<b ? a : b; }
template<class T> T max( T a, T b ) { return a>b ? a : b; }
template<class T> T Min( T a, T b ) { return a<b ? a : b; }
template<class T> T Max( T a, T b ) { return a>b ? a : b; }

inline int isnan(double x) { return _isnan(x); };

inline float Clamp( float x, float min, float max ) { return x < min ? min : (x > max ? max : x); }
inline float Saturate( float x ) { return Clamp(x,0,1); }
inline float SignedSaturate( float x ) { return Clamp(x,-1,1); }
inline float Sign( float x ) { return x>=0?1.0f:-1.0f; }
inline float Lerp(float b, float a, float f) { return a*f + b*(1-f); }

inline unsigned short ClampFloatToU16( float f )
{
	int i = f < 0 ? 0
		: (f > 0xFFFF ? 0xFFFF : (int)f);
	return (unsigned short)(i & 0xFFFF);
}

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
