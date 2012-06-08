//------------------------------------------------------------------------------
#pragma once
namespace eight {
//------------------------------------------------------------------------------

template<int V, int I> struct Pow2Down_Try;
template<bool B, int V, int I> struct Pow2Down_Select
{
	const static int value = I-1;//bust, use prev I
};
template<int V, int I> struct Pow2Down_Select<false,V,I>
{
	const static int value = Pow2Down_Try<V, I+1>::value;//valid, try next I
};
template<int V, int I> struct Pow2Down_Try
{
	const static int value = Pow2Down_Select< (1<<I > V), V, I >::value;//see if we've bust or not
};
template<int V> struct Pow2Down
{
	const static int value = Pow2Down_Try<V,1>::value;//start at 2^1 and increment the exponent until over V, then back up to previous (good) value
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
