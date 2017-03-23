//------------------------------------------------------------------------------
#pragma once
#include "stddef.h"
//------------------------------------------------------------------------------

#ifndef COMMA
#define COMMA ,
#endif

#define eiUNUSED( a )   (void)sizeof((void)(a),1)

#define eiKiB(x) (      x *1024)
#define eiMiB(x) (eiKiB(x)*1024)
#define eiGiB(x) (eiMiB(x)*1024)

#define eiLikely(  x) !!(x)
#define eiUnlikely(x) !!(x)

#define eiJOIN_(a, b) a##b
#define eiJOIN(a, b) eiJOIN_(a, b)


#define eiOffsetOf(X, Y) offsetof(X, Y)

#define eiAlignOf(T) __alignof(T)

#ifdef eiBUILD_64BIT
#define eiMAGIC_POINTER( bytePattern )	\
	((void*)(ptrdiff_t(bytePattern&0xff) | (ptrdiff_t(bytePattern&0xff)<<8) | (ptrdiff_t(bytePattern&0xff)<<16) | ((bytePattern&0xff)<<24) | (ptrdiff_t(bytePattern&0xff)<<32) | (ptrdiff_t(bytePattern&0xff)<<40) | (ptrdiff_t(bytePattern&0xff)<<48) | (ptrdiff_t(bytePattern&0xff)<<56)))
#else
#define eiMAGIC_POINTER( bytePattern )	\
	((void*)(ptrdiff_t(bytePattern&0xff) | (ptrdiff_t(bytePattern&0xff)<<8) | (ptrdiff_t(bytePattern&0xff)<<16) | (ptrdiff_t(bytePattern&0xff)<<24)))
#endif