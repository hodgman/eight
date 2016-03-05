//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/stack.h>

namespace eight {
//------------------------------------------------------------------------------
void* Malloc( size_t size );
template<class T> T* Malloc() { return (T*)Malloc(sizeof(T)); }
void  Free( void* );
void* AlignedMalloc( size_t size, uint align );
void  AlignedFree( void* );

void* VramAlloc( size_t size, uint align );
void  VramFree( void* );

#define eiSAFE_FREE( x ) {if(x){Free(x);x=0;}
#define eiSAFE_VRAM_FREE( x ) {if(x){VramFree(x);x=0;}


#define eiMEMCPY memcpy
#define eiMEMCMP memcmp
#define eiMEMSET memset


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
