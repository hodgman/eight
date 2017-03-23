//------------------------------------------------------------------------------
#pragma once
namespace eight {
//------------------------------------------------------------------------------

typedef          long long 	int64;
typedef unsigned long long	uint64;
typedef unsigned long	    ulong;
typedef          int		int32;
typedef unsigned int	    uint;
typedef unsigned int		uint32;
typedef          short		int16;
typedef unsigned short	    ushort;
typedef unsigned short		uint16;
typedef          char		int8;
typedef unsigned char		uint8;
typedef          int8       byte;
typedef          uint8      ubyte;

typedef  int64 s64;
typedef uint64 u64;
typedef  int32 s32;
typedef uint32 u32;
typedef  int16 s16;
typedef uint16 u16;
typedef  int8  s8;
typedef uint8  u8;

class class_ {};
typedef char class_::* memptr;
typedef void (class_::*memfuncptr)(void*);
typedef void (*callback)(void*);

typedef u8 sizeone[1];
typedef u8 sizetwo[2];

//template <typename T, int N> uint eiArraySize(T(&)[N]) { return N; } //TODO - move

template<uint N> struct Sizer { char elems[N]; };
template<class Type, uint N> Sizer<N> ArraySize_( Type(&)[N] );

#define eiArraySize( a ) sizeof( ArraySize_( a ).elems )


struct Nil
{
	operator void() {}
	bool operator==( const Nil& ) { return true; }
};
const static Nil nil;

template<class T> struct Maybe
{
	Maybe( const T& v ) : value(v), initialized(true) {}
	Maybe( const Nil& ) : initialized(false) {}
	Maybe() : initialized(false) {}
	union
	{
		Nil nil;
		T value;
	};
	bool initialized;
};

#define eiPTR_TO_U64( x ) ((u64)(intptr_t)(void*)(x))
#define eiINTPTR_TO_PTR( x ) ((void*)((intptr_t)x))

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
