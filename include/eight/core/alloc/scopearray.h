//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/new.h>
#include <eight/core/alloc/scope.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T, bool POD=true>
class ScopeArray
{
public:
	ScopeArray( Scope& a, uint s ) : data( s ? eiAllocArray( a, T, s ) : 0 ) { eiDEBUG( capacity=s ); }
	T& operator[]( uint i ) { eiASSERT( i < capacity ); return data[i]; }
	const T& operator[]( uint i ) const { eiASSERT( i < capacity ); return data[i]; }
	T* Begin() { return data; }
	int Index( const T& item ) { return (int)(&item - data); }
private:
	T* data;
	eiDEBUG( uint capacity );
};
template<class T>
class ScopeArray<T,false>
{
public:
	ScopeArray( Scope& a, uint s ) : data( s ? eiNewArray( a, T, s ) : 0 ) { eiDEBUG( capacity=s ); }
	T& operator[]( uint i ) { eiASSERT( i < capacity ); return data[i]; }
	const T& operator[]( uint i ) const { eiASSERT( i < capacity ); return data[i]; }
	T* Begin() { return data; }
	int Index( const T& item ) { return (int)(&item - data); }
private:
	T* data;
	eiDEBUG( uint capacity );
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
