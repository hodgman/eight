//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------

#ifndef COMMA
#define COMMA ,
#endif

#define eiUNUSED( a )   (void)(a);

#define eiKiB(x) (      x *1024)
#define eiMiB(x) (eiKiB(x)*1024)
#define eiGiB(x) (eiMiB(x)*1024)

#define eiLikely(  x) !!(x)
#define eiUnlikely(x) !!(x)