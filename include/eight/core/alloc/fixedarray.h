//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/debug.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T, uint SIZE>
class FixedArray
{
public:
	T& operator[](uint i) { eiASSERT(i < SIZE); return data[i]; }
	const T& operator[](uint i) const { eiASSERT(i < SIZE); return data[i]; }
	int IndexOf(const T& t) const
	{
		for (uint i = 0; i != SIZE; ++i)
		{
			if (data[i] == t)
				return i;
		}
		return -1;
	}
private:
	T data[SIZE];
};


template<class T>
class FixedArray<T, 0>
{
public:
	T& operator[](int i) { eiFatalError("bounds check"); return *(T*)0; }
	const T& operator[](int i) const { eiFatalError("bounds check"); return *(T*)0; }
	int IndexOf(const T&) const { return -1; }
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
