//------------------------------------------------------------------------------
template<class T> inline void* Scope::New()
{
	eiASSERT( !Sealed() );
	return AllocObject<T>();
}
template<class T> inline T* Scope::New( uint count )
{
	eiASSERT( !Sealed() );
	eiASSERT( count );
	T* data = AllocObject<T>( count );
	if( data ) for( int i=0; i != count; ++i )
	{
		new(&data[i]) T;
	}
	return data;
}
template<class T> inline T* Scope::New( uint count, const T& init )
{
	eiASSERT( !Sealed() );
	eiASSERT( count );
	T* data = AllocObject<T>( count );
	if( data ) for( int i=0; i != count; ++i )
	{
		new(&data[i]) T(init);
	}
	return data;
}
template<class T> inline T* Scope::Alloc()
{
	eiASSERT( !Sealed() );
	alloc.Align(alignment);
	return alloc.Alloc<T>();
}
template<class T> inline T* Scope::Alloc( uint count )
{
	eiASSERT( !Sealed() );
	eiASSERT( count );
	T* data = 0;
	if( alloc.Align(alignment) )
		data = alloc.Alloc<T>( count );
	return data;
}
/*
template<class T> inline T* Scope::Alloc( uint count, const T& init )
{
	eiASSERT( !Sealed() );
	eiASSERT( count );
	alloc.Align(alignment);
	T* data = 0;
	if( alloc.Align(alignment) )
	{
		data = alloc.Alloc<T>( count );
		for( int i=0; i != count; ++i )
		{
			new(&data[i]) T(init);
		}
	}
	return data;
}*/
template<class T> inline void Scope::DestructorCallback(void* p)
{
	((T*)p)->~T();
}
template<class T> inline T* Scope::AllocObject()
{
	FnDestructor* fn = &DestructorCallback<T>;
	return (T*)AllocObject( sizeof(T), HasDestructor<T>::value ? fn : 0 );
}
template<class T> inline T* Scope::AllocObject( uint count )
{
	FnDestructor* fn = &DestructorCallback<T>;
	return (T*)AllocObject( count, sizeof(T), HasDestructor<T>::value ? fn : 0 );
}
//------------------------------------------------------------------------------