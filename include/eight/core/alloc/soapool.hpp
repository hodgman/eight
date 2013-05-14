
template<class T>
CompactSoaPool<T>::CompactSoaPool(Scope& a, uint size)
: m_pool(a,size)
, m_map(a,size)
, m_data(a,size)
{
}

template<class T>
typename CompactSoaPool<T>::Proxy CompactSoaPool<T>::operator[](uint handle)
{
	eiDEBUG( m_pool.Dbg_ValidateIndex(handle) );
	uint physical = m_map.ToPhysical(handle);
	return m_data[physical];
}

template<class T>
void CompactSoaPool<T>::Set( uint handle, const T& tuple )
{
	eiDEBUG( m_pool.Dbg_ValidateIndex(handle) );
	uint physical = m_map.ToPhysical(handle);
	m_data.Set( physical, tuple );
}

template<class T>
void CompactSoaPool<T>::Get( uint handle, T& tuple ) const
{
	eiDEBUG( m_pool.Dbg_ValidateIndex(handle) );
	uint physical = m_map.ToPhysical(handle);
	m_data.Get( physical, tuple );
}

template<class T>
int CompactSoaPool<T>::Alloc(const T& tuple)
{
	int handle = m_pool.Alloc();
	if( handle < 0 )
		return -1;
	uint physical = m_map.AllocHandleToPhysical(handle);
	m_data.Set( physical, tuple );
	return handle;
}

template<class T>
int CompactSoaPool<T>::Alloc()
{
	int handle = m_pool.Alloc();
	if( handle < 0 )
		return -1;
	m_map.AllocHandleToPhysical(handle);
	return handle;
}

template<class T>
void CompactSoaPool<T>::Release( int handle )
{
	eiDEBUG( m_pool.Dbg_ValidateIndex(handle) );
	m_pool.Release( handle );
	HandleMap::MoveCmd move = m_map.EraseHandle( handle );
	if( move.to != move.from )
	{
		T temp;
		m_data.Get( move.from, temp );
		m_data.Set( move.to, temp );
		eiDEBUG( memset(&temp, 0xfefefefe, sizeof(T)) );
		eiDEBUG( m_data.Set( move.from, temp ) );
	}
}

template<class T>
void CompactSoaPool<T>::Clear()
{
	m_map.Clear(Capactiy());
	m_pool.Clear();
}
