#include <eight/core/alloc/stack.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/profiler.h>

using namespace eight;

const ei_GlobalHeap::PassThru ei_GlobalHeap::OnConstructed;

Scope::Scope( StackAlloc& a, const char* n ) : originalAlloc(a),        alloc(&a),      parent(a.Owner()), base(alloc->Mark()), destructList(), name(n), overflowChain(), capacity(a.Capacity()) { alloc->TransferOwnership(parent,this); }
Scope::Scope( Scope& p,      const char* n ) : originalAlloc(*p.alloc), alloc(p.alloc), parent(&p),        base(alloc->Mark()), destructList(), name(n), overflowChain(), capacity(p.capacity  ) { alloc->TransferOwnership(parent,this); }
Scope::~Scope()
{
	Destructor* list = destructList;
	while(list)
	{
		list->fn(list->object);
		list = list->next;
	}
	if(!Sealed())
	{
		originalAlloc.Unwind(base);
		originalAlloc.TransferOwnership(this, parent);
	}
	FreeOverflowChain();
}
void Scope::Unwind()
{
	eiProfile("Scope::Unwind");
	eiASSERT( alloc->Owner() == this );
	Destructor* list = destructList;
	while(list)
	{
		list->fn(list->object);
		list = list->next;
	}
	destructList = 0;
	originalAlloc.Unwind(base);
	alloc = &originalAlloc;
	FreeOverflowChain();
	overflowChain = 0;
}
void Scope::FreeOverflowChain()
{
	OverflowNode* n = overflowChain;
	while( n )
	{
		OverflowNode* f = n;
		n = n->next;
		eiFree(GlobalHeap,f->heap);
		eiDelete(GlobalHeap,f);
	}
}

void Scope::OnOutOfMemory( uint required )
{
	uint size = capacity;//grow by 2x
	size = size > required ? size : required;//or if that's not big enough, grow by the current request
	capacity += size;
	eiASSERT( capacity >= size );
	u8* heap = eiAllocArray(GlobalHeap, u8, size);
	overflowChain = eiNew(GlobalHeap,OverflowNode)( overflowChain, heap, size );
	alloc = &overflowChain->alloc;
	alloc->TransferOwnership(0,this);
}

void Scope::Seal()
{
	eiASSERT( !Sealed() );
	alloc->TransferOwnership(this, parent);
}
bool Scope::Sealed() const
{
	return alloc->Owner() != this;
}

u8* Scope::Alloc( uint size, uint align )
{
	eiASSERT( !Sealed() );
	align = align ? align : 16;
	for(;;) 
	{
		if( alloc->Align(align) )
		{
			u8* data = alloc->Alloc(size);
			if( data )
				return data;
		}
		OnOutOfMemory( align + size );
	}
}

void Scope::OnUnwind( void* object, FnDestructor* fn )
{
	eiASSERT( fn );
	AddDestructor( Alloc<Destructor>(), (u8*)object, fn );
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
	eiASSERT(fn);
	eiASSERT(destructors);
	eiASSERT(objects);
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
