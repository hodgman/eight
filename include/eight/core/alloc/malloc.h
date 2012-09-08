//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/stack.h>

namespace eight {
//------------------------------------------------------------------------------

void* Malloc( uint size );
void  Free( void* );

class ScopedMalloc : NonCopyable
{
public:
	ScopedMalloc( uint size ) : buffer( Malloc(size) ) {}
	~ScopedMalloc() { Free( buffer ); }
	operator void*() { return buffer; }
private:
	void* buffer;
};

class MallocStack : NonCopyable
{
public:
	MallocStack( uint size ) : stack( Malloc(size), size ) {}
	~MallocStack() { Free( (void*)stack.Begin() ); }
	StackAlloc stack;
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
