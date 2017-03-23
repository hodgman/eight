//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------

eiInfoGroup( Pool, true );

template<bool Enumerable>
class PoolBase : NonCopyable
{
public:
	PoolBase( Scope& a, uint size );
	void Clear() { eiASSERT( false && "todo" ); }
	void Dbg_ValidateIndex(uint i) const;
	bool IsIndexValid(uint i) const;
	int Alloc();//ret -1 on fail
	void Release( int index );
	bool Empty() const;
	int Begin();
	int Next(uint index);
	uint Capacity() const { return m_size; }
private:
	struct Link { u16 prev, next; }; eiSTATIC_ASSERT( sizeof(Link) == sizeof(u32) );
	const uint  m_size;
	       int  m_free;
	       int* m_offsNextFreeMinusOne;
		   //if Enumerable: //todo - use specialization to avoid having these mempers in non Enumerable instances
	       int  m_used;
	      Link* m_usedLinks;//todo - could just use the m_offsNextFreeMinusOne data, as it's invalid when allocated...
	eiDEBUG( uint m_dbgCount );
};

//------------------------------------------------------------------------------

template<class T, bool Enumerable, bool POD=false>
class Pool : NonCopyable
{
	eiDEBUG( const static bool dbgNoMemcmp = true );
public:
	Pool( Scope& a, uint size );
	      T& operator[](uint i);
	const T& operator[](uint i) const;
	uint Index(const T& object);
	T*   Alloc(const T& data);
	T*   Alloc();
	void Release( T* object );
	T*   Begin();
	T*   Next(const T& object);
	uint Capacity() const { return m_pool.Capacity(); }
	bool IsIndexValid(u32 i) const { return m_pool.IsIndexValid(i); }
private:
	PoolBase<Enumerable> m_pool;
	T* m_data;
};

//------------------------------------------------------------------------------
#include "pool.hpp"
//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
