//------------------------------------------------------------------------------
inline Scope::Scope( StackAlloc& a, const char* n ) : alloc(a),       parent(a.Owner()), base(alloc.Mark()), destructList(), name(n) { alloc.TransferOwnership(parent,this); }
inline Scope::Scope( Scope& p, const char* n ) : alloc(p.alloc), parent(&p),        base(alloc.Mark()), destructList(), name(n) { alloc.TransferOwnership(parent,this); }
inline Scope::~Scope()
{
	Destructor* list = destructList;
	while(list)
	{
		list->fn(list->object);
		list = list->next;
	}
	if(alloc.Owner()==this)
	{
		alloc.Unwind(base);
		alloc.TransferOwnership(this, parent);
	}
}
inline const u8* Scope::Tell() const
{
	return alloc.Mark();
}
inline void Scope::Unwind()
{
	eiASSERT( alloc.Owner()==this );
	Destructor* list = destructList;
	while(list)
	{
		list->fn(list->object);
		list = list->next;
	}
	destructList = 0;
	alloc.Unwind(base);
}

inline void Scope::Seal()
{
	eiASSERT( !Sealed() );
	alloc.TransferOwnership(this, parent);
}
inline bool Scope::Sealed() const
{
	return alloc.Owner()!=this;
}

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
}
inline u8* Scope::Alloc( uint size, uint align )
{
	eiASSERT( !Sealed() );
	u8* data = 0;
	align = align ? align : alignment;
	if( alloc.Align(align) )
		data = alloc.Alloc( size );
	return data;
}
template<class T> inline void Scope::DestructorCallback(void* p)
{
	((T*)p)->~T();
}
template<class T> inline T* Scope::AllocObject()
{
	eiASSERT( alloc.Owner() == this );
	bool alignOk = true;
	const u8* failMark = alloc.Mark();
	Destructor* d = 0;

	alignOk &= alloc.Align(alignment);
	if( HasDestructor<T>::value )
	{
		d = alloc.Alloc<Destructor>();
		alignOk &= alloc.Align(alignment);
	}
	T* object = alloc.Alloc<T>();
	if( eiLikely(alignOk && object) )
	{
		if( HasDestructor<T>::value && d )
		{
			d->fn = &DestructorCallback<T>;
			d->object = object;
			d->next = destructList;
			destructList = d;
		}
		else
		{
			eiASSERT( !HasDestructor<T>::value );
		}
		return object;
	}
	else
	{
		alloc.Unwind(failMark);
		return 0;
	}
}
template<class T> inline T* Scope::AllocObject( uint count )
{
	eiASSERT( alloc.Owner() == this );
	bool alignOk = true;
	const u8* failMark = alloc.Mark();
	Destructor* destructors = 0;

	alignOk &= alloc.Align(alignment);
	if( HasDestructor<T>::value )
	{
		Destructor* destructors = alloc.Alloc<Destructor>( count );
		alignOk &= alloc.Align(alignment);
		if( !destructors )
		{
			alloc.Unwind(failMark);
			return 0;
		}
	}
	T* objects = alloc.Alloc<T>( count );
	if( !objects || !alignOk )
	{
		alloc.Unwind(failMark);
		return 0;
	}

	eiASSERT( !destructors || HasDestructor<T>::value );
	if( destructors )
	{
		Destructor* head = destructList;
		for( int i=0; i != count; ++i )
		{
			T* object = &objects[i];
			destructors[i].fn = &DestructorCallback<T>;
			destructors[i].object = object;
			destructors[i].next = head;
			head = &destructors[i];
		}
		destructList = head;
	}

	return objects;
}
//------------------------------------------------------------------------------