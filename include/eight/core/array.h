#pragma once
#include <eight/core/types.h>
#include <eight/core/alloc/new.h>
#include <eight/core/debug.h>

namespace eight {
	class Scope;
	
template<class T> struct Ptr
{
	Ptr(T* data=0) : p(data) {}
	operator       T*()       { return p; }
	operator const T*() const { return p; }
	      T* operator->()       { eiASSERT(p); return  p; }
	const T* operator->() const { eiASSERT(p); return  p; }
	      T& operator *()       { eiASSERT(p); return *p; }
	const T& operator *() const { eiASSERT(p); return *p; }
	operator bool() const { return !!begin; }
private:
	T* p;
};
template<class T> struct ArrayPtr
{
	ArrayPtr() : begin() { eiDEBUG(dbg_size=0); }
	ArrayPtr(Scope& a, uint size) : begin(eiAllocArray(a,T,size)) { eiDEBUG(dbg_size=size); }
	ArrayPtr(T* data, uint size) : begin(data) { eiDEBUG(dbg_size=size); }
	      T& operator[]( uint i )       { eiASSERT(i<dbg_size); return begin[i]; }
	const T& operator[]( uint i ) const { eiASSERT(i<dbg_size); return begin[i]; }
	operator bool() const { return !!begin; }
private:
	T* begin;
	eiDEBUG( uint dbg_size );
};

template<class T> struct ArrayData
{
	uint length;
	T*   data;
	explicit ArrayData( uint length=0 )
		: length(length)
		, data(length ? eiNewArray(GlobalHeap, T, length) : 0)
	{
	}
	ArrayData( ArrayData&& o )
		: length(o.length)
		, data(o.data)
	{
		o.length = 0;
		o.data = 0;
	}
	ArrayData( const ArrayData& o )
		: length(0)
		, data(0)
	{
		*this = o;
	}
	void Consume( ArrayData& o )
	{
		Clear();
		length = o.length;
		data = o.data;
		o.length = 0;
		o.data = 0;
	}
	ArrayData& operator=( const ArrayData& o )
	{
		Clear();
		length = o.length;
		data = o.length ? eiNewArray(GlobalHeap, T, o.length) : 0;
		for( uint i=0; i!=o.length; ++i )
			data[i] = o.data[i];
		return *this;
	}
	~ArrayData()
	{
		eiDeleteArray(GlobalHeap, data, length);
	}
	void ClearToSize(uint size)
	{
		Clear();
		if( size )
		{
			length = size;
			data = eiNewArray(GlobalHeap, T, size);
		}
	}
	void Clear()
	{
		eiDeleteArray(GlobalHeap, data, length);
		data = 0;
		length = 0;
	}

	      T& operator[]( uint i )       { eiASSERT(i<length); return data[i]; }
	const T& operator[]( uint i ) const { eiASSERT(i<length); return data[i]; }
};

}
