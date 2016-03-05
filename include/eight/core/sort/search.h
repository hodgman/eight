#pragma once

#include <eight/core/algorithm.h>

namespace eight {
//------------------------------------------------------------------------------
template <class I, class T>
I BinarySearch( I begin, I end, const T& value )
{
	begin = std::lower_bound( begin, end, value );
	return (begin!=end && !(value<*begin)) ? begin : end;
}
template <class I, class T, class F>
I BinarySearch( I begin, I end, const T& value, F pred )
{
	begin = std::lower_bound( begin, end, value, pred );
	return (begin!=end && !(pred(value,*begin))) ? begin : end;
}
//------------------------------------------------------------------------------
template <class I, class T>
I LinearSearch( I begin, I end, const T& value )
{
	for( I i = begin; i != end; ++i )
	{
		if( *i == value )
			return i;
	}
	return end;
}
template <class I, class T, class F>
I LinearSearch( I begin, I end, const T& value, F pred )
{
	for( I i = begin; i != end; ++i )
	{
		if( pred(*i,value) )
			return i;
	}
	return end;
}
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
