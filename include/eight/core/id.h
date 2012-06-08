//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T,class Name,T d=T()>
struct PrimitiveWrap
{
	explicit PrimitiveWrap( T v=d ) : value(v) {}
	operator const T&() const { return value; }
	operator       T&()       { return value; }
private:
	T value;
};

#ifndef eiBUILD_RETAIL
# define eiTYPEDEF_ID( name )							\
	struct tag_##name {};								\
	typedef PrimitiveWrap<uint,tag_##name> name;		//
#else
# define eiTYPEDEF_ID( name )							\
	typedef uint name;									//
#endif

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
