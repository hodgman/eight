#include <eight/core/alloc/stack.h>
#include <eight/core/alloc/scope.h>

using namespace eight;
Scope::Scope( StackAlloc& a, const char* n ) : alloc(a),       parent(a.Owner()), base(alloc.Mark()), destructList(), name(n) { alloc.TransferOwnership(parent,this); }
Scope::Scope( Scope& p, const char* n ) : alloc(p.alloc), parent(&p),        base(alloc.Mark()), destructList(), name(n) { alloc.TransferOwnership(parent,this); }
Scope::~Scope()
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
const u8* Scope::Tell() const
{
	return alloc.Mark();
}
void Scope::Unwind()
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

void Scope::Seal()
{
	eiASSERT( !Sealed() );
	alloc.TransferOwnership(this, parent);
}
bool Scope::Sealed() const
{
	return alloc.Owner()!=this;
}

u8* Scope::Alloc( uint size, uint align )
{
	eiASSERT( !Sealed() );
	u8* data = 0;
	align = align ? align : alignment;
	if( alloc.Align(align) )
		data = alloc.Alloc( size );
	return data;
}

bool Scope::OnUnwind( void* object, FnDestructor* fn )
{
	eiASSERT( fn );
	const u8* failMark = alloc.Mark();
	bool ok = alloc.Align(alignment);
	Destructor* d = alloc.Alloc<Destructor>();
	ok &= !!d;
	if( ok )
	{
		AddDestructor( d, (u8*)object, fn );
		return true;
	}
	alloc.Unwind(failMark);
	return false;
}

u8* Scope::AllocObject( uint size, FnDestructor* fn )
{
	eiASSERT( alloc.Owner() == this );
	const u8* failMark = alloc.Mark();
	Destructor* d = 0;

	bool alignOk = alloc.Align(alignment);
	if( fn )
	{
		d = alloc.Alloc<Destructor>();
		alignOk &= alloc.Align(alignment) & !!d;
	}
	u8* object = alloc.Alloc( size );
	if( eiLikely(alignOk && object) )
	{
		eiASSERT( !!fn == !!d );
		if( fn )
			AddDestructor( d, object, fn );
		return object;
	}
	alloc.Unwind(failMark);
	return 0;
}

u8* Scope::AllocObject( uint count, uint size, FnDestructor* fn )
{
	eiASSERT( alloc.Owner() == this );
	const u8* failMark = alloc.Mark();
	bool alignOk = alloc.Align(alignment);
	Destructor* destructors = 0;
	if( fn )
	{
		destructors = alloc.Alloc<Destructor>( count );
		alignOk &= alloc.Align(alignment);
		if( !destructors )
			goto error;
	}
	u8* objects = alloc.Alloc( count * size );
	if( !objects || !alignOk )
		goto error;

	eiASSERT( !!destructors == !!fn );
	if( fn )
		AddDestructors( destructors, objects, count, size, fn );

	return objects;
error:
	alloc.Unwind(failMark);
	return 0;
}

void Scope::AddDestructor( Destructor* d, u8* object, FnDestructor* fn )
{
	d->fn = fn;
	d->object = object;
	d->next = destructList;
	destructList = d;
}

void Scope::AddDestructors( Destructor* destructors, u8* objects, uint count, uint size, FnDestructor* fn )
{
	Destructor* head = destructList;
	for( int i=0; i != count; ++i )
	{
		destructors[i].fn = fn;
		destructors[i].object = objects;
		destructors[i].next = head;
		head = destructors + i;
		objects += size;
	}
	destructList = head;
}