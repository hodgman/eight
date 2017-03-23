#include <eight/core/types.h>
#include <eight/core/debug.h>
#include <eight/core/test.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/thread/pool.h>
#include <eight/core/thread/fifo_mpmc.h>
#include <eight/core/thread/futex.h>
#include <eight/core/thread/threadlocal.h>
#include <eight/core/math/arithmetic.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <eight/core/macro.h>
#include <eight/core/sort/radix.h>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <rdestl/list.h>
#include <rdestl/rde_string.h>

//TODO
#include <eight/core/os/win32.h>

bool eight::MessageBoxRetry( const char* text, const char* title )
{
	return ::MessageBoxA(0, text, title, MB_RETRYCANCEL|MB_ICONWARNING) != IDCANCEL;
}

namespace eight { namespace internal { void OnThreadExitCleanup(); } }

using namespace eight;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#if defined(eiBUILD_WINDOWS)
void* eight::Malloc( size_t size )//TODO - make hookable?
{
	return malloc( size );
}
void eight::Free( void* p )
{
	free( p );
}
void* eight::AlignedMalloc( size_t size, uint align )
{
	return _aligned_malloc( size, align );
}
void eight::AlignedFree( void* p )
{
	_aligned_free( p );
}

void* eight::VramAlloc( size_t size, uint align )
{
	return AlignedMalloc(size, align);
}
void eight::VramFree( void* p )
{
	AlignedFree(p);
}
#else
#error
#endif


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
		DebugOutput( l, f, fn, "Assertion failure - %s", e );
		//TODO - dialog box
	}
	return IsDebuggerAttached();
}

#ifdef eiIN_MEMORY_LOG
struct InMemoryLog
{
	InMemoryLog() : maxCapacity( 4096 ), size(0) {}
	void Push( const rde::string& msg )
	{
		ScopeLock<Futex> l(lock);
		messages.push_back(msg);
		++size;
		if( size >= maxCapacity )
			messages.pop_front();
	}
	uint GetLogSize()
	{
		return min( size, maxCapacity );
	}
	void GetLogItem( uint idxBegin, uint idxEnd, void( *pfn )(uint, const char*, void*), void* user )
	{
		eiASSERT( idxBegin < GetLogSize() );
		eiASSERT( idxEnd <= GetLogSize() );
		eiASSERT( idxBegin <= idxEnd );
		eiASSERT( pfn );
		lock.Lock();
		rde::list<rde::string>::iterator it = messages.begin();
		for( uint i=0; i!=idxBegin; ++i )
			++it;
		for( uint i=idxBegin; i!=idxEnd; ++i, ++it )
		{
//#if eiDEBUG_LEVEL > 0
			if( it == messages.end() )
			{
				lock.Unlock();
			//	eiASSERT( false );
				return;
			}
//#endif
			pfn(i, it->c_str(), user);
		}
		lock.Unlock();
	}
private:
	Futex lock;
	const uint maxCapacity;
	uint size;
	rde::list<rde::string> messages;
} g_log;
uint eight::GetLogSize() { return g_log.GetLogSize(); }
void eight::GetLogItems( uint idxBegin, uint idxEnd, void( *pfn )(uint, const char*, void*), void* user )  { g_log.GetLogItem( idxBegin, idxEnd, pfn, user ); }
#endif

static Futex futex;
void eight::Print( const char* msg )
{
	printf( "%s", msg );
#ifdef eiIN_MEMORY_LOG
	g_log.Push( rde::string( msg ) );
#endif
}
void eight::Printf( const char* fmt, ... )
{
	va_list	v;
	va_start(v, fmt);
	vprintf(fmt, v);
	va_end( v );
#ifdef eiIN_MEMORY_LOG
	//todo
//	sprintf(0, fmt, )
//	g_log.Push( rde::string( msg ) );
#endif
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
	int len = (int)strlen(prefix);
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
