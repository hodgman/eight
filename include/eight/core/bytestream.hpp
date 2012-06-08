//------------------------------------------------------------------------------

template<class T> struct AssignBool { static void Do(T&,bool){} };
template<>  struct AssignBool<bool> { static void Do(bool& o,bool b){o=b;} };
template<class T,bool> struct AssignByte         { static void Do(T&  ,const T&  ){    } };
template<class T>      struct AssignByte<T,true> { static void Do(T& o,const T& i){o=i;} };

template<IoDir Mode, bool C>
void StreamIterator<Mode,C>::read( bool& d )
{
	eiSTATIC_ASSERT( Mode == Read )
	if( !C )
	{
		eiASSERT( state.it+1 <= state.end )
		d = !!(*state.it);
		++state.it;
	}
	else if( C )
		read_var_bool( d );
}

template<IoDir Mode, bool C>
void StreamIterator<Mode,C>::write( bool d )
{
	eiSTATIC_ASSERT( Mode == Write )
	if( !C )
	{
		eiASSERT( state.it+1 <= state.end )
		*state.it = ubyte(d);
		++state.it;
	}
	else if( C )
		write_var_bool( d );
}

template<IoDir Mode, bool C>
template<class T> void StreamIterator<Mode,C>::read( T& value )
{
	eiSTATIC_ASSERT( Mode == Read )
	eiSTATIC_ASSERT( !IsBoolean<T>::value )
	if( C && IsInteger<T>::value )
		ReadVarInt<T,IsInteger<T>::value>::Call( *this, d );
	else if( sizeof(T) == 1 )
	{
		eiASSERT( state.it+1 <= state.end )
		*reinterpret_cast<ubyte*>(&value) = T(*state.it);
		++state.it;
	}
	else
	{
		eiASSERT( state.it + sizeof(T) <= state.end )
		ubyte* output = reinterpret_cast<ubyte*>(&d);
		memcpy( output, state.it, size );
	}
}

template<IoDir Mode, bool C>
template<class T> void StreamIterator<Mode,C>::write( const T& d )
{
	eiSTATIC_ASSERT( Mode == Write )
	eiSTATIC_ASSERT( !IsBoolean<T>::value )

	if( C && IsInteger<T>::value )
		WriteVarInt<T,IsInteger<T>::value>::Call( *this, d );
	else if( sizeof(T) == 1 )
	{
		eiASSERT( state.it+1 <= state.end )
		*state.it = *reinterpret_cast<const ubyte*>( &d );
		++state.it;
	}
	else
	{
		eiASSERT( state.it + sizeof(T) <= state.end )
		ubyte* input = reinterpret_cast<ubyte*>(&d);
		memcpy( state.it, input, size );
	}
}


template<bool C, class T> StreamIterator<Read,C>& operator>>( StreamIterator<Read,C>& it, T& d )
{
	it.read( d );
	return *this;
}
//------------------------------------------------------------------------------
template<bool C, class T> StreamIterator<Write,C>& operator<<( StreamIterator<Write,C>& it, const T& d )
{
	it.write( d );
	return *this;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<IoDir Mode, bool C>
template<class T>
uint StreamIterator<Mode,C>::read( T* data, uint& size, uint allocated_size )
{
	uint actual_size = 0;
	read( actual_size );
	uint num_read = min( actual_size, allocated_size );
	uint remaining = actual_size - num_read;
	for( uint i=0; i<num_read; ++i )
	{
		read( data[i] );
	}
	size = num_read;
	return remaining;
}
//------------------------------------------------------------------------------
template<IoDir Mode, bool C>
template<class T>
void StreamIterator<Mode,C>::write( T* data, uint size )
{
	write( size );
	for( uint i=0; i!=size; ++i )
	{
		write( data[i] );
	}
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<IoDir Mode, bool C>
void StreamIterator<Mode,C>::read_var_bool( bool& output )
{
	eiSTATIC_ASSERT( Mode == Read )
	eiSTATIC_ASSERT( C )
	if( !m_rBools )
	{
		*this >> m_rLastBoolPack;
		output = m_rLastBoolPack & 1;
		m_rBools++;
	}
	else
	{
		ASSERT( m_rBools < 8 )
		output = !!(m_rLastBoolPack & (1 << m_rBools));
		m_rBools++;
		if( m_rBools == 8 )
			m_rBools = 0;
	}
}
//------------------------------------------------------------------------------
template<IoDir Mode, bool C>
void StreamIterator<Mode,C>::write_var_bool( bool input )
{
	eiSTATIC_ASSERT( Mode == Write )
	eiSTATIC_ASSERT( C )
	if( !m_wBools )
	{
		ubyte pack = input ? 1 : 0;
		*this << pack;
		m_wBools++;
	}
	else
	{
		ASSERT( m_wBools < 8 )
		if( input )
			m_Data.back() |= (1 << m_wBools);
		m_wBools++;
		if( m_wBools == 8 )
			m_wBools = 0;
	}
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<IoDir Mode, bool C>
template<class T> T StreamIterator<Mode,C>::read_var_int()
{
	eiSTATIC_ASSERT( Mode == Read )
	eiSTATIC_ASSERT( C )
	typedef UnsignedType<T>::type U;
	U rawOutput = 0;
	uint bitsRead = 0;
	while( true )
	{
		ubyte curByte;
		*this >> curByte;
		U decoded = (U(curByte & 0x7F) << bitsRead);
		rawOutput |= decoded;
		if( !(curByte & 0x80) )
			break;
		bitsRead += 7;
	}

	T output;
	if( IsSigned<T>::value && rawOutput )
	{
		if( rawOutput % 2 )
			output = (rawOutput+1)/2;
		else
			output = T(rawOutput / 2) * -1;
	}
	else
		output = rawOutput;
	return output;
}
//------------------------------------------------------------------------------
template<IoDir Mode, bool C>
template<class T> void StreamIterator<Mode,C>::write_var_int( T rawInput )
{
	eiSTATIC_ASSERT( Mode == Write )
	eiSTATIC_ASSERT( C )
	typedef UnsignedType<T>::type U;
	U input;
	if( IsSigned<T>::value && rawInput )
	{
		if( rawInput > 0 )
			input = (U(rawInput) * 2) - 1;
		else
			input = U(rawInput * -1) * 2;
	}
	else
		input = rawInput;

	do
	{
		ubyte curByte = input & 0x7F;
		input >>= 7;
		if( input )
			curByte |= 0x80;
		*this << curByte;
	}while( input );
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
