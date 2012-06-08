//------------------------------------------------------------------------------
template<class T, class FnKey>
T* RadixSort( T* eiRESTRICT inputBegin, T* eiRESTRICT bufferBegin, uint size, const FnKey& fnKey )
{
	typedef FnKey::Type Key;
	if( size <= 1 )
		return inputBegin;
	const uint passes = sizeof( Key );
	uint count[256];
	uint index[256];
	T* buffers[2] = { inputBegin, bufferBegin };
	for( uint pass=0; pass!=passes; ++pass )
	{
		T* out = buffers[1];
		T* begin = buffers[0];
		T* end = begin + size;
		//Build the histogram
		memset( count, 0, 256*sizeof(uint) );
		for( T* it=begin; it!=end; ++it )
		{
			Key k = fnKey(it);
			const u8& key = *(reinterpret_cast<u8*>(&k) + pass);
			++count[key];
		}
		//Build the partition lookup
		index[0] = 0;
		for( uint i=1; i!=256; ++i )
		{
			index[i] = index[i-1] + count[i-1];
		}
		//Perform the sort
		for( T* it=begin; it!=end; ++it )
		{
			Key k = fnKey(it);
			u8& key = *(reinterpret_cast<u8*>(&k) + pass);
			uint idx = index[key]++;
//			ASSERT( idx < size );
			out[idx] = *it;
		}
		//flip buffers for the next pass
		T* temp = buffers[0]; buffers[0] = buffers[1]; buffers[1] = temp;
	}
	return buffers[0];
}
//------------------------------------------------------------------------------
template<class T>
T* RadixSort( T* eiRESTRICT inputBegin, T* eiRESTRICT bufferBegin, uint size )
{
	return RadixSort( inputBegin, bufferBegin, size, DefaultRadixKey<T>() );
}
//------------------------------------------------------------------------------