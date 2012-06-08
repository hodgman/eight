#pragma once

#include <eight/core/types.h>
#include <eight/core/performance.h>

namespace eight {
//------------------------------------------------------------------------------
template<class T>
struct DefaultRadixKey
{
	typedef T Type;//the type that the algorithm is actually sorting from
	Type operator()( T* in ) const { return *in; }//convert an array element into a key
};

#define eiRESTRICT

template<class T, class FnKey>
T* RadixSort( T* eiRESTRICT inputBegin, T* eiRESTRICT bufferBegin, uint size, const FnKey& fnKey );

template<class T>
T* RadixSort( T* eiRESTRICT inputBegin, T* eiRESTRICT bufferBegin, uint size );

//------------------------------------------------------------------------------
#include "radix.hpp"
//------------------------------------------------------------------------------
}
