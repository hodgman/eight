
template<class T>
FifoMpmc<T>::FifoMpmc( Scope& a, uint size ) : m_data(eiNewArray(a,Node,size)), m_size(size)
{
	eiASSERT( m_data );
	eiASSERT( size > 1 );
}

template<class T>
bool FifoMpmc<T>::Empty() const
{
	return m_read == m_write;
}

template<class T>
bool FifoMpmc<T>::Push( const T& value )
{
	while(1)
	{
		u32 read, write, nextWrite, nextWriteIndex, readIndex, writeIndex, writePass;
		{
			read = m_read;
			write = m_write;
			nextWrite = write+1;

			readIndex = read % m_size;
			writeIndex = write % m_size;
			writePass = write / m_size;
			nextWriteIndex = nextWrite % m_size;

			if( nextWrite-read >= m_size )
				return false;//queue full
		}

		Node& node = m_data[writeIndex];

		u32 state = node.state;
		u32 expectedAba = writePass << abaShift;
		u32 expectedFree = expectedAba|freeForWrite;
		u32 newAllocState = expectedAba|allocForWrite;
		if( state==expectedFree && node.state.SetIfEqual(newAllocState, expectedFree) )
		{
			m_write = nextWrite;
			node.data = value;
			u32 newFreeState = expectedAba|freeForRead;
			node.state = newFreeState;
			return true;
		}
		YieldHardThread();
	}
}

template<class T>
bool FifoMpmc<T>::Pop( T& value )
{
	while(1)
	{
		u32 read, write, nextRead, readIndex, writeIndex, readPass;
		{
			read = m_read;
			write = m_write;

			readIndex = read % m_size;
			readPass = read / m_size;
			writeIndex = write % m_size;
			nextRead = read + 1;

			if( read == write )
				return false;//queue empty
		}

		Node& node = m_data[readIndex];

		u32 state = node.state;
		u32 expectedAba = readPass << abaShift;
		u32 expectedFree = expectedAba|freeForRead;
		u32 newAllocState = expectedAba|allocForRead;
		if( state==expectedFree && node.state.SetIfEqual(newAllocState, expectedFree) )
		{
			m_read = nextRead;

			value = node.data;
			u32 nextWriteAba = (readPass+1)<<abaShift;
			u32 newFreeState = nextWriteAba|freeForWrite;

			node.state = newFreeState;
			return true;
		}
		YieldHardThread();
	}
}
