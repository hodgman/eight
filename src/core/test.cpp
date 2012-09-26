#include <eight/core/test.h>
#include <eight/core/types.h>
#include <eight/core/debug.h>
#include <eight/core/os/win32.h>
#include <eight/core/thread/atomic.h>
#include <eight/core/thread/tasksection.h>
#include <stdio.h>
#include <cstdarg>
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")
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

eiInfoGroup( Test, true );

bool eight::RunUnitTest( const char* name, void(*pfn)(), const char* file, int line, const char* function )
{
	eiInfo( Test, "=====- Testing : %s -=====\n", name );//TODO - prepend thread index to log messages?
	EnterTest();
	if( !RunProtected( pfn ) )
	{
		TestFailure( file, line, function, "Unknown crash" );
	}
	bool ok = ExitTest();
	if( ok )
		eiInfo( Test, "Test complete : %s\n", name );
	else
		eiInfo( Test, "Test failed : %s\n", name );
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
		return EXCEPTION_CONTINUE_SEARCH;//don't catch if we're debugging - let the debugger catch the crash.
	return EXCEPTION_EXECUTE_HANDLER;//catch the exception.
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

void PrintStack( CONTEXT ctx )
{
	STACKFRAME64 sf = {};
	sf.AddrPC.Offset    = ctx.Eip;
	sf.AddrPC.Mode      = AddrModeFlat;
	sf.AddrStack.Offset = ctx.Esp;
	sf.AddrStack.Mode   = AddrModeFlat;
	sf.AddrFrame.Offset = ctx.Ebp;
	sf.AddrFrame.Mode   = AddrModeFlat;

	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();


	for (;;)
	{
		SetLastError(0);
		BOOL stack_walk_ok = StackWalk64(IMAGE_FILE_MACHINE_I386, process, thread, &sf,
							 &ctx, 0, &SymFunctionTableAccess64, 
							 &SymGetModuleBase64, 0);
		if (!stack_walk_ok || !sf.AddrFrame.Offset)
			return;

		// write the address
		printf( "0x%x|", sf.AddrPC.Offset );

		DWORD64 module_base = SymGetModuleBase64(process, sf.AddrPC.Offset);
		char module_name[MAX_PATH] = {};
		if(module_base)
			GetModuleFileNameA((HMODULE)module_base, module_name, MAX_PATH);
		printf("%s|", (*module_name)?module_name:"Module?");

		SYMBOL_INFO_PACKAGE sym = { sizeof(sym) };
		sym.si.MaxNameLen = MAX_SYM_NAME;
		if (SymFromAddr(process, sf.AddrPC.Offset, 0, &sym.si))
			printf("%s()", sym.si.Name);
		else
			printf("Function?");

		IMAGEHLP_LINE64 ih_line = { sizeof(IMAGEHLP_LINE64) };
		DWORD dummy = 0;
		if (SymGetLineFromAddr64(process, sf.AddrPC.Offset, &dummy, &ih_line))
			printf( "|%s:%d", ih_line.FileName, ih_line.LineNumber );

		printf("\n");
	}
}

LONG WINAPI MiniDump_UnhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo)
{
	LONG retval = EXCEPTION_CONTINUE_SEARCH;

    HANDLE hFile = CreateFile("crash.dmp",
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    MINIDUMP_EXCEPTION_INFORMATION aMiniDumpInfo;
    aMiniDumpInfo.ThreadId = GetCurrentThreadId();
    aMiniDumpInfo.ExceptionPointers = ExceptionInfo;
    aMiniDumpInfo.ClientPointers = TRUE;

    BOOL ok = MiniDumpWriteDump(GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            (MINIDUMP_TYPE) (MiniDumpNormal|MiniDumpWithDataSegs|MiniDumpWithFullMemory|MiniDumpWithHandleData),
            &aMiniDumpInfo,
            NULL,
            NULL);
	if( ok )
	{
		printf( "Saved dump file to '%s'\n", "crash.dmp" );
		retval = EXCEPTION_EXECUTE_HANDLER;
	}
	else
	{
		DWORD error = GetLastError();
		void* errText = 0;
		DWORD errLength = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errText, 0, 0);
		printf( "Failed to save dump file to '%s' (error %x, %s)\n", "crash.dmp", error, errText );
		LocalFree(errText); 
	}

    CloseHandle(hFile);

	PrintStack( *ExceptionInfo->ContextRecord );

    return retval;
}

void eight::InitCrashHandler()
{
	SetUnhandledExceptionFilter(&MiniDump_UnhandledExceptionFilter);
	SymInitialize(GetCurrentProcess(), 0, TRUE);
	SymSetOptions( SymGetOptions() | SYMOPT_LOAD_LINES );
}
//SymCleanup(GetCurrentProcess()), SetUnhandledExceptionFilter(NULL)