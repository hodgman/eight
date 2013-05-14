
template<class T>
CompactPool<T>::CompactPool( Scope& a, uint size )
	: m_pool(a, size)
	, m_map(a, size)
	, m_data(eiAllocArray(a,T,size))
{
}

template<class T>
CompactPool<T>::~CompactPool()
{
	if( s_NonPod )
	{
		for( uint i=0, end=Size(); i!=end; ++i )
			m_data[i].~T();
	}
}

template<class T>
T& CompactPool<T>::operator[]( int handle )
{
	eiDEBUG( m_pool.Dbg_ValidateIndex(handle) );
	uint physical = m_map.ToPhysical(handle);
	T& data = m_data[physical];
	return data;
}

template<class T>
int CompactPool<T>::Handle(const T& object)
{
	eiASSERT( &object >= m_data && &object < m_data+m_pool.Size() );
	int physical = &object-m_data;
	return m_map.ToHandle(physical);
}

template<class T>
int CompactPool<T>::Alloc(const T& data)
{
	int handle = m_pool.Alloc();
	if( handle < 0 )
		return -1;
	uint physical = m_map.AllocHandleToPhysical(handle);
	T* t = &m_data[physical];
	if( s_NonPod )
		new(t) T(data);
	else
		*t = data;
	return handle;
}

template<class T>
int CompactPool<T>::Alloc()
{
	int handle = m_pool.Alloc();
	if( handle < 0 )
		return -1;
	uint physical = m_map.AllocHandleToPhysical(handle);
	if( s_NonPod )
	{
		T* t = &m_data[physical];
		new(t) T;
	}
	return handle;
}

template<class T>
void CompactPool<T>::Release( int handle )
{
	eiDEBUG( m_pool.Dbg_ValidateIndex(handle) );
	m_pool.Release( handle );
	HandleMap::MoveCmd move = m_map.EraseHandle( handle );
	if( s_NonPod )
	{
		m_data[move.to].~T();
		if( move.from != move.to )
		{
			new(&m_data[move.to])( m_data[move.from] );
			m_data[move.from].~T();
		}
	}
	else
		m_data[move.to] = m_data[move.from];
}

template<class T>
void CompactPool<T>::Clear()
{
	if( s_NonPod )
	{
		for( uint i=0, end=Size(); i!=end; ++i )
			m_data[i].~T();
	}
	m_map.Clear(Capactiy());
	m_pool.Clear();
}
