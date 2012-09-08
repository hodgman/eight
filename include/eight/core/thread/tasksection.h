//------------------------------------------------------------------------------
#pragma once
#include "eight/core/debug.h"
#include "eight/core/math/arithmetic.h"
#include "eight/core/macro.h"
#include "eight/core/thread/atomic.h"
#include "eight/core/thread/threadlocal.h"
#include "eight/core/noncopyable.h"
namespace eight {
//------------------------------------------------------------------------------

eiInfoGroup( TaskSection, false );

#define eiEnterTaskSection_Impl(section, option, lock, extra)								\
	{																						\
		using namespace internal;															\
		eiASSERT( &*_ei_thread_id );														\
		extra; lock _ei_section_lock;														\
		TaskDistribution  _ei_task_distribution_value										\
			= EnterTaskSection##option(section, *_ei_thread_id, _ei_section_lock);			\
		if( _ei_task_distribution_value.thisWorker >= 0 )									\
		{																					\
			eiASSERT( _ei_task_distribution_value.numWorkers > 0 );							\
			TaskSectionScope<lock> _ei_task_scope = MakeScope(_ei_task_distribution_value	\
			                                                 ,_ei_section_lock, &section);	\
			eiDEBUG(const void* const _ei_this_section = &section;)							//

	
///@define eiBeginSectionTask fo bar
#define eiBeginSectionTask(	        section )     eiEnterTaskSection_Impl(section, ,         TaskSectionNoLock, )		///< Begin execution of a block that is scheduled by a TaskSection object
#define eiBeginSectionThread(   section )     eiEnterTaskSection_Impl(section,Group,    TaskSectionGroupLock, )	///< Begin execution of a block that is scheduled by a ThreadGroup object
#define eiBeginSectionRedundant( section )    eiEnterTaskSection_Impl(section,Multiple,  TaskSectionNoLock, )		///< Begin execution of a possibly redundant block that is scheduled by a TaskSection object
#define eiBeginSectionSemaphore( section )    eiEnterTaskSection_Impl(section,Semaphore, TaskSectionLock,				\
                                                                            eiASSERT(_ei_semaphore_lock==Nil());		\
																			eiDEBUG(bool _ei_semaphore_lock = true;)	\
																			eiUNUSED( _ei_semaphore_lock ); )///< 

#define eiEndSection(section)															\
			eiASSERT( _ei_this_section == &section );										\
		}																					\
	}																						//

//if Exit isn't paired with Enter, MSVC reports "error C2678: binary '==' : no operator found which takes a left-hand operand of type 'eight::Nil' (or there is no acceptable conversion)"
//If two semaphore sections are nested, MSVC reports " error C2678: binary '==' : no operator found which takes a left-hand operand of type 'bool' (or there is no acceptable conversion)"

#define eiResetTaskSection(section)															\
	{  internal::ResetTaskSection(section);  }												//

// Creates a synchronisation fence that halts any thread in the pool here, until all threads
//  have completed the specified section.
#define eiWaitForSection(section)														\
	eight::YieldThreadUntil( eight::internal::WaitForTaskSection(&section) );				//

// A eiWaitForSection will ensure the specified section and it's preceeding sectons will
//  be complete, allowing some 'waits' to be assumed instead of stated via eiWaitForSection.
// When relying on this behaviour (of *not* waiting for preceeding sections if a
//  later section has already been waited for) it is recommended to document the assumption 
//  via this assertion.
#define eiAssertTaskDone(section)															\
	eiASSERT( eight::internal::IsTaskSectionDone(section) );								//

#define eiAssertInTaskSection(section)														\
	eiASSERT( true );//TODO - implement this

// Given a number of items in a task, find the range to be executed by this thread
#define eiDistributeTask( items, begin, end ) 0;											\
	{																						\
		using namespace internal;															\
		TaskDistribution* dist = _ei_task_distribution;										\
		eiASSERT( dist );																	\
		int numWorkers = dist->numWorkers;													\
		int workerIndex = dist->thisWorker;													\
		eiASSERT( numWorkers >= 1 );														\
		eiASSERT( workerIndex >= 0 );														\
		DistributeTask( workerIndex, numWorkers, items, begin, end );						\
	}																						//

template<class T>
inline void DistributeTask( uint workerIndex, uint numWorkers, T items, T &begin, T& end )
{
	eiASSERT( workerIndex < numWorkers );
	uint perWorker = items / numWorkers;												
	begin = perWorker * workerIndex;													
	end = (workerIndex==numWorkers-1)													
		? items                      //ensure last thread covers whole range
		: begin + perWorker;															
	begin = perWorker ? begin : min(workerIndex, (u32)items);   //special case,
	end   = perWorker ? end   : min(workerIndex+1, (u32)items); //less items than workers
	eiASSERT( begin >= 0 && end >= begin && end <= items );
}

// Optionally use this macro as an optimisation.
// Caches the ThreadId from thread-local-storage to a local variable to save the above macros
//  from performing the same TLS fetch continuously.
#define eiUsingTaskSections																	\
	internal::ThreadId _ei_thread_id_value = *internal::_ei_thread_id;						\
	internal::ThreadId* _ei_thread_id = &_ei_thread_id_value;								//


class TaskSection;


struct TaskDistribution
{
	int thisWorker;
	int numWorkers;
	int threadIndex;
	int poolSize;
};

struct SectionBlob;
class TaskSection;
class ThreadGroup;
class Semaphore;

namespace internal
{
	struct ThreadId
	{
		u32 mask;
		u32 index;
		u32 poolSize;
		u32 poolMask;
		Atomic* exit;
	};
	inline bool operator==( const ThreadId& a, const ThreadId& b )
	{
		return (a.mask==b.mask) & (a.index==b.index) & (a.poolSize==b.poolSize) & (a.poolMask==b.poolMask);
	}
	extern Nil _ei_this_section;
	extern Nil _ei_semaphore_lock;
	struct Tag_ThreadId {}; struct Tag_TaskDistribution {};
	extern ThreadLocalStatic<ThreadId, Tag_ThreadId> _ei_thread_id;
	extern ThreadLocalStatic<TaskDistribution, Tag_TaskDistribution> _ei_task_distribution;
	
	struct TaskSectionGroupLock;
	struct TaskSectionNoLock;
	struct TaskSectionLock;

	TaskDistribution EnterTaskSection(const TaskSection&, ThreadId, TaskSectionNoLock&);
	TaskDistribution EnterTaskSectionGroup(const ThreadGroup&, ThreadId, TaskSectionGroupLock&);
	TaskDistribution EnterTaskSectionMultiple(const TaskSection&, ThreadId, TaskSectionNoLock&);
	TaskDistribution EnterTaskSectionSemaphore(Semaphore&, ThreadId, TaskSectionLock&);
	void ExitTaskSection(TaskSection&, const TaskDistribution&, ThreadId);
	void ExitTaskSectionGroup();
	void ExitTaskSectionSemaphore(Semaphore&, const TaskDistribution&, ThreadId);
	bool IsTaskSectionDone(const TaskSection&);
	bool IsTaskSectionDone(const Semaphore&);
	void ResetTaskSection(SectionBlob&);
	void ResetTaskSection(TaskSection&);
	void ResetTaskSection(Semaphore&);
	
	struct TaskSectionGroupLock
	{
		void Exit(TaskSection& section, const TaskDistribution& dist, ThreadId thread)
		{
			ExitTaskSectionGroup();
		}
	};

	struct TaskSectionNoLock
	{
		 TaskSectionNoLock() : entered() {}
		~TaskSectionNoLock() { eiASSERT( !entered ); }
		void Entered(bool e) { entered = e; }
		void Exit(TaskSection& section, const TaskDistribution& dist, ThreadId thread)
		{
			eiASSERT(entered);
			ExitTaskSection(section, dist, thread);
			eiDEBUG( entered = false );
		}
	private:
		bool entered;
	};

	struct TaskSectionLock
	{
		 TaskSectionLock() : locked() {}
		~TaskSectionLock() { eiASSERT( !locked ); }
		void Lock() { locked = true; }
		void Exit(TaskSection& section, const TaskDistribution& dist, ThreadId thread)
		{
			eiASSERT(locked);
			ExitTaskSectionSemaphore((Semaphore&)section, dist, thread);
			eiDEBUG( locked = false );
		}
	private:
		bool locked;
	};

	struct WaitForTaskSection
	{
		WaitForTaskSection( const TaskSection* s ) : s(s) {} const TaskSection* s; 
		WaitForTaskSection( const SectionBlob* s ) : s((TaskSection*)s) {} 
		WaitForTaskSection( const Semaphore* s ) : s((TaskSection*)s) {} 
		bool operator()() const { return IsTaskSectionDone(*s); }
	};
		
	template<class T>
	class TaskSectionScope
	{
		TaskSection* m_section;
		T* m_section_lock;
		TaskDistribution* m_task_distribution_backup;
		eiDEBUG( TaskDistribution* m_task_distribution_current );
	public:
		TaskSectionScope(TaskDistribution& new_value, T& lock, TaskSection& section)
		{
			m_section = &section;
			m_section_lock = &lock;
			m_task_distribution_backup  = _ei_task_distribution;
			_ei_task_distribution       = &new_value;
			eiDEBUG( m_task_distribution_current = &new_value );
		}
		~TaskSectionScope()
		{
				eiASSERT( _ei_task_distribution == m_task_distribution_current );
				m_section_lock->Exit(*m_section, *_ei_task_distribution, *_ei_thread_id);
				_ei_task_distribution = m_task_distribution_backup;
		}
	};
	template<class T>
	TaskSectionScope<T> MakeScope( TaskDistribution& value, T& lock, void* section )
	{
		return TaskSectionScope<T>( value, lock, *(TaskSection*)section );
	}
}

struct TaskSectionType
{
	enum Type
	{
		TaskSection = 0,
		ThreadGroup,
		Semaphore
	};
};

struct SectionBlob
{
	s64 bits;

	TaskSectionType::Type Type() const;
};

class TaskSection
{
public:
	TaskSection( int maxThreads=0, u32 threadMask=~0U );
	explicit TaskSection( const ThreadGroup& );

	bool Current() const;
	
	operator SectionBlob() const { eiSTATIC_ASSERT(sizeof(*this)==sizeof(SectionBlob)); return *reinterpret_cast<const SectionBlob*>(this); }
protected:
	TaskSection( s32 workersUsed, bool semaphore, bool single );
	s32 WorkerMask() const { return WorkersUsed(); }
private:
	void Init(int maxThreads, u32 threadMask);
	//The high two bits of workersUsed record the type
	//For non-semaphore sections: Bitmasks of the thread-indices that will run the task.
	//For semaphore sections: Number of times the task has been invoked, and required number of times before it is done.
	//                        The high-bit of workersDone is locked upon incrementing the counter/starting the task, and cleared upon ending the task.
	Atomic workersDone;
	s32    workersUsed;//When both vars match, the task has been completed.
	s32 WorkersUsed () const;
	bool IsSemaphore() const;
	bool IsGroup   () const;
	friend SectionBlob;
	friend TaskDistribution internal::EnterTaskSection(const TaskSection&, ThreadId, TaskSectionNoLock&);
	friend TaskDistribution internal::EnterTaskSectionMultiple(const TaskSection&, ThreadId, TaskSectionNoLock&);
	friend TaskDistribution internal::EnterTaskSectionGroup(const ThreadGroup&, ThreadId, TaskSectionGroupLock&);
	friend TaskDistribution internal::EnterTaskSectionSemaphore(Semaphore&, ThreadId, TaskSectionLock&);
	friend             void internal::ExitTaskSectionSemaphore(Semaphore&, const TaskDistribution&, ThreadId);
	friend             void internal::ExitTaskSection(TaskSection&, const TaskDistribution&, ThreadId);
	friend             bool internal::IsTaskSectionDone(const TaskSection&);
	friend             void internal::ResetTaskSection(TaskSection&);
};


class ThreadGroup : private TaskSection
{
public://todo - refactor - was SingleThread, nee
	ThreadGroup( int threadIndex=-1 );
	ThreadGroup( const ThreadGroup& );

	bool Current() const;

	uint WorkerIndex() const;

	operator SectionBlob() const { eiSTATIC_ASSERT(sizeof(*this)==sizeof(SectionBlob)); return *reinterpret_cast<const SectionBlob*>(this); }

	friend TaskDistribution internal::EnterTaskSectionGroup(const ThreadGroup&, ThreadId, TaskSectionGroupLock&);
};


class Semaphore : private TaskSection
{
public:
	Semaphore( int maxThreads=1 );

	operator SectionBlob() const { eiSTATIC_ASSERT(sizeof(*this)==sizeof(SectionBlob)); return *reinterpret_cast<const SectionBlob*>(this); }

	friend TaskDistribution internal::EnterTaskSectionSemaphore(Semaphore&, ThreadId, TaskSectionLock&);
	friend             void internal::ExitTaskSectionSemaphore(Semaphore&, const TaskDistribution&, ThreadId);
};

eiSTATIC_ASSERT( sizeof(SectionBlob) == sizeof(TaskSection) );
eiSTATIC_ASSERT( sizeof(SectionBlob) == sizeof(ThreadGroup) );
eiSTATIC_ASSERT( sizeof(SectionBlob) == sizeof(Semaphore) );

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------