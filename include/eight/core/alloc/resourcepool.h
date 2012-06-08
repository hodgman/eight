//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
namespace eight {
//------------------------------------------------------------------------------

eiInfoGroup( ResourcePool, true );
//WIP!
template<class T> class ResourcePool : NonCopyable
{
	struct Node
	{
		T data;
		int offsNextFreeMinusOne;
	};
	const static bool s_NonPod = true;
	eiDEBUG( const static bool dbgNoMemcmp = false );
public:
	ResourcePool( Scope& a, uint size ) : m_size(size), m_free(0), m_array(eiAllocArray(a,Node,size))
	{
		memset( m_array, 0, sizeof(Node)*size );
		m_array[size-1].offsNextFreeMinusOne = -1;
	}
	~ResourcePool()
	{
		if( s_NonPod )
		{
			eiDEBUG( int endNodes = 0 );
			for( uint i=0; i!=m_size; ++i )
			{
				// either in the free list (i.e. not allocated) OR contains data, but not both.
				eiDEBUG( u8 null[sizeof(T)]= {}; );
				eiDEBUG( bool containsData = memcmp(&m_array[i].data, null, sizeof(T)) != 0 );//not valid test in general case
				eiDEBUG( bool inFreeList = m_array[i].offsNextFreeMinusOne != -1 );
				eiDEBUG( endNodes += !(inFreeList ^ containsData) );
				if( m_array[i].offsNextFreeMinusOne == -1 )//not in free list
				{
					m_array[i].data.~T();
				}
			}
			eiASSERT( dbgNoMemcmp || endNodes == 1 );
		}
	}
	T& operator[](uint i)//todo -make these 0-based
	{
		eiASSERT( i && i-1 < m_size );
		Node& node = m_array[i-1];
		eiASSERT( node.offsNextFreeMinusOne == -1 );
		eiDEBUG( u8 null[sizeof(T)]= {}; );
		eiASSERT( dbgNoMemcmp || memcmp(&node.data, null, sizeof(T))!=0 );
		return node.data;
	}
	uint Index(T* object)//starting from 1, 0 is invalid
	{
		eiASSERT( (Node*)object >= m_array && (Node*)object < m_array+m_size );
		int index = ((Node*)object)-m_array;
		return (uint)index+1;
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
		int index = ((Node*)object)-m_array;
		eiASSERT( index >= 0 && (uint)index < m_size );
		Node& node = m_array[index];
		if( m_free < 0 )
			node.offsNextFreeMinusOne = -1;
		else
		{
			int offsNextFree = m_free - index;
			node.offsNextFreeMinusOne = offsNextFree - 1;
		}
		if( s_NonPod )
			object->~T();
		eiDEBUG( memset(object, 0, sizeof(T)) );
		m_free = index;
	}
private:
	T* Alloc_()
	{
		int index = m_free;
		if( index < 0 )
			return 0;
		Node& freeNode = m_array[index];
		int next = index + freeNode.offsNextFreeMinusOne + 1;
		if( next == index )
		{
			next = -1;
			eiInfo( ResourcePool, "Pool filled" );
		}
		m_free = next;
		freeNode.offsNextFreeMinusOne = -1;
		eiASSERT( (void*)&freeNode.data == (void*)&freeNode );
		return &freeNode.data;
	}
	uint  m_size;
	 int  m_free;
	Node* m_array;
};

}