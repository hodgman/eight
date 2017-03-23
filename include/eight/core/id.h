//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------
	
#define ei4CC(str)                                          \
	((u32)(u8)(str[0])        | ((u32)(u8)(str[1]) << 8) |   \
	((u32)(u8)(str[2]) << 16) | ((u32)(u8)(str[3]) << 24 ))  //

template<class T,class Name/*,T d=T()*/>
struct PrimitiveWrap
{
	typedef T Type;
//	explicit PrimitiveWrap( T v=d ) : value(v) {}
	PrimitiveWrap() : value() {}
	explicit PrimitiveWrap( T v ) : value(v) {}
	operator const T&() const { return value; }
	operator       T&()       { return value; }
private:
	T value;
};

template<class T> struct HideIntCastWarnings { typedef T Type; };
template<class T, class N> struct HideIntCastWarnings< PrimitiveWrap<T, N> > { typedef T Type; };

//#ifndef eiBUILD_RETAIL
# define eiTYPEDEF_ID( name )							\
	struct tag_##name;									\
	typedef PrimitiveWrap<uint,tag_##name> name;		//

#define eiTYPEDEF_ID_U64( name )						\
	struct tag_##name;									\
	typedef PrimitiveWrap<u64,tag_##name> name;			//

#define eiTYPEDEF_ID_U32( name )						\
	struct tag_##name;									\
	typedef PrimitiveWrap<u32,tag_##name> name;			//

#define eiTYPEDEF_ID_U16( name )						\
	struct tag_##name;									\
	typedef PrimitiveWrap<u16,tag_##name> name;			//

#define eiTYPEDEF_ID_U8( name )							\
	struct tag_##name;									\
	typedef PrimitiveWrap<u8,tag_##name> name;			//

#define eiTYPEDEF_ID_PTR( name )						\
	struct tag_##name;									\
	typedef tag_##name* name;							//

// #else
// # define eiTYPEDEF_ID( name )							\
// 	typedef uint name;									//
// 
// # define eiTYPEDEF_PTR( name )							\
// 	typedef void* name;									//
// #endif

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
