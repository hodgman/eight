//------------------------------------------------------------------------------

inline Scope& Scope::DestructorHelper::Owner() const
{
	eiSTATIC_ASSERT( (eiOffsetOf(Scope,OnConstructed)) == 0 );
	return *(Scope*)this;
}
template<class T> void* Scope::New()
{
	void* newObject = Alloc<T>();
	if( IsSameType<T,Scope>::value )
		OnUnwind(newObject, &DestructorCallback<T>, sizeof(T));
	return newObject;
}
template<class T> T* Scope::New( uint count )
{
	eiASSERT( !Sealed() );
	if( eiUnlikely(!count) )
		return 0;
	T* result = Alloc<T>( count );
	for( int i=0; i != count; ++i )
	{
		new(&result[i]) T;
	}
	if( HasDestructor<T>::value )
	{
		if( count > 1 )
		{
			ArrayDestructor* d = Alloc<ArrayDestructor>();
			d->count = count;
			d->p = result;
			OnUnwind( d, &ArrayDestructor::Callback<T>, sizeof(T)*count );
		}
		else
			OnUnwind( result, &DestructorCallback<T>, sizeof(T) );
	}
	return result;
}
template<class T> T* Scope::New( uint count, const T& init )
{
	eiASSERT( !Sealed() );
	if( eiUnlikely(!count) )
		return 0;
	T* result = Alloc<T>( count );
	for( int i=0; i != count; ++i )
	{
		new(&result[i]) T(init);
	}
	if( HasDestructor<T>::value )
	{
		if( count > 1 )
		{
			ArrayDestructor* d = Alloc<ArrayDestructor>();
			d->count = count;
			d->p = result;
			OnUnwind( d, &ArrayDestructor::Callback<T>, sizeof(T)*count );
		}
		else
			OnUnwind( result, &DestructorCallback<T>, sizeof(T) );
	}
	return result;
}
template<class T> T* Scope::Alloc()
{
	eiASSERT( !Sealed() );
	for(;;) 
	{
		if( alloc->Align(eiAlignOf(T)) )
		{
			T* data = alloc->Alloc<T>();
			if( data )
				return data;
		}
		OnOutOfMemory( eiAlignOf(T) + sizeof(T) );
	}
}
template<class T> T* Scope::Alloc( uint count )
{
	eiASSERT( !Sealed() );
	if( eiUnlikely(!count) )
		return 0;
	for(;;) 
	{
		if( alloc->Align(eiAlignOf(T)) )
		{
			T* data = alloc->Alloc<T>( count );
			if( data )
				return data;
		}
		OnOutOfMemory( eiAlignOf(T) + sizeof(T) * count );
	}
}
template<class T> void Scope::ArrayDestructor::Callback(void* a)
{
	eiASSERT(a);
	ArrayDestructor* ad = (ArrayDestructor*)a;
	eiASSERT(ad->p);
	for( T* t = (T*)ad->p, *end=t + ad->count; t!=end; ++t )
	{
		t->~T();
	}
}
template<class T> void Scope::DestructorCallback(void* p)
{
	eiASSERT(p);
	((T*)p)->~T();
}
//------------------------------------------------------------------------------