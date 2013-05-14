#include <eight/core/types.h>
#include <eight/core/debug.h>
#include <eight/core/test.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/thread/pool.h>
#include <eight/core/thread/fifo_mpmc.h>
#include <eight/core/thread/futex.h>
#include <eight/core/thread/threadlocal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <eight/core/macro.h>
#include <eight/core/sort/radix.h>
#include <iostream>
#include <vector>
#include <stdlib.h>

namespace eight { namespace internal { void OnThreadExitCleanup(); } }

using namespace eight;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void* eight::Malloc( uint size )//TODO - make hookable
{
	return malloc( size );
}
void eight::Free( void* p )
{
	free( p );
}
void* eight::AlignedMalloc( uint size, uint align )
{
	return _aligned_malloc( size, align );
}
void eight::AlignedFree( void* p )
{
	_aligned_free( p );
}


template<bool> struct IndexMeta       { typedef uint32 Type; const static int size = 32; };
template<    > struct IndexMeta<true> { typedef uint64 Type; const static int size = 64; };
template<int bits>
class Index
{
	typedef IndexMeta<(bits>=32)> Meta;
	typedef typename Meta::Type Type;
	struct Data
	{
		const static int ababits = (bits==Meta::size) ? Meta::size : Meta::size-bits;
		Type value :    bits;
		Type ababa : ababits;
	} data;
	eiSTATIC_ASSERT( sizeof(Data) == sizeof(Type) );
};

void TODO_unit_test()
{
	eiSTATIC_ASSERT( sizeof( Index< 1> ) == 4 );
	eiSTATIC_ASSERT( sizeof( Index<15> ) == 4 );
	eiSTATIC_ASSERT( sizeof( Index<31> ) == 4 );
	eiSTATIC_ASSERT( sizeof( Index<32> ) == 8 );
	eiSTATIC_ASSERT( sizeof( Index<33> ) == 8 );
}


bool eight::Assert( const char* e, int l, const char* f, const char* fn )
{
	if( InTest() )
	{
		TestFailure( f, l, fn, "Assertion failure - %s", e );
	}
	else
	{
		//TODO - dialog box
	}
	return IsDebuggerAttached();
}

static Futex futex;
void eight::Print( const char* msg )
{
	printf(msg);
}
void eight::Printf( const char* fmt, ... )
{
	va_list	v;
	va_start(v, fmt);
	vprintf(fmt, v);
	va_end(v);
}

struct DebugOutput_Tag{};
struct DebugOutput_Buffer{ char buffer[2048]; };
static ThreadLocalStatic<DebugOutput_Buffer, DebugOutput_Tag> s_tlsBuffers;

void eight::internal::OnThreadExitCleanup()
{
	DebugOutput_Buffer* tlBuffer = s_tlsBuffers;
	if( tlBuffer )
	{
		s_tlsBuffers = (DebugOutput_Buffer*)0;
		delete tlBuffer;//todo - replace new/delete with hookable general allocator
	}
}

void eight::DebugOutput( const char* prefix, const char* fmt, ... )
{
	DebugOutput_Buffer* tlBuffer = s_tlsBuffers;
	if( !tlBuffer )
		s_tlsBuffers = tlBuffer = new DebugOutput_Buffer;//todo - replace new/delete with hookable general allocator
	char* buffer = tlBuffer->buffer;
	int len = strlen(prefix);
	memcpy(buffer, prefix, len);
	va_list	v;
	va_start(v, fmt);
	len += vsprintf(&buffer[len], fmt, v);
	va_end(v);
	buffer[len] = '\n';
	buffer[len+1] = '\0';
	eiASSERT( len+2 <= 2048 );
	eight::Print(buffer);
}

void eight::DebugOutput( int line, const char* file, const char* function, const char* fmt, ... )
{
	va_list	args;
	va_start(args, fmt);
	DebugOutput( line, file, function, fmt, args );
	va_end(args);
}
void eight::DebugOutput( int line, const char* file, const char* function, const char* fmt, va_list& args )
{
	DebugOutput_Buffer* tlBuffer = s_tlsBuffers;
	if( !tlBuffer )
		s_tlsBuffers = tlBuffer = new DebugOutput_Buffer;//todo - replace new/delete with hookable general allocator
	char* buffer = tlBuffer->buffer;
	int len = sprintf(buffer, "%s(%d) : %s : ", file, line, function );
	len += vsprintf(&buffer[len], fmt, args);
	buffer[len] = '\n';
	buffer[len+1] = '\0';
	eiASSERT( len+2 <= 2048 );
	eight::Print(buffer);
}
