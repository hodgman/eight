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
	void Dbg_ValidateIndex(uint i);
	int Alloc();
	void Release( int index );
	bool Empty() const;
	int Begin();
	int Next(uint index);
	uint Capactiy() const { return m_size; }
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

template<class T, bool Enumerable>
class Pool : NonCopyable
{
	const static bool s_NonPod = true;
	eiDEBUG( const static bool dbgNoMemcmp = true );
public:
	Pool( Scope& a, uint size );
	T& operator[](uint i);
	uint Index(const T& object);
	T*   Alloc(const T& data);
	T*   Alloc();
	void Release( T* object );
	T*   Begin();
	T*   Next(const T& object);
private:
	PoolBase<Enumerable> m_pool;
	T* m_data;
};

//------------------------------------------------------------------------------
#include "pool.hpp"
//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
