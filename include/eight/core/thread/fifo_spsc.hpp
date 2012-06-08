
template<class T>
FifoSpsc<T>::FifoSpsc( Scope& a, uint size ) : m_data(eiNewArray(a,T,size)), m_size(size)
{
	eiASSERT( size > 1 );
}

template<class T>
u32 FifoSpsc<T>::Next( u32 index )
{
	++index;
	return index >= m_size ? 0 : index;
}

template<class T>
bool FifoSpsc<T>::Push( const T& value )
{
	u32 read = m_read;
	u32 write = m_write;
	u32 nextWrite = Next(write);
	if( nextWrite == read )
		return false;
	m_data[write] = value;
	m_write = nextWrite;
	return true;
}

template<class T>
bool FifoSpsc<T>::Pop( T& value )
{
	u32 read = m_read;
	u32 write = m_write;
	if( read == write )
		return false;
	value = m_data[read];
	m_read = Next( read );
	return true;
}
