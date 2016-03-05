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