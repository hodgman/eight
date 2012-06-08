#include <eight/core/test.h>
#include <eight/core/types.h>
#include <eight/core/debug.h>
#include <eight/core/os/win32.h>
#include <eight/core/thread/atomic.h>
#include <eight/core/thread/tasksection.h>
#include <stdio.h>
#include <cstdarg>
using namespace eight;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static Atomic g_InTest = Atomic(0);//TODO - should be TLS for a multi-thread safe testing sytem. At the moment it's assume only one test is running at a time.
static Atomic g_TestFailure = Atomic(0);

void eight::EnterTest()
{
	eiASSERT( g_InTest++ == 0 );
	eiASSERT( !g_TestFailure );
	g_InTest = 1;
	g_TestFailure = 0;
}

bool eight::ExitTest()
{
	eiASSERT( g_InTest-- == 1 );
	g_InTest = 0;
	bool ok = (g_TestFailure==0);
	g_TestFailure = 0;
	return ok;
}

bool eight::InTest()
{
	return !!g_InTest;
}

bool eight::RunUnitTest( const char* name, void(*pfn)(), const char* file, int line, const char* function )
{
	Printf( "=====- Testing : %s -=====\n", name );//TODO - prepend thread index to log messages?
	EnterTest();
	if( !RunProtected( pfn ) )
	{
		TestFailure( file, line, function, "Unknown crash" );
	}
	bool ok = ExitTest();
	if( ok )
		Printf( "Test complete : %s\n", name );
	else
		Printf( "Test failed : %s\n", name );
	return ok;
}

void eight::TestFailure( const char* file, int line, const char* function, const char* message, ... )
{
	va_list args;
	va_start( args, message );
	DebugOutput( line, file, function, message, args );
	va_end( args );
	++g_TestFailure;
}

bool eight::IsDebuggerAttached()
{
	return !!IsDebuggerPresent();
}

static int Filter(uint code)
{
	if( internal::_ei_thread_id && internal::_ei_thread_id->exit )
		*internal::_ei_thread_id->exit = 1;//notify the thread pool that one of it's threads has crashed.
	if( IsDebuggerAttached() )
		return 0;//don't catch if we're debugging - let the debugger catch the crash.
	return 1;//catch the exception.
}

bool eight::RunProtected( void(*pfn)(void*), void* arg )
{
	bool error = false;
	__try
	{
		pfn(arg);
	}
	__except(Filter(GetExceptionCode()))
	{
		error = true;
	}
	return !error;
}

bool eight::RunProtected( void(*pfn)() )
{
	bool error = false;
	__try
	{
		pfn();
	}
	__except(Filter(GetExceptionCode()))
	{
		error = true;
	}
	return !error;
}