//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T,class Name/*,T d=T()*/>
struct PrimitiveWrap
{
//	explicit PrimitiveWrap( T v=d ) : value(v) {}
	PrimitiveWrap() {}
	explicit PrimitiveWrap( T v ) : value(v) {}
	operator const T&() const { return value; }
	operator       T&()       { return value; }
private:
	T value;
};

#ifndef eiBUILD_RETAIL
# define eiTYPEDEF_ID( name )							\
	struct tag_##name;									\
	typedef PrimitiveWrap<uint,tag_##name> name;		//

# define eiTYPEDEF_PTR( name )							\
	struct tag_##name;									\
	typedef tag_##name* name;							//
#else
# define eiTYPEDEF_ID( name )							\
	typedef uint name;									//

# define eiTYPEDEF_PTR( name )							\
	typedef void* name;									//
#endif

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
