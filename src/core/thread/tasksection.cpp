#include <eight/core/thread/tasksection.h>
#include <eight/core/thread/pool.h>
#include <eight/core/math/arithmetic.h>
#include <eight/core/bit/twiddling.h>
#include <eight/core/macro.h>
#include <eight/core/throw.h>
#include <stdio.h>
using namespace eight;

Nil eight::internal::_ei_this_section;
Nil eight::internal::_ei_semaphore_lock;
ThreadLocalStatic<internal::ThreadId, internal::Tag_ThreadId> eight::internal::_ei_thread_id;
ThreadLocalStatic<TaskDistribution, internal::Tag_TaskDistribution> eight::internal::_ei_task_distribution;

inline TaskDistribution EnterTaskSection_Impl( u32 workersUsed, internal::ThreadId thread)
{
	using namespace internal;
	eiASSERT( *_ei_thread_id == thread );//global thread info system is working
	eiASSERT( (workersUsed & ~thread.poolMask)  == 0 );//tasks aren't trying to use non-existant workers
	eiASSERT( thread.poolSize > thread.index );//indices in range
	eiASSERT( thread.poolMask == (1U<<thread.poolSize)-1 );//masks match counts
	eiASSERT( thread.mask == 1U<<thread.index );
	eiASSERT( CountBitsSet(workersUsed) <= (uint)thread.poolSize );
	TaskDistribution work;
	work.numWorkers = CountBitsSet(workersUsed);
	work.thisWorker =( (thread.mask & workersUsed) != 0 )//is this thread used?
	                ? CountBitsSet((thread.mask-1) & workersUsed)//count used workers with masks less than the current thread
	                : -1;//current thread not used
	work.threadIndex = thread.index;
	work.poolSize = thread.poolSize;
	return work;
}

TaskDistribution eight::internal::EnterTaskSection(const TaskSection& task, ThreadId thread, TaskSectionNoLock& lock)
{
	eiASSERT( task.IsSemaphore() == false && task.IsMask() == false );
	eiASSERT( 0 == (thread.mask & task.workersDone) );
	TaskDistribution work =  EnterTaskSection_Impl( task.WorkersUsed(), thread );
	lock.Entered( work.thisWorker != -1 );
	return work;
}

TaskDistribution eight::internal::EnterTaskSectionMultiple(const TaskSection& task, ThreadId thread, TaskSectionNoLock& lock)
{
	eiASSERT( task.IsSemaphore() == false && task.IsMask() == false );
	TaskDistribution work = EnterTaskSection_Impl( task.WorkersUsed(), thread );
	if( thread.mask & task.workersDone )
		work.thisWorker = -1;
	lock.Entered( work.thisWorker != -1 );
	return work;
}

TaskDistribution eight::internal::EnterTaskSectionMask(const ThreadMask& task, ThreadId thread, TaskSectionMaskLock&)
{
	u32 taskWorkers = task.WorkersUsed();
	eiASSERT( task.IsMask() == true );
	eiASSERT( 0 == (thread.mask & task.workersDone) );
	eiASSERT( 1 == CountBitsSet(taskWorkers) );
	TaskDistribution work;
	work.thisWorker = (thread.mask & taskWorkers) ? 0 : -1;
	work.numWorkers = 1;
	work.threadIndex = thread.index;
	work.poolSize = thread.poolSize;
	if(	work.thisWorker != -1 )
		ReadBarrier();
	return work;
}

TaskDistribution eight::internal::EnterTaskSectionSemaphore(Semaphore& task, ThreadId thread, TaskSectionLock& lock)
{
	eiASSERT( task.IsSemaphore() == true && task.IsMask() == false );
	
	TaskDistribution work;
	work.numWorkers = task.WorkersUsed();

	//if limit not reached, increment and lock high bit
	s32 value;
	do {
		value = task.workersDone;
		value &= ~0x20000000;
		if( value >= work.numWorkers )
		{
			value = -1;//use -1 when all worker locks exhausted
			eiInfo(TaskSection, "%d - Pass - EnterTaskSectionSemaphore\n", internal::_ei_thread_id->index );
			goto nolock;
		}
	} while( !task.workersDone.SetIfEqual( (value+1) | 0x20000000, value ) );
	lock.Lock();
	eiInfo(TaskSection, "%d - Lock - EnterTaskSectionSemaphore\n", internal::_ei_thread_id->index );
nolock:

	work.thisWorker = value;
	work.threadIndex = thread.index;
	work.poolSize = thread.poolSize;
	return work;
}

void eight::internal::ExitTaskSection(TaskSection& task, const TaskDistribution& work, ThreadId thread)
{
	eiASSERT( !task.IsSemaphore() && !task.IsMask() );
	eiASSERT( work.thisWorker >= 0 );
	eiASSERT( work.numWorkers >  0 );
	u32 localDone;
	do
	{
		localDone = task.workersDone;
	} while( !task.workersDone.SetIfEqual(localDone | thread.mask, localDone) );
	//TODO - atomic add would be fine here
}

void eight::internal::ExitTaskSectionMask()
{
	WriteBarrier();
}

void eight::internal::ExitTaskSectionSemaphore(Semaphore& task, const TaskDistribution& work, ThreadId thread)
{
	eiInfo(TaskSection, "%d - Exit - EnterTaskSectionSemaphore\n", internal::_ei_thread_id->index );
	eiASSERT( task.IsSemaphore() && !task.IsMask() );
	eiASSERT( work.thisWorker >= 0 );
	eiASSERT( work.numWorkers >  0 );
	s32 value = task.workersDone;
	eiASSERT( (value & 0x20000000) );
	if( false eiDEBUG(||true) )
	{
		eiASSERT( task.workersDone.SetIfEqual( value & ~0x20000000, value ) );
	}
	else
		task.workersDone = value & ~0x20000000;
}


#ifdef eiBUILD_EXCEPTIONS
class task_section_wait_after_pool_exit : public std::exception {};
#endif

bool eight::internal::IsTaskSectionDone(const Semaphore& task)
{
	return IsTaskSectionDone(reinterpret_cast<const TaskSection&>(task));
}
bool eight::internal::IsTaskSectionDone(const TaskSection& task)
{
	using namespace internal;
	eiASSERT( _ei_thread_id->exit );
	eiDEBUG( if( *_ei_thread_id->exit ) { eiASSERT( !"waiting on task after pool-therads have terminated" ); eiTHROW(task_section_wait_after_pool_exit()); return true; } )
	eiASSERT( !task.IsMask() );
	return task.WorkersUsed() == task.workersDone;
}

void eight::internal::ResetTaskSection(SectionBlob& task)
{
	TaskSection& self = *reinterpret_cast<TaskSection*>(&task);
	ResetTaskSection(self);
}
void eight::internal::ResetTaskSection(Semaphore& task)
{
	TaskSection& self = *reinterpret_cast<TaskSection*>(&task);
	ResetTaskSection(self);
}
void eight::internal::ResetTaskSection(TaskSection& task)
{
	eiASSERT( !task.IsMask() );
	eiASSERT( task.WorkersUsed() == task.workersDone || task.workersDone == 0 );
	task.workersDone = 0;
}

TaskSection::TaskSection( int maxThreads, u32 threadMask )
{
	eiSTATIC_ASSERT( TaskSectionType::TaskSection == 0 );
	Init(maxThreads, threadMask);
}
TaskSection::TaskSection( const ThreadMask& thread )
{
	workersDone = 0;
	workersUsed = ((const TaskSection&)thread).WorkersUsed();
}
TaskSection::TaskSection( s32 workerMask, bool semaphore, bool mask )
{
	workersDone = 0;
	//TODO - assert workerMask is valid?
	s32 flags = (semaphore ? TaskSectionType::Semaphore  : 0)
	          | (mask      ? TaskSectionType::ThreadMask : 0);
	workersUsed = workerMask | (flags << 30);
}

Semaphore::Semaphore( int maxThreads ) : TaskSection( min( (u32)maxThreads, internal::_ei_thread_id->poolSize ), true, false )
{
}

static s32 ValidateThreadMask( s32 mask )
{
	mask &= CurrentPoolThread().poolMask;
	return mask ? mask : 1;
}
static s32 ValidateThreadMaskFromIndex( int index )
{
	return index < 0 ? 1 : ValidateThreadMask( 1<<index );
}

ThreadMask::ThreadMask( u32 threadMask )
: TaskSection( ValidateThreadMask(threadMask), false, true )
{
}
ThreadMask::ThreadMask( const ThreadMask& thread )
	: TaskSection( thread.WorkerMask(), false, true )
{
}
ThreadMask::ThreadMask( s32 workersUsed, bool semaphore, bool mask )
	: TaskSection( workersUsed, semaphore, mask )
{
	eiASSERT( !semaphore && mask );
}

SingleThread::SingleThread( int threadIndex )
	: ThreadMask( ValidateThreadMaskFromIndex(threadIndex), false, true )
{
}
SingleThread::SingleThread( const SingleThread& thread )
	: ThreadMask( thread.WorkerMask(), false, true )
{
}

uint ThreadMask::GetMask() const
{
	return (uint)WorkerMask();
}
bool ThreadMask::Current() const
{
	using namespace internal;
	return (_ei_thread_id->mask & WorkerMask()) != 0;
}
bool SingleThread::Current() const
{
	using namespace internal;
	return _ei_thread_id->mask == WorkerMask();
}
uint SingleThread::WorkerIndex() const
{
	return BitIndex( WorkerMask() );
}
bool TaskSection::Current() const
{
	using namespace internal;
	return (_ei_thread_id->mask & WorkerMask()) != 0;
}

void TaskSection::Init(int a_numThreads, u32 threadMask)
{
	using namespace internal;
	u32 poolSize = _ei_thread_id->poolSize;
	u32 numThreads = max( (u32)a_numThreads, ((u32)a_numThreads)-1 );
	numThreads = min( numThreads, poolSize );
	eiASSERT( threadMask );
	u32 poolMask = _ei_thread_id->poolMask;
	eiASSERT( poolMask );
	u32 usableMask = threadMask & poolMask;
	uint usableCount = CountBitsSet( usableMask );
	while( usableCount < numThreads )
	{
		eiSTATIC_ASSERT( sizeof(threadMask)==4 );
		uint firstOff = LeastSignificantBit( ~usableMask );
		eiASSERT( usableMask != (usableMask | 1U<<firstOff) );
		usableMask |= 1U<<firstOff;
		++usableCount;
	}
	while( usableCount > numThreads )
	{
		--usableCount;
		usableMask &= usableMask - 1; // clear the least significant bit set
	}
	eiASSERT( CountBitsSet(usableMask)==usableCount && usableCount==numThreads && numThreads );
	workersDone = 0;
	workersUsed = usableMask;
}


s32  TaskSection::WorkersUsed() const { return workersUsed & ~(0x3<<30); }
bool TaskSection::IsSemaphore() const { return 0!=(TaskSectionType::Semaphore  & (int(workersUsed) >> 30)); }
bool TaskSection::IsMask     () const { return 0!=(TaskSectionType::ThreadMask & (int(workersUsed) >> 30)); }

TaskSectionType::Type SectionBlob::Type() const
{
	const TaskSection& self = *reinterpret_cast<const TaskSection*>(this);
	int type = self.workersUsed >> 30;
	return (TaskSectionType::Type)type;
}