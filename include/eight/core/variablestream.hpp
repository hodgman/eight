//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline CVariableStream::CVariableStream( CByteStream& data ) : m_Data(data)
{
	DEBUG( m_NextId = 0; )
		//TODO - add debug checks to ensure that all read/writes are surrounded by group start/end calls
}
//------------------------------------------------------------------------------
inline void CVariableStream::ReadGroupStart()
{
	ASSERT( m_NextId == 0 )
	m_NextId = sm_NotYetRead;
}
//------------------------------------------------------------------------------
inline void CVariableStream::ReadGroupEnd()
{
	//if the termination marker hasn't yet been read, make sure to remove it from the stream now
	if( m_NextId != 0 )
	{
		m_Data >> m_NextId;
		ASSERT( m_NextId == 0 )
	}
}
//------------------------------------------------------------------------------
inline void CVariableStream::WriteGroupStart()
{
}
//------------------------------------------------------------------------------
inline void CVariableStream::WriteGroupEnd()
{
	//put a termination marker into the stream
	m_Data << uint(0);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<class T> bool CVariableStream::Read( uint id, T& d )
{
	if( m_NextId == sm_NotYetRead )
		m_Data >> m_NextId;
	ASSERT( m_NextId == 0 || m_NextId >= id+1 )
	if( m_NextId == id+1 )
	{
		m_Data >> d;
		m_NextId = sm_NotYetRead;
		return true;
	}
	else
		return false;
}
//------------------------------------------------------------------------------
template<class T> void CVariableStream::Write( uint id, const T& d )
{
	m_Data << id+1;
	m_Data << d;
}
//------------------------------------------------------------------------------
template<class T> void CVariableStream::NullWrite( uint id )
{
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
