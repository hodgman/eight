//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------

eiInfoGroup( Pool, true );
//WIP!
template<class T, bool Enumerable> class Pool : NonCopyable
{
	const static bool s_NonPod = true;
	eiDEBUG( const static bool dbgNoMemcmp = true );
public:
	Pool( Scope& a, uint size ) : m_size(size), m_free(0), m_data(eiAllocArray(a,T,size)), m_offsNextFreeMinusOne(eiAllocArray(a,int,size)), m_used(-1), m_usedLinks(Enumerable ? eiAllocArray(a,Link,size) : 0)
	{
		eiASSERT( !Enumerable || size < 0xFFFF );
		eiDEBUG( if( Enumerable ) memset( m_usedLinks, 0xFFFFFFFF, sizeof(Link)*size ) );
		memset( m_offsNextFreeMinusOne, 0, sizeof(int)*size );
		m_offsNextFreeMinusOne[size-1] = -1;
		eiDEBUG( m_dbgCount = 0 );
	}
	~Pool()
	{/*
		if( (false eiDEBUG(||true)) && s_NonPod )
		{
			eiDEBUG( int endNodes = 0 );
			bool leaked = false;
			for( uint i=0; i!=m_size; ++i )
			{
				// either in the free list (i.e. not allocated) OR contains data, but not both.
				eiDEBUG( u8 null[sizeof(T)]= {}; );
				eiDEBUG( bool containsData = memcmp(&m_data[i], null, sizeof(T)) != 0 );//not valid test in general case
				eiDEBUG( bool inFreeList = m_offsNextFreeMinusOne[i] != -1 );
				eiDEBUG( endNodes += !(inFreeList ^ containsData) );
				if( m_offsNextFreeMinusOne[i] == -1 )//not in free list
				{
					m_data[i].~T();
					leaked = true;
				}
			}
			eiASSERT( dbgNoMemcmp || endNodes == 1 );
			eiASSERT( !leaked );
		}*/
	}
	T& operator[](uint i)
	{
		eiASSERT( i  < m_size );
		T& data = m_data[i];
		eiASSERT( m_offsNextFreeMinusOne[i] == -1 );
		eiASSERT( !Enumerable || m_used==i || (m_usedLinks[i].prev != 0xFFFF && m_usedLinks[i].next != 0xFFFF) );
		eiDEBUG( u8 null[sizeof(T)]= {}; );
		eiASSERT( dbgNoMemcmp || memcmp(&data, null, sizeof(T))!=0 );
		return data;
	}
	uint Index(const T& object)
	{
		eiASSERT( &object >= m_data && &object < m_data+m_size );
		int index = &object-m_data;
		return (uint)index;
	}
	T* Alloc(const T& data)
	{
		T* t = Alloc_();
		if( t )
		{
			eiDEBUG( u8 null[sizeof(T)]= {}; );
			eiASSERT( dbgNoMemcmp || memcmp(t, null, sizeof(T))==0 );
			if( s_NonPod )
				new(t) T(data);
			else
				*t = data;
			eiASSERT( dbgNoMemcmp || memcmp(t, null, sizeof(T))!=0 );
		}
		return t;
	}
	T* Alloc()
	{
		T* t = Alloc_();
		return (t && s_NonPod) ? new(t) T : t;
	}
	void Release( T* object )
	{
		int index = object-m_data;
		eiASSERT( index >= 0 && (uint)index < m_size );
		if( m_free < 0 )
			m_offsNextFreeMinusOne[index] = -1;
		else
		{
			int offsNextFree = m_free - index;
			m_offsNextFreeMinusOne[index] = offsNextFree - 1;
		}
		if( s_NonPod )
			object->~T();
		if( Enumerable )
		{
			eiASSERT( m_used >= 0 && (uint)m_used < m_size );
			Link& link = m_usedLinks[index];
			if( m_used == index )
			{
				eiASSERT( link.prev == 0xFFFF );
				if( link.next == 0xFFFF )
				{
					eiASSERT( m_dbgCount == 1 );
					m_used = -1;
				}
				else
				{
					eiASSERT( m_dbgCount > 1 );
					eiASSERT( link.next < m_size );
					Link& next = m_usedLinks[link.next];
					eiASSERT( next.prev == index );
					m_used = link.next;
					next.prev = 0xFFFF;
				}
			}
			else
			{
				eiASSERT( m_dbgCount > 1 );
				eiASSERT( link.prev != 0xFFFF && link.prev < m_size );
				Link& prev = m_usedLinks[link.prev];
				prev.next = link.next;
				if( link.next != 0xFFFF )
				{
					eiASSERT( m_dbgCount > 2 );
					eiASSERT( link.next < m_size );
					Link& next = m_usedLinks[link.next];
					next.prev = link.prev;
				}
			}
			eiDEBUG( (*(u32*)&m_usedLinks[index]) = 0xFFFFFFFF );
		}
		eiDEBUG( memset(object, 0, sizeof(T)) );
		m_free = index;
		eiDEBUG( m_dbgCount-- );
	}
	bool Empty() const
	{
		if( !Enumerable )
		{
			int index = m_free;
			if( index < 0 )
			{
				eiASSERT( m_dbgCount == m_size );
				return false;
			}
			int next;
			int numFree = 0;
			do
			{
				++numFree;
				next = index + m_offsNextFreeMinusOne[index] + 1;
			}while( next != index );
			eiASSERT( m_dbgCount == m_size-numFree );
			return numFree==m_size;
		}
		else
		{
			eiASSERT( (m_used==-1)==(m_dbgCount==0) );
			return m_used==-1;
		}
	}
	T* Begin()
	{
		return m_used == -1 ? 0 : &m_data[m_used];
	}
	T* Next(const T& object)
	{
		uint index = Index(object);
		uint next = m_usedLinks[index].next;
		eiASSERT( next == 0xFFFF || next < m_size );
		return next == 0xFFFF ? 0 : &m_data[next];
	}
private:
	T* Alloc_()
	{
		int index = m_free;
		T* result = &m_data[index];
		if( index < 0 )
		{
			eiInfo( Pool, "Alloc fail - Pool full" );
			return 0;
		}
		int next = index + m_offsNextFreeMinusOne[index] + 1;
		if( next == index )
		{
			next = -1;
			eiInfo( Pool, "Pool filled" );
		}
		m_free = next;
		m_offsNextFreeMinusOne[index] = -1;
		if( Enumerable )
		{
			eiASSERT( (*(u32*)&m_usedLinks[index]) == 0xFFFFFFFF );
			eiASSERT( m_used == -1 || (m_used>=0 && (uint)m_used<m_size) );
			Link l = { 0xFFFF, m_used>=0 ? (u16)m_used : 0xFFFF };
			m_usedLinks[index] = l;
			if( m_used != -1 )
			{
				Link& head = m_usedLinks[m_used];
				eiASSERT( head.prev == 0xFFFF );
				head.prev = (u16)index;
			}
			m_used = index;
		}
		eiDEBUG( m_dbgCount++ );
		return result;
	}
	struct Link { u16 prev, next; }; eiSTATIC_ASSERT( sizeof(Link) == sizeof(u32) );
	const uint  m_size;
	       int  m_free;
	         T* m_data;
	       int* m_offsNextFreeMinusOne;
		   //if Enumerable:
	       int  m_used;
	      Link* m_usedLinks;//todo - could just use the m_offsNextFreeMinusOne data, as it's invalid when allocated...
	eiDEBUG( uint m_dbgCount );
};

}