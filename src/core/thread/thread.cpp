#include <eight/core/thread/thread.h>
#include <eight/core/thread/atomic.h>
#include <eight/core/test.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/os/win32.h>

namespace eight { namespace internal { void OnThreadExitCleanup(); } }


#if eiDEBUG_LEVEL > 0 && !defined(eiPROTECTED_THREADS)
#define eiPROTECTED_THREADS
#endif

using namespace eight;

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
	YieldThreadUntil( WaitForTrue(info->runnung) );//wait until the main thread has filled in the info struct
//	while( !info->runnung ){}//wait until the main thread has filled in the info struct
	entryInfo.started = true;//signal to main thread that it can destroy the entryInfo struct now
	int result = entry( arg, (int)info->systemId );
	info->runnung = false;//signal to main that the thread has finished
#if !defined(eiPROTECTED_THREADS)
	internal::OnThreadExitCleanup();
#endif
	return (DWORD)result;
}

#if defined(eiPROTECTED_THREADS)
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
	internal::OnThreadExitCleanup();
	return ctx.retVal;
}
#endif

Thread::Thread(Scope& a, ThreadEntry entry, void* arg) : pimpl(a.Alloc<ThreadInfo>())
{
	ThreadInfo& info = *(ThreadInfo*)(pimpl);

	ThreadEntryInfo entryInfo;
	entryInfo.entry = entry;
	entryInfo.arg = arg;
	entryInfo.info = &info;
	entryInfo.started = false;
	info.runnung = false;
#if defined(eiPROTECTED_THREADS)
	info.handle = CreateThread( 0, 0, &ThreadMain_Test, &entryInfo, 0, &info.systemId );
#else
	info.handle = CreateThread( 0, 0, &ThreadMain_Internal, &entryInfo, 0, &info.systemId );
#endif
	eiASSERT( info.handle != 0 );
	if( !info.handle )
		return;
	info.runnung = true;//signal the new thread that the info struct has been filled in
	YieldThreadUntil( WaitForTrue(entryInfo.started) );//wait until the thread has consumed the local entryInfo variable
}

Thread::~Thread()
{
	ThreadInfo& info = *(ThreadInfo*)(pimpl);
	if( info.runnung )
		WaitForSingleObject( info.handle, INFINITE );
}
