//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/new.h>
#include <eight/core/alloc/scope.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T>
class ScopeArray
{
public:
	ScopeArray( Scope& a, uint s ) : data(eiAllocArray(a, T, s))/*, size()*/ { eiDEBUG(capacity=s); }
/*	void Push( const T& t )
	{
		eiASSERT( size < capacity );
		data[size++] = t;
	}
	uint Size() const { return size; }*/
	T& operator[]( uint i ) { eiASSERT( /*i < size &&*/ i < capacity ); return data[i]; }
	T* Begin() { return data; }
private:
	T* data;
//	uint size;
	eiDEBUG( uint capacity );
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
