#include <eight/core/thread/tasksection.h>
#include <eight/core/thread/pool.h>
#include <eight/core/math/arithmetic.h>
#include <eight/core/bit/twiddling.h>
#include <eight/core/macro.h>
#include <eight/core/throw.h>
#include <stdio.h>
using namespace eight;

Nil eight::internal::_ei_this_section;
Nil eight::internal::_ei_tasklist_lock;
ThreadLocalStatic<internal::ThreadId, internal::Tag_ThreadId> eight::internal::_ei_thread_id;
//ThreadLocalStatic<TaskDistribution, internal::Tag_TaskDistribution> eight::internal::_ei_task_distribution;

inline TaskDistribution EnterTaskSection_Impl( u32 workersUsed, const internal::ThreadId& thread)
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

TaskDistribution eight::internal::EnterTaskSection( const TaskSection& task, const ThreadId& thread, TaskSectionNoLock& lock )
{
	eiASSERT( task.IsTaskList() == false && task.IsMask() == false );
	eiASSERT( 0 == (thread.mask & task.workersDone) );
	TaskDistribution work =  EnterTaskSection_Impl( task.WorkersUsed(), thread );
	lock.Entered( task, work.thisWorker != -1 );
	return work;
}

TaskDistribution eight::internal::EnterTaskSectionMultiple( const TaskSection& task, const ThreadId& thread, TaskSectionNoLock& lock )
{
	eiASSERT( task.IsTaskList() == false && task.IsMask() == false );
	TaskDistribution work = EnterTaskSection_Impl( task.WorkersUsed(), thread );
	if( thread.mask & task.workersDone )
		work.thisWorker = -1;
	lock.Entered( task, work.thisWorker != -1 );
	return work;
}

TaskDistribution eight::internal::EnterTaskSectionMask( const ThreadMask& task, const ThreadId& thread, TaskSectionMaskLock& lock )
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
	{
		ReadBarrier();
		lock.Enter(task);
	}
	return work;
}

void eight::internal::ExitTaskSection( TaskSection& task, const TaskDistribution& work, const ThreadId& thread )
{
	eiASSERT( !task.IsTaskList() && !task.IsMask() );
	eiASSERT( work.thisWorker >= 0 );
	eiASSERT( work.numWorkers >  0 );
	u32 localDone, newDoneValue;
	do
	{
		localDone = task.workersDone;
		newDoneValue = localDone | thread.mask;
	} while( !task.workersDone.SetIfEqual( newDoneValue, localDone ) );
	//TODO - atomic add would be fine here

#if defined(eiBUILD_USE_TASK_WAKE_EVENTS)
	if( task.WorkersUsed() == newDoneValue )
		FireWakeEvent( (eight::ThreadId&)thread );
#endif
}

void eight::internal::ExitTaskSectionMask()
{
	WriteBarrier();
}

TaskDistribution eight::internal::EnterTaskSectionTaskList( TaskList& task, const ThreadId& thread, TaskSectionLock& lock )
{
	eiASSERT( task.IsTaskList() == true && task.IsMask() == false );
	
	TaskDistribution work;
	work.numWorkers = task.WorkersUsed();
	eiASSERT( work.numWorkers <= 0xFFFF ); //not enough bits here

	//if limit not reached, increment and lock high bit
	s32 value, newValue;
	do {
		value = task.workersDone;
		if( (value & 0xFFFF) >= work.numWorkers )
		{
			value = -1;//use -1 when all worker locks exhausted
			eiInfo(TaskSection, "%d - Pass - EnterTaskSectionTaskList\n", internal::_ei_thread_id->index );
			goto nolock;
		}
		newValue = value + 1;
		if( value == 0 )//first worker initialize a completion countdown in the high bits
			newValue |= work.numWorkers << 16;
	} while( !task.workersDone.SetIfEqual( newValue, value ) );
	lock.Lock(task);
	eiInfo(TaskSection, "%d - Lock - EnterTaskSectionTaskList\n", internal::_ei_thread_id->index );
	
	value &= 0xFFFF;
	eiASSERT( value < work.numWorkers );
nolock:
	
	work.thisWorker = value;
	work.threadIndex = thread.index;
	work.poolSize = thread.poolSize;
	return work;
}

void eight::internal::ExitTaskSectionTaskList( TaskList& task, const TaskDistribution& work, const ThreadId& thread )
{
	eiInfo(TaskSection, "%d - Exit - EnterTaskSectionTaskList\n", internal::_ei_thread_id->index );
	eiASSERT( task.IsTaskList() && !task.IsMask() );
	eiASSERT( work.thisWorker >= 0 );
	eiASSERT( work.numWorkers >  0 );
	eiASSERT( work.thisWorker < work.numWorkers );
	
	s32 value, newValue;
	do {
		value = task.workersDone;
		u32 counter = value & 0xffff0000U;
		newValue = (value & ~0xffff0000U) | (counter - 0x10000);
		eiASSERT( counter && newValue < value && (newValue&0xffff)==(value&0xffff) );
	} while( !task.workersDone.SetIfEqual( newValue, value ) );

#if defined(eiBUILD_USE_TASK_WAKE_EVENTS)
	if( task.WorkersUsed() == newValue )
		FireWakeEvent( (eight::ThreadId&)thread );
#endif
}


#ifdef eiBUILD_EXCEPTIONS
class task_section_wait_after_pool_exit : public std::exception {};
#endif

bool eight::internal::IsTaskSectionDone(const TaskList& task)
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
bool eight::internal::IsTaskSectionDone(const SectionBlob& blob)
{
	switch( blob.Type() )
	{
	case TaskSectionType::TaskSection:
		{
			const TaskSection& section = reinterpret_cast<const TaskSection&>(blob);
			return IsTaskSectionDone(section);
		}
	case TaskSectionType::TaskList:
		{
			const TaskList& section = reinterpret_cast<const TaskList&>(blob);
			return IsTaskSectionDone(section);
		}
	case TaskSectionType::ThreadMask:
	default:
		eiASSERT( false );
		return false;
	}
}

void eight::internal::ResetTaskSection(SectionBlob& task)
{
	TaskSection& self = *reinterpret_cast<TaskSection*>(&task);
	ResetTaskSection(self);
}
void eight::internal::ResetTaskSection(TaskList& task)
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
TaskSection::TaskSection( s32 workerMask, bool taskList, bool mask )
{
	workersDone = 0;
	//TODO - assert workerMask is valid?
	s32 flags = (taskList ? TaskSectionType::TaskList   : 0)
	          | (mask     ? TaskSectionType::ThreadMask : 0);
	workersUsed = workerMask | (flags << 30);
}

TaskList::TaskList( int workCount ) : TaskSection( (u32)workCount, true, false )
{
	eiASSERT( workCount >= 0 );
}

inline s32 ValidateThreadMask( s32 mask )
{
	if( mask == 1 )
		return mask;
	mask &= internal::_ei_thread_id->poolMask;
	return mask ? mask : 1;
}
inline s32 ValidateThreadMaskFromIndex( int index )
{
	if( index <= 0 )
		return 1;
	index = index % internal::_ei_thread_id->poolSize;
	eiASSERT( ValidateThreadMask( 1<<index ) == 1<<index );
	return 1<<index;
}

ThreadMask::ThreadMask( u32 threadMask )
: TaskSection( ValidateThreadMask(threadMask), false, true )
{
}
ThreadMask::ThreadMask( const ThreadMask& thread )
	: TaskSection( thread.WorkerMask(), false, true )
{
}
ThreadMask::ThreadMask( s32 workersUsed, bool taskList, bool mask )
	: TaskSection( workersUsed, taskList, mask )
{
	eiASSERT( !taskList && mask );
}

SingleThread::SingleThread( int threadIndex )
	: ThreadMask( ValidateThreadMaskFromIndex(threadIndex), false, true )
{
}
SingleThread::SingleThread( const SingleThread& thread )
	: ThreadMask( thread.WorkerMask(), false, true )
{
}

SingleThread SingleThread::CurrentThread()
{
	using namespace internal;
	return SingleThread(_ei_thread_id->index);
}

uint ThreadMask::GetMask() const
{
	return (uint)WorkerMask();
}
bool ThreadMask::IsCurrent() const
{
	using namespace internal;
	return (_ei_thread_id->mask & WorkerMask()) != 0;
}
bool SingleThread::IsCurrent() const
{
	using namespace internal;
	return _ei_thread_id->mask == WorkerMask();
}
uint SingleThread::WorkerIndex() const
{
	return BitIndex( WorkerMask() );
}
bool TaskSection::IsCurrent() const
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
bool TaskSection::IsTaskList() const { return 0!=(TaskSectionType::TaskList  & (int(workersUsed) >> 30)); }
bool TaskSection::IsMask     () const { return 0!=(TaskSectionType::ThreadMask & (int(workersUsed) >> 30)); }

TaskSectionType::Type SectionBlob::Type() const
{
	const TaskSection& self = *reinterpret_cast<const TaskSection*>(this);
	int type = ((u32)self.workersUsed) >> 30;
	return (TaskSectionType::Type)type;
}

#if !defined(eiBUILD_RETAIL)
struct Tag_SectionList {};
static ThreadLocalStatic<internal::TaskSectionLockBase, Tag_SectionList> g_currentSectionLock;
void internal::TaskSectionLockBase::Enter(const void* s)
{
	section = s;
	next = g_currentSectionLock;
	g_currentSectionLock = this;
	eiASSERT( g_currentSectionLock == this );
}
void internal::TaskSectionLockBase::Exit(const void* s)
{
	eiASSERT( section == s );
	eiASSERT( g_currentSectionLock == this );
	g_currentSectionLock = next;
}
#endif


bool eight::internal::IsInTaskSection(const TaskSection& task)
{
#if defined(eiBUILD_RETAIL)
	return true;
#else
	TaskSectionLockBase* base = g_currentSectionLock;
	return base && base->section == &task;
#endif
}
bool eight::internal::IsInTaskSection(const TaskList& task)
{
#if defined(eiBUILD_RETAIL)
	return true;
#else
	TaskSectionLockBase* base = g_currentSectionLock;
	return base && base->section == &task;
#endif
}
bool eight::internal::IsInTaskSection(const ThreadMask& task)
{
#if defined(eiBUILD_RETAIL)
	return true;
#else
	return (internal::_ei_thread_id->mask & task.GetMask()) != 0;
#endif
}
bool eight::internal::IsInTaskSection(const SectionBlob& blob)
{
#if defined(eiBUILD_RETAIL)
	return true;
#else
	switch( blob.Type() )
	{
	case TaskSectionType::TaskSection:
		{
			const TaskSection& section = reinterpret_cast<const TaskSection&>(blob);
			return IsInTaskSection(section);
		}
	case TaskSectionType::TaskList:
		{
			const TaskList& section = reinterpret_cast<const TaskList&>(blob);
			return IsInTaskSection(section);
		}
	case TaskSectionType::ThreadMask:
	default:
		eiASSERT( false );
		return false;
	}
#endif
}