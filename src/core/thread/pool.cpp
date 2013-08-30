#include <eight/core/thread/pool.h>
#include <eight/core/thread/thread.h>
#include <eight/core/thread/tasksection.h>
#include <eight/core/thread/atomic.h>
#include <eight/core/thread/fifo_mpmc.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/alloc/malloc.h>

using namespace eight;

class JobPoolImpl : NonCopyable
{
public:
	JobPoolImpl(Scope& a, uint numWorkers) : m_queue(a, 1024)//todo size
	{
	}
	~JobPoolImpl()
	{
	}
	void PushJob( JobPool::Job job, uint threadIndex, uint threadCount )
	{
		if( !m_queue.Push(job) )
		{
			job.fn( job.arg, threadIndex, threadCount );
			RunJob( threadIndex, threadCount );
		}
	}
	bool RunJob( uint threadIndex, uint threadCount )
	{
		JobPool::Job job;
		if( m_queue.Pop( job ) )
		{
			job.fn( job.arg, threadIndex, threadCount );
			return true;
		}
		return false;
	}
private:
	FifoMpmc<JobPool::Job> m_queue;//todo - multiple queues to reduce contention?
};
void JobPool::PushJob( Job j, uint i, uint c ) {        ((JobPoolImpl*)this)->PushJob( j, i, c ); }
bool JobPool::RunJob(         uint i, uint c ) { return ((JobPoolImpl*)this)->RunJob(     i, c ); }

struct PoolThreadEntryInfo
{
	FnPoolThreadEntry* entry;
	Atomic* exit;
	JobPool* jobs;
	int threadIndex;
	int numThreads;
	void* arg;
};

void ClearThreadId()
{
	internal::_ei_thread_id = (internal::ThreadId*)0;
}
static int PoolThreadMain( void* data, int systemId )
{
	PoolThreadEntryInfo& info = *(PoolThreadEntryInfo*)(data);
	internal::ThreadId threadId;
	threadId.index = info.threadIndex;
	threadId.mask = 1U<<info.threadIndex;
	threadId.poolSize = info.numThreads;
	threadId.poolMask = (1U<<info.numThreads)-1;
	threadId.exit = info.exit;
	threadId.jobs = info.jobs;
	internal::_ei_thread_id = &threadId;
	int result = info.entry( info.arg, info.threadIndex, info.numThreads, systemId );
	*info.exit = 1;
	threadId.index = 0;
	threadId.mask = 0;
	threadId.poolSize = 0;
	threadId.poolMask = 0;
	threadId.exit = 0;
	threadId.jobs = 0;
	internal::_ei_thread_id = (internal::ThreadId*)0;
	return result;
}

void eight::StartThreadPool(Scope& a, FnPoolThreadEntry* entry, void* arg, int numWorkers)
{
	if( numWorkers < 1 )
//		numWorkers = NumberOfPhysicalCores(a);  
		numWorkers = max(1U, NumberOfPhysicalCores(a) - 1);
//		numWorkers = max(1, NumberOfPhysicalCores()/2 + 1);
//		numWorkers = (NumberOfPhysicalCores()+1)/2; 
	Atomic* exit = eiNew(a, Atomic)();
	JobPoolImpl* jobs = eiNew(a, JobPoolImpl)( a, numWorkers );
	PoolThreadEntryInfo info;
	info.entry = entry;
	info.exit = exit;
	info.jobs = (JobPool*)jobs;
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
	eiASSERT( threadId.poolSize && threadId.mask && (threadId.mask & threadId.poolMask) );
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
	eiASSERT( threadId.poolSize );
	return threadId.poolSize;
}

JobPoolThread eight::CurrentJobPool()
{
	const internal::ThreadId& threadId = *internal::_ei_thread_id;
	eiASSERT( threadId.jobs );
	JobPoolThread v = 
	{
		threadId.jobs,
		threadId.index,
		threadId.poolSize
	};
	return v;
}

#include <stdio.h>

extern "C"
{
	typedef void (FnIspcTask)(void *data, int threadIndex, int threadCount,
		int taskIndex, int taskCount);
	struct IspcTaskNode
	{
		IspcTaskNode* next;
		FnIspcTask* pfn;
		void* data;
		s32 countTarget;
		s32 countStarted;
		s32 countFinished;
	};
	struct IspcDataNode
	{
		IspcDataNode* next;
		void* data;
	};
	struct IspcTaskGroup
	{
		IspcTaskNode* tasks;
		IspcDataNode* data;
	};
	static IspcTaskGroup* IspcInit( void** handlePtr )
	{
		IspcTaskGroup* group;
		if( *handlePtr == 0 )
		{
			group = (IspcTaskGroup*)Malloc(sizeof(IspcTaskGroup));
			group->data = 0;
			group->tasks = 0;
			*handlePtr = group;
		}
		else
		{
			group = (IspcTaskGroup*)*handlePtr;
		}
		return group;
	}
	static void IspcJob(void* handle, uint threadIndex, uint threadCount)
	{
		eiASSERT( handle );
		IspcTaskNode* task = (IspcTaskNode*)handle;
		s32 index = 0;
		if( task->countTarget > 1 )
		{
			index = ((Atomic&)task->countStarted)++;
		/*	if( index >= task->countTarget )
				return;*/
		}
	/*	else
			eiASSERT( task->countTarget == 1 );*/
		task->pfn( task->data, threadIndex, threadCount, index, task->countTarget );
		((Atomic&)task->countFinished)++;
	}
	void *ISPCAlloc(void **handlePtr, s64 size, s32 alignment)
	{
		IspcTaskGroup* group = IspcInit(handlePtr);
		void* data = AlignedMalloc((uint)size + sizeof(IspcDataNode), alignment);
		IspcDataNode* node = (IspcDataNode*)&((u8*)data)[size];
		node->next = group->data;
		node->data = data;
		group->data = node;
		return data;
	}
	void ISPCLaunch(void **handlePtr, void *f, void *data, int count)
	{
		eiASSERT( InPool() );
		IspcTaskGroup* group = IspcInit(handlePtr);

		IspcTaskNode* task = (IspcTaskNode*)Malloc(sizeof(IspcTaskNode));
		task->next = group->tasks;
		group->tasks = task;

		task->data = data;
		task->pfn = (FnIspcTask*)f;
		task->countTarget = count;
		task->countStarted = 0;
		task->countFinished = 0;

		JobPoolThread jobPool = CurrentJobPool();
		JobPool::Job job = { &IspcJob, task };
		for( int i=0; i!=count; ++i )
			jobPool.jobs->PushJob( job, jobPool.threadIndex, jobPool.threadCount );
	}
	struct IspcTaskComplete
	{
		IspcTaskComplete(IspcTaskNode* task):task(task){}IspcTaskNode* task;
		bool operator()()
		{
		/*	if( task->countTarget > 1 && task->countStarted < task->countTarget)
			{
				JobPoolThread jobs = CurrentJobPool();
				while( ((Atomic&)task->countStarted) < task->countTarget )
				{
					IspcJob( task, jobs.threadIndex, jobs.threadCount );
				}
			}*/
			return ((Atomic&)task->countFinished) == task->countTarget;
		}
	};
	void ISPCSync(void *handle)
	{
		eiASSERT( handle );
		IspcTaskGroup* group = (IspcTaskGroup*)handle;
		IspcTaskNode* task = group->tasks;
		while( task )
		{
			IspcTaskNode* temp = task;
			task = task->next;
			YieldThreadUntil( IspcTaskComplete(temp) );
			Free(temp);
		}
		IspcDataNode* node = group->data;
		while( node )
		{
			IspcDataNode* temp = node;
			node = node->next;
			AlignedFree(temp->data);
		}
		Free(group);
	}
};
