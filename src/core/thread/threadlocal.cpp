#include <eight/core/thread/threadlocal.h>
#include <eight/core/os/win32.h>

using namespace eight;


u32  eight::TlsAlloc()
{
	u32 index = ::TlsAlloc();
	eiASSERT( index != TLS_OUT_OF_INDEXES );
	return index;
}

void eight::TlsFree( u32 index )
{
	eiDEBUG( BOOL ok = ) ::TlsFree( index );
	eiASSERT( ok );
}

void* eight::TlsGet( u32 index )
{
	return ::TlsGetValue( index );
}

void eight::TlsSet( u32 index, void* value )
{
	::TlsSetValue( index, value );
}