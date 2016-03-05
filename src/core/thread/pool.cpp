#include <eight/core/thread/pool.h>
#include <eight/core/thread/thread.h>
#include <eight/core/thread/tasksection.h>
#include <eight/core/thread/atomic.h>
#include <eight/core/thread/fifo_mpmc.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/profiler.h>
#include <eight/core/os/win32.h>
#include <stdio.h>

using namespace eight;
class JobPoolImpl : NonCopyable
{
public:
	JobPoolImpl(Scope& a, uint numWorkers) 
		: id(s_nextId++)
		, m_jobs(a, 1024)//todo sizes
		, m_lists(a, 1024)
	{
		m_semaphore = ::CreateSemaphore( 0, 0, 0x0FFFFFFF, 0) ;
		eiASSERT( m_semaphore );
	}
	~JobPoolImpl()
	{
		//todo - assert is empty? run jobs until it is empty?
		::CloseHandle( m_semaphore );
	}
	void PushJob( JobPool::Job job )
	{
		if( !m_jobs.Push( job ) )
		{
			ThreadId* thread = GetThreadId();
			//todo - log that the job queue filled up
			job.fn( job.arg, *thread );
			RunJob( *thread );
		}
		else
		{
			::ReleaseSemaphore( m_semaphore, 1, 0 );//a thread may be waiting on a different condition - if we spuriously wake it, it may consume the job.
		}
	}
	void PushJob( JobPool::Job job, ThreadId& thread )
	{
		if( !m_jobs.Push(job) )
		{
			//todo - log that the job queue filled up
			job.fn( job.arg, thread );
			RunJob( thread );
		}
		else
		{
			::ReleaseSemaphore( m_semaphore, 1, 0 );//a thread may be waiting on a different condition - if we spuriously wake it, it may consume the job.
		}
	}
	void PushJobList( JobPool::JobList list, ThreadId& thread_ )
	{
		while( !m_lists.Push( list ) )
		{
			eiASSERT(false && "todo");
			//todo - log that the job queue filled up
			RunJob( thread_ );
		}

		const internal::ThreadId& thread = (const internal::ThreadId&)thread_;
		::ReleaseSemaphore( m_semaphore, min(thread.poolSize,list.numJobs), 0 );//threads may be waiting on different conditions - if we spuriously wake them, they may consume the jobs.

	}
	bool RunJob( ThreadId& thread )
	{
		JobPool::Job job;
		if( m_jobs.Pop( job ) )
		{
		//	eiProfile("RunJob");
			job.fn( job.arg, thread );
			return true;
		}
		else
		{
			//first draft code!
			//TODO ---- finish this!!!
			JobPool::JobList* list;
			if( m_lists.Peek( list ) )
			{
				eiASSERT( false && "todo" );
			//	eiProfile( "RunJobList" );
				bool ran = false;
				while( 1 )
				{
					s32 index = (*list->nextToExecute)++;
					if( (u32)index < list->numJobs )
					{
						JobPool::Job& job = list->jobs[index];
						job.fn( job.arg, thread );
						if( (u32)index+1 == list->numJobs )
						{
							m_lists.Pop( *list );
						}
						ran = true;
					}
					else break;
				}
				return ran;
			}
		}
		return false;
	}

	bool WaitForWakeEvent()
	{
	//	eiProfile("WaitForWakeEvent");
		DWORD result = ::WaitForSingleObjectEx( m_semaphore, 1, TRUE );
#ifndef eiBUILD_RETAIL
		if( result == WAIT_FAILED )
		{
			result = GetLastError();
			void* errText = 0;
			DWORD errLength = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, 0, result, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPTSTR)&errText, 0, 0 );
			printf("%s\n", errText);
			LocalFree( errText );
		}
#endif
		eiASSERT(result != WAIT_FAILED);
		return result == WAIT_OBJECT_0;
	}
	void FireWakeEvent(uint threadCount)
	{
		eiProfile("FireWakeEvent");
		eiASSERT(threadCount);
		::ReleaseSemaphore( m_semaphore, threadCount, 0 );
	}
	const u32 id;
private:
	static Atomic s_nextId;
	HANDLE m_semaphore;
	FifoMpmc<JobPool::Job> m_jobs;//todo - multiple queues to reduce contention?
	FifoMpmc<JobPool::JobList> m_lists;
};
Atomic JobPoolImpl::s_nextId;
void JobPool::PushJob(     Job     j              ) { ((JobPoolImpl*)this)->PushJob(        j    ); }
void JobPool::PushJob(     Job     j, ThreadId& t ) { ((JobPoolImpl*)this)->PushJob(        j, t ); }
void JobPool::PushJobList( JobList l, ThreadId& t ) { ((JobPoolImpl*)this)->PushJobList(    l, t ); }
bool JobPool::RunJob(                 ThreadId& t ) { return ((JobPoolImpl*)this)->RunJob(     t ); }

bool JobPool::WaitForWakeEvent()               { return ((JobPoolImpl*)this)->WaitForWakeEvent();  }
void JobPool::FireWakeEvent(uint threadCount)  {        ((JobPoolImpl*)this)->FireWakeEvent(threadCount); }
u32 JobPool::Id() const { return ((JobPoolImpl*)this)->id; }

struct PoolThreadEntryInfo
{
	FnPoolThreadEntry* entry;
	Atomic* exit;
	JobPool* jobs;
	int threadIndex;
	int numThreads;
	void* arg;
	Atomic* runningCount;
};

void eight_ClearThreadId()
{
	internal::_ei_thread_id = (internal::ThreadId*)0;
}
static int PoolThreadMain( void* data, int systemId )
{
	PoolThreadEntryInfo& info = *(PoolThreadEntryInfo*)(data);
	if( info.runningCount )
		++(*info.runningCount);
	internal::ThreadId threadId;
	threadId.index = info.threadIndex;
	threadId.mask = 1U<<info.threadIndex;
	threadId.poolSize = info.numThreads;
	threadId.poolMask = (1U<<info.numThreads)-1;
	threadId.exit = info.exit;
	threadId.jobs = info.jobs;
	threadId.poolId = info.jobs->Id();
	internal::_ei_thread_id = &threadId;
	int result = info.entry( info.arg, (ThreadId&)threadId, systemId);
	*info.exit = 1;
	threadId.index = 0;
	threadId.mask = 0;
	threadId.poolSize = 0;
	threadId.poolMask = 0;
	threadId.exit = 0;
	threadId.jobs = 0;
	internal::_ei_thread_id = (internal::ThreadId*)0;
	if( info.runningCount )
		--(*info.runningCount);
	return result;
}

void eight::StartThreadPool( Scope& a, FnPoolThreadEntry* entry, void* arg, int numWorkers, Atomic* runningCount )
{
	if( numWorkers < 1 )
//		numWorkers = NumberOfPhysicalCores(a);  
		numWorkers = max(1U, NumberOfPhysicalCores(a)-1);
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
	info.runningCount = runningCount;
	for( int i=1; i<numWorkers; ++i )
	{
		PoolThreadEntryInfo* data = eiAlloc( a, PoolThreadEntryInfo );
		*data = info;
		data->threadIndex = i;
		eiNew( a, Thread ) ( a, &PoolThreadMain, data );
	}
	PoolThreadMain( &info, 0 );
}
JobPool* eight::StartBackgroundThreadPool( Scope& a, FnPoolThreadEntry* entry, void* arg, int numWorkers, Atomic* runningCount )
{
	if( numWorkers < 1 )
		numWorkers = 1;

	Atomic* exit = eiNew(a, Atomic)();
	JobPoolImpl* jobs = eiNew(a, JobPoolImpl)( a, numWorkers );
	PoolThreadEntryInfo info;
	info.entry = entry;
	info.exit = exit;
	info.jobs = (JobPool*)jobs;
	info.arg = arg;
	info.threadIndex = 0;
	info.numThreads = numWorkers;
	info.runningCount = runningCount;
	for( int i=0; i<numWorkers; ++i )
	{
		PoolThreadEntryInfo* data = eiAlloc( a, PoolThreadEntryInfo );
		*data = info;
		data->threadIndex = i;
		eiNew( a, Thread ) ( a, &PoolThreadMain, data );
	}
	return (JobPool*)jobs;
}

bool eight::InPool()
{
	return !!internal::_ei_thread_id;
}

uint eight::GetOsThreadId()
{
	return ::GetCurrentThreadId();
}

ThreadId* eight::GetThreadId()
{
	internal::ThreadId* threadId = internal::_ei_thread_id;
	return (ThreadId*)threadId;
}

u32 ThreadId::PoolId() const
{
	const internal::ThreadId& threadId = *(internal::ThreadId*)this;
	return threadId.poolId;
}
u32 ThreadId::ThreadIndex() const
{
	const internal::ThreadId& threadId = *(internal::ThreadId*)this;
	return threadId.index;
}
u32 ThreadId::NumThreadsInPool() const
{
	const internal::ThreadId& threadId = *(internal::ThreadId*)this;
	return threadId.poolSize;
}
JobPool& ThreadId::Jobs() const
{
	const internal::ThreadId& threadId = *(internal::ThreadId*)this;
	return *threadId.jobs;
}
/*
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
*/
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
	static void IspcJob(void* handle, ThreadId& thread)
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
		task->pfn( task->data, thread.ThreadIndex(), thread.NumThreadsInPool(), index, task->countTarget );
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

		ThreadId* thread = GetThreadId();
		JobPool::Job job = { &IspcJob, task };
		for( int i=0; i!=count; ++i )
			thread->Jobs().PushJob( job, *thread );
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
