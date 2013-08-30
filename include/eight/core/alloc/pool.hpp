
template<bool Enumerable>
PoolBase<Enumerable>::PoolBase( Scope& a, uint size ) : m_size(size), m_free(0), m_offsNextFreeMinusOne(eiAllocArray(a,int,size)), m_used(-1), m_usedLinks(Enumerable ? eiAllocArray(a,Link,size) : 0)
{
	eiASSERT( !Enumerable || size < 0xFFFF );
	eiDEBUG( if( Enumerable ) memset( m_usedLinks, 0xFFFFFFFF, sizeof(Link)*size ) );
	memset( m_offsNextFreeMinusOne, 0, sizeof(int)*size );
	m_offsNextFreeMinusOne[size-1] = -1;
	eiDEBUG( m_dbgCount = 0 );
}

/*PoolBase<Enumerable>::~PoolBase()
{
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
	}
}*/

template<bool Enumerable>
void PoolBase<Enumerable>::Dbg_ValidateIndex(uint i)
{
	eiASSERT( i  < m_size );
	eiASSERT( m_offsNextFreeMinusOne[i] == -1 );
//	eiASSERT( !Enumerable || m_used==i || (m_usedLinks[i].prev != 0xFFFF && m_usedLinks[i].next != 0xFFFF) );
	eiASSERT( !Enumerable || m_used==i || (m_usedLinks[i].prev != 0xFFFF || m_usedLinks[i].next != 0xFFFF) );
}

template<bool Enumerable>
int PoolBase<Enumerable>::Alloc()
{
	int index = m_free;
	if( index < 0 )
	{
		eiInfo( Pool, "Alloc fail - Pool full" );
		return -1;
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
	return index;
}

template<bool Enumerable>
void PoolBase<Enumerable>::Release( int index )
{
	eiASSERT( index >= 0 && (uint)index < m_size );
	if( m_free < 0 )
		m_offsNextFreeMinusOne[index] = -1;
	else
	{
		int offsNextFree = m_free - index;
		m_offsNextFreeMinusOne[index] = offsNextFree - 1;
	}
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
	m_free = index;
	eiDEBUG( m_dbgCount-- );
}

template<bool Enumerable>
bool PoolBase<Enumerable>::Empty() const
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

template<bool Enumerable>
int PoolBase<Enumerable>::Begin()
{
	eiSTATIC_ASSERT(Enumerable);
	return m_used;
}

template<bool Enumerable>
int PoolBase<Enumerable>::Next(uint index)
{
	eiSTATIC_ASSERT(Enumerable);
	uint next = m_usedLinks[index].next;
	eiASSERT( next == 0xFFFF || next < m_size );
	return next == 0xFFFF ? -1 : next;
}

//------------------------------------------------------------------------------

template<class T, bool E>
Pool<T,E>::Pool( Scope& a, uint size ) : m_pool(a, size), m_data(eiAllocArray(a,T,size))
{
}

template<class T, bool E>
T& Pool<T,E>::operator[](uint i)
{
	eiDEBUG( m_pool.Dbg_ValidateIndex(i) );
	T& data = m_data[i];
	eiDEBUG( u8 null[sizeof(T)]= {}; );
	eiASSERT( dbgNoMemcmp || memcmp(&data, null, sizeof(T))!=0 );
	return data;
}

template<class T, bool E>
uint Pool<T,E>::Index(const T& object)
{
	eiASSERT( &object >= m_data && &object < m_data+m_pool.Capactiy() );
	int index = &object-m_data;
	return (uint)index;
}

template<class T, bool E>
T* Pool<T,E>::Alloc(const T& data)
{
	int index = m_pool.Alloc();
	if( index < 0 )
		return 0;
	T* t = &m_data[index];
	eiDEBUG( u8 null[sizeof(T)]= {}; );
	eiASSERT( dbgNoMemcmp || memcmp(t, null, sizeof(T))==0 );
	if( s_NonPod )
		new(t) T(data);
	else
		*t = data;
	eiASSERT( dbgNoMemcmp || memcmp(t, null, sizeof(T))!=0 );
	return t;
}

template<class T, bool E>
T* Pool<T,E>::Alloc()
{
	int index = m_pool.Alloc();
	if( index < 0 )
		return 0;
	T* t = &m_data[index];
	return s_NonPod ? new(t) T : t;
}

template<class T, bool E>
void Pool<T,E>::Release( T* object )
{
	int index = object-m_data;
	m_pool.Release( index );
	if( s_NonPod )
		object->~T();
	eiDEBUG( if(!dbgNoMemcmp) memset(object, 0, sizeof(T)) );
}

template<class T, bool E>
T* Pool<T,E>::Begin()
{
	int begin = m_pool.Begin();
	return begin < 0 ? 0 : &m_data[begin];
}

template<class T, bool E>
T* Pool<T,E>::Next(const T& object)
{
	uint index = Index( object );
	int next = m_pool.Next( index );
	return next < 0 ? 0 : &m_data[next];
}

