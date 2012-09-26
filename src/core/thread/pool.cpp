#include <eight/core/thread/pool.h>
#include <eight/core/thread/thread.h>
#include <eight/core/thread/tasksection.h>
#include <eight/core/thread/atomic.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>

using namespace eight;

struct PoolThreadEntryInfo
{
	FnPoolThreadEntry* entry;
	Atomic* exit;
	int threadIndex;
	int numThreads;
	void* arg;
};

static int PoolThreadMain( void* data, int systemId )
{
	PoolThreadEntryInfo& info = *(PoolThreadEntryInfo*)(data);
	internal::ThreadId threadId;
	threadId.index = info.threadIndex;
	threadId.mask = 1U<<info.threadIndex;
	threadId.poolSize = info.numThreads;
	threadId.poolMask = (1U<<info.numThreads)-1;
	threadId.exit = info.exit;
	internal::_ei_thread_id = &threadId;
	int result = info.entry( info.arg, info.threadIndex, info.numThreads, systemId );
	*info.exit = 1;
	threadId.index = 0;
	threadId.mask = 0;
	threadId.poolSize = 0;
	threadId.poolMask = 0;
	threadId.exit = 0;
	internal::_ei_thread_id = (internal::ThreadId*)0;
	return result;
}

void eight::StartThreadPool(Scope& a, FnPoolThreadEntry* entry, void* arg, int numWorkers)
{
	if( numWorkers < 1 )
		numWorkers = NumberOfPhysicalCores(a);  
//		numWorkers = max(1, NumberOfPhysicalCores() - 1);
//		numWorkers = max(1, NumberOfPhysicalCores()/2 + 1);
//		numWorkers = (NumberOfPhysicalCores()+1)/2; 
//		numWorkers = 1;
	Atomic* exit = eiNew(a, Atomic)();
	PoolThreadEntryInfo info;
	info.entry = entry;
	info.exit = exit;
	info.arg = arg;
	info.threadIndex = 0;
	info.numThreads = numWorkers;
	for( int i=1; i<numWorkers; ++i )
	{
		PoolThreadEntryInfo* data = eiAlloc( a, PoolThreadEntryInfo );
		*data = info;
		data->threadIndex = i;
		eiNew( a, Thread ) ( a, &PoolThreadMain, data );
	}
	PoolThreadMain( &info, 0 );
}

bool eight::InPool()
{
	return !!internal::_ei_thread_id;
}
PoolThread eight::CurrentPoolThread()
{
	const internal::ThreadId& threadId = *internal::_ei_thread_id;
	PoolThread v = 
	{
		threadId.mask,
		threadId.index,
		threadId.poolSize,
		threadId.poolMask
	};
	return v;
}

uint eight::CurrentPoolSize()
{
	const internal::ThreadId& threadId = *internal::_ei_thread_id;
	return threadId.poolSize;
}