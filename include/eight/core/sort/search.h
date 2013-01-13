#pragma once

#include <algorithm>

namespace eight {
//------------------------------------------------------------------------------
template <class I, class T>
I BinarySearch( I first, I last, const T& value )
{
	first = std::lower_bound( first, last, value );
	return (first!=last && !(value<*first)) ? first : last;
}
template <class I, class T, class F>
I BinarySearch( I first, I last, const T& value, F pred )
{
	first = std::lower_bound( first, last, value, pred );
	return (first!=last && !(pred(value,*first))) ? first : last;
}
//------------------------------------------------------------------------------
}
