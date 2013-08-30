#include <eight/core/thread/thread.h>
#include <eight/core/thread/atomic.h>
#include <eight/core/test.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/os/win32.h>

namespace eight { namespace internal { void OnThreadExitCleanup(); } }


using namespace eight;

uint eight::NumberOfHardwareThreads()
{
    SYSTEM_INFO si;
    GetSystemInfo( &si );
    return (uint)si.dwNumberOfProcessors;// TODO - this gives us the number of hardware threads, not the number of processing cores.
}
uint eight::NumberOfPhysicalCores(Scope& a)
{
	BOOL (WINAPI *getLogicalProcessorInformation)( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD ) 
		= (BOOL(WINAPI*)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,PDWORD)) GetProcAddress( GetModuleHandle("kernel32"), "GetLogicalProcessorInformation" );
	if( !getLogicalProcessorInformation )
		return NumberOfHardwareThreads();
	Scope temp(a,"temp");
	DWORD bufferSize = 0;
	DWORD rc = getLogicalProcessorInformation(0, &bufferSize);
	if( rc == TRUE || GetLastError() != ERROR_INSUFFICIENT_BUFFER || bufferSize == 0 )
		return NumberOfHardwareThreads();
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)temp.Alloc(bufferSize);
	rc = getLogicalProcessorInformation(buffer, &bufferSize);
	if( rc == FALSE )
		return NumberOfHardwareThreads();
	eiASSERT( (bufferSize/sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION))*sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) == bufferSize );
	uint numCores = 0;
	for( byte* end=((byte*)buffer)+bufferSize; (byte*)buffer < end; ++buffer )
	{
		numCores += ( buffer->Relationship == RelationProcessorCore ) ? 1 : 0;
	}
	if( numCores == 0 )
		return NumberOfHardwareThreads();
	return numCores;
}

struct ThreadInfo
{
	Atomic runnung;
	DWORD systemId;
	HANDLE handle;
};

struct ThreadEntryInfo
{
	Atomic started;
	ThreadEntry entry;
	void* arg;
	ThreadInfo* info;
};

static DWORD WINAPI ThreadMain_Internal( void* data )
{
	ThreadEntryInfo& entryInfo = *(ThreadEntryInfo*)(data);
	ThreadEntry entry = entryInfo.entry;
	void* arg = entryInfo.arg;
	ThreadInfo* info = entryInfo.info;
	YieldThreadUntil( WaitForTrue(info->runnung), 0, false );//wait until the main thread has filled in the info struct
//	while( !info->runnung ){}//wait until the main thread has filled in the info struct
	entryInfo.started = true;//signal to main thread that it can destroy the entryInfo struct now
	int result = entry( arg, (int)info->systemId );
	info->runnung = false;//signal to main that the thread has finished
	internal::OnThreadExitCleanup();
	return (DWORD)result;
}

namespace { struct TestContext
{
	DWORD retVal;
	void* data;
}; }
static void ThreadMain_Test_Internal( void* data )
{
	TestContext& ctx = *(TestContext*)data;
	ctx.retVal = ThreadMain_Internal( ctx.data );
}
static DWORD WINAPI ThreadMain_Test( void* data )
{
	TestContext ctx = { 0, data };
	bool didntCrash = RunProtected(&ThreadMain_Test_Internal, &ctx);
	eiASSERT( didntCrash );
	if( !didntCrash )
		internal::OnThreadExitCleanup();
	return ctx.retVal;
}

Thread::Thread(Scope& a, ThreadEntry entry, void* arg) : pimpl(a.Alloc<ThreadInfo>())
{
	void ClearThreadId();
	     ClearThreadId();
	ThreadInfo& info = *(ThreadInfo*)(pimpl);

	ThreadEntryInfo entryInfo;
	entryInfo.entry = entry;
	entryInfo.arg = arg;
	entryInfo.info = &info;
	entryInfo.started = false;
	info.runnung = false;
	if( eight::InTest() )
		info.handle = CreateThread( 0, 0, &ThreadMain_Test, &entryInfo, 0, &info.systemId );
	else
		info.handle = CreateThread( 0, 0, &ThreadMain_Internal, &entryInfo, 0, &info.systemId );
	eiASSERT( info.handle != 0 );
	if( !info.handle )
		return;
//	SetThreadPriority( info.handle, THREAD_PRIORITY_ABOVE_NORMAL );
	info.runnung = true;//signal the new thread that the info struct has been filled in
	YieldThreadUntil( WaitForTrue(entryInfo.started), 0, false );//wait until the thread has consumed the local entryInfo variable
}

Thread::~Thread()
{
	ThreadInfo& info = *(ThreadInfo*)(pimpl);
	if( info.runnung )
		WaitForSingleObject( info.handle, INFINITE );
}
