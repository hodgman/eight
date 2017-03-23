//------------------------------------------------------------------------------
#pragma once
#include "eight/core/debug.h"
#include "eight/core/math/arithmetic.h"
#include "eight/core/macro.h"
#include "eight/core/thread/atomic.h"
#include "eight/core/thread/threadlocal.h"
#include "eight/core/noncopyable.h"
#include "eight/core/profiler.h"
namespace eight {
//------------------------------------------------------------------------------

#define eiBUILD_USE_TASK_WAKE_EVENTS //todo - move to project config

eiInfoGroup( TaskSection, false );

#define eiEnterTaskSection_Impl(section, option, lock, extra, iterate, type, comment)		\
	{																						\
		using namespace eight::internal;													\
		eiASSERT( &*_ei_thread_id );														\
		bool _ei_break=false; extra; lock _ei_section_lock;									\
		TaskDistribution _ei_task_distribution_value;										\
		while( !_ei_break && ((_ei_task_distribution_value									\
			= EnterTaskSection##option(section, *_ei_thread_id, _ei_section_lock)),			\
			_ei_task_distribution_value.thisWorker >= 0) )									\
		{																					\
			iterate;																		\
			TaskDistribution* _ei_task_distribution = &_ei_task_distribution_value;			\
			eiASSERT( _ei_task_distribution_value.numWorkers > 0 );							\
			eiProfile(type ": " comment " (" #section ")");									\
			TaskSectionScope<lock> _ei_task_scope = MakeScope(_ei_task_distribution_value	\
			                                                 ,_ei_section_lock, &section);	\
			eiDEBUG(const void* const _ei_this_section = &section;)							//

//-V:eiBeginSectionRedundant:561
	
///@define eiBeginSectionTask fo bar
#define eiBeginSectionTask(	     section, ... )    eiEnterTaskSection_Impl(section, ,         TaskSectionNoLock, , _ei_break=true, "Task", __VA_ARGS__)		///< Begin execution of a block that is scheduled by a TaskSection object
#define eiBeginSectionThread(    section, ... )    eiEnterTaskSection_Impl(section,Mask,      TaskSectionMaskLock, , _ei_break=true, "Thread", __VA_ARGS__)	///< Begin execution of a block that is scheduled by a ThreadMask object
#define eiBeginSectionRedundant( section, ... )    eiEnterTaskSection_Impl(section,Multiple,  TaskSectionNoLock, , _ei_break=true, "Task", __VA_ARGS__)		///< Begin execution of a possibly redundant block that is scheduled by a TaskSection object
#define eiBeginSectionTaskList( section, ... )    eiEnterTaskSection_Impl(section,TaskList, TaskSectionLock,				\
                                                                            eiASSERT(_ei_tasklist_lock==Nil());		\
																			eiDEBUG(bool _ei_tasklist_lock = true;)	\
																			eiUNUSED( _ei_tasklist_lock );, , "TaskList", __VA_ARGS__ )///< 

//todo - use lambda syntax (e.g. auto l =[ &]{};) to remove the need for begin/end

#define eiEndSection(section)																\
			eiASSERT( _ei_this_section == &section );										\
		}																					\
	}																						//

//if Exit isn't paired with Enter, MSVC reports "error C2678: binary '==' : no operator found which takes a left-hand operand of type 'eight::Nil' (or there is no acceptable conversion)"
//If two taskList sections are nested, MSVC reports " error C2678: binary '==' : no operator found which takes a left-hand operand of type 'bool' (or there is no acceptable conversion)"

#define eiResetTaskSection(section)															\
	{  internal::ResetTaskSection(section);  }												//

// Creates a synchronisation fence that halts any thread in the pool here, until all threads
//  have completed the specified section.
#if defined(eiBUILD_USE_TASK_WAKE_EVENTS)
#define eiWaitForSection(section, ...)														\
	{eiProfile("Wait: " __VA_ARGS__ "(" #section ")" );										\
	eight::YieldThreadUntil( eight::internal::WaitForTaskSection(&section), 0, true, true );}		//
#else
#define eiWaitForSection(section)															\
	{eiProfile("Wait: " __VA_ARGS__ "(" #section ")" );										\
	eight::YieldThreadUntil( eight::internal::WaitForTaskSection(&section) );}				//
#endif

// A eiWaitForSection will ensure the specified section and it's preceeding sectons will
//  be complete, allowing some 'waits' to be assumed instead of stated via eiWaitForSection.
// When relying on this behaviour (of *not* waiting for preceeding sections if a
//  later section has already been waited for) it is recommended to document the assumption 
//  via this assertion.
#define eiAssertTaskDone(section)															\
	eiASSERT( eight::internal::IsTaskSectionDone(section) );								//

#define eiAssertInTaskSection(section)														\
	eiASSERT( eight::internal::IsInTaskSection(section) );									//

#define eiIsTaskDone(section)															\
	eight::internal::IsTaskSectionDone(section)											//

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

#define eiGetTaskDistribution() (*_ei_task_distribution)

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
	internal::ThreadId& _ei_thread_id_value = *internal::_ei_thread_id;						\
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
class ThreadMask;
class TaskList;
class JobPool;

namespace internal
{
	struct ThreadId
	{
		u32 mask;
		u32 index;
		u32 poolSize;
		u32 poolMask;
		Atomic* exit;
		JobPool* jobs;
		u32 poolId;
	};
	inline bool operator==( const ThreadId& a, const ThreadId& b )
	{
		return &a == &b;
	}
	extern Nil _ei_this_section;
	extern Nil _ei_tasklist_lock;
	struct Tag_ThreadId {}; struct Tag_TaskDistribution {};
	extern ThreadLocalStatic<ThreadId, Tag_ThreadId> _ei_thread_id;
	//extern ThreadLocalStatic<TaskDistribution, Tag_TaskDistribution> _ei_task_distribution;
	
	struct TaskSectionMaskLock;
	struct TaskSectionNoLock;
	struct TaskSectionLock;

	TaskDistribution EnterTaskSection( const TaskSection&, const ThreadId&, TaskSectionNoLock& );
	TaskDistribution EnterTaskSectionMask( const ThreadMask&, const ThreadId&, TaskSectionMaskLock& );
	TaskDistribution EnterTaskSectionMultiple( const TaskSection&, const ThreadId&, TaskSectionNoLock& );
	TaskDistribution EnterTaskSectionTaskList( TaskList&, const ThreadId&, TaskSectionLock& );
	void ExitTaskSection( TaskSection&, const TaskDistribution&, const ThreadId& );
	void ExitTaskSectionMask();
	void ExitTaskSectionTaskList( TaskList&, const TaskDistribution&, const ThreadId& );
	bool IsTaskSectionDone(const TaskSection&);
	bool IsTaskSectionDone(const TaskList&);
	bool IsTaskSectionDone(const SectionBlob&);
	bool IsInTaskSection(const TaskSection&);//n.b. these don't work in retail builds...
	bool IsInTaskSection(const TaskList&);
	bool IsInTaskSection(const ThreadMask&);
	bool IsInTaskSection(const SectionBlob&);
	void ResetTaskSection(SectionBlob&);
	void ResetTaskSection(TaskSection&);
	void ResetTaskSection(TaskList&);
	
	struct TaskSectionLockBase
	{
#ifdef eiBUILD_RETAIL
		inline void Enter(const void*){}
		inline void Exit(const void*){}
#else
		TaskSectionLockBase() : section(), next() {}
		void Enter(const void*);
		void Exit(const void*);
		const void* section;
		TaskSectionLockBase* next;
#endif
	};

	struct TaskSectionMaskLock : TaskSectionLockBase
	{
		void Enter(const TaskSection& s) { TaskSectionLockBase::Enter(&s); }
		void Exit( TaskSection& section, const TaskDistribution& dist, const ThreadId& thread )
		{
			ExitTaskSectionMask();
			TaskSectionLockBase::Exit(&section);
		}
	};

	struct TaskSectionNoLock : TaskSectionLockBase
	{
		 TaskSectionNoLock() : entered() {}
		~TaskSectionNoLock() { eiASSERT( !entered ); }
		void Entered(const TaskSection& s, bool e) { entered = e; if(e) TaskSectionLockBase::Enter(&s); }
		void Exit( TaskSection& section, const TaskDistribution& dist, const ThreadId& thread )
		{
			eiASSERT(entered);
			ExitTaskSection(section, dist, thread);
			eiDEBUG( entered = false );
			TaskSectionLockBase::Exit(&section);
		}
	private:
		bool entered;
	};

	struct TaskSectionLock : TaskSectionLockBase
	{
		 TaskSectionLock() : locked() {}
		~TaskSectionLock() { eiASSERT( !locked ); }
		void Lock(const TaskList& s) { locked = true; TaskSectionLockBase::Enter(&s); }
		void Exit( TaskSection& section, const TaskDistribution& dist, const ThreadId& thread )
		{
			eiASSERT(locked);
			ExitTaskSectionTaskList((TaskList&)section, dist, thread);
			eiDEBUG( locked = false );
			TaskSectionLockBase::Exit(&section);
		}
	private:
		bool locked;
	};

	struct WaitForTaskSection
	{
		WaitForTaskSection( const TaskSection* s ) : s(s) {} const TaskSection* s; 
		WaitForTaskSection( const SectionBlob* s ) : s((TaskSection*)s) {} 
		WaitForTaskSection( const TaskList* s ) : s((TaskSection*)s) {} 
		bool operator()() const { return IsTaskSectionDone(*s); }
	};
		
	template<class T>
	class TaskSectionScope
	{
		TaskSection* m_section;
		T* m_section_lock;
	//	TaskDistribution* m_task_distribution_backup;
	//	eiDEBUG( TaskDistribution* m_task_distribution_current );
		TaskDistribution* m_task_distribution_current;
	public:
		TaskSectionScope(TaskDistribution& new_value, T& lock, TaskSection& section)
		{
			m_section = &section;
			m_section_lock = &lock;
		//	m_task_distribution_backup  = _ei_task_distribution;
		//	_ei_task_distribution       = &new_value;
			m_task_distribution_current = &new_value;
		}
		~TaskSectionScope()
		{
		//	eiASSERT( _ei_task_distribution == m_task_distribution_current );
			m_section_lock->Exit(*m_section, *m_task_distribution_current, *_ei_thread_id);
		//	_ei_task_distribution = m_task_distribution_backup;
		}
	};
	template<class T>
	TaskSectionScope<T> MakeScope( TaskDistribution& value, T& lock, const void* section )
	{
		return TaskSectionScope<T>( value, lock, *(TaskSection*)section );
	}
}

struct TaskSectionType
{
	enum Type
	{
		TaskSection = 0,
		ThreadMask,
		TaskList
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
	explicit TaskSection( const ThreadMask& );

	bool IsCurrent() const;

	static TaskSection CurrentThread();
	
	operator SectionBlob() const { eiSTATIC_ASSERT(sizeof(*this)==sizeof(SectionBlob)); return *reinterpret_cast<const SectionBlob*>(this); }
protected:
	TaskSection( s32 workersUsed, bool taskList, bool mask );
	s32 WorkerMask() const { return WorkersUsed(); }
private:
	void Init(int maxThreads, u32 threadMask);
	//The high two bits of workersUsed record the type
	//For non-taskList sections: Bitmasks of the thread-indices that will run the task.
	//For taskList sections: Number of times the task has been invoked, and required number of times before it is done.
	//                        The high-bits of workersDone are set upon starting the task, and cleared upon ending the task.
	Atomic workersDone;
	s32    workersUsed;//When both vars match, the task has been completed.
	s32 WorkersUsed() const;
	bool IsTaskList() const;
	bool IsMask    () const;
	friend SectionBlob;
	friend TaskDistribution internal::EnterTaskSection( const TaskSection&, const ThreadId&, TaskSectionNoLock& );
	friend TaskDistribution internal::EnterTaskSectionMultiple( const TaskSection&, const ThreadId&, TaskSectionNoLock& );
	friend TaskDistribution internal::EnterTaskSectionMask( const ThreadMask&, const ThreadId&, TaskSectionMaskLock& );
	friend TaskDistribution internal::EnterTaskSectionTaskList( TaskList&, const ThreadId&, TaskSectionLock& );
	friend             void internal::ExitTaskSectionTaskList( TaskList&, const TaskDistribution&, const ThreadId& );
	friend             void internal::ExitTaskSection( TaskSection&, const TaskDistribution&, const ThreadId& );
	friend             bool internal::IsTaskSectionDone( const TaskSection& );
	friend             void internal::ResetTaskSection( TaskSection& );
};


class ThreadMask : private TaskSection
{
public:
	ThreadMask( u32 threadMask = 0xFFFFFFFF );
	ThreadMask( const ThreadMask& );

	bool IsCurrent() const;
	uint GetMask() const;

	operator SectionBlob() const { eiSTATIC_ASSERT(sizeof(*this)==sizeof(SectionBlob)); return *reinterpret_cast<const SectionBlob*>(this); }

	friend TaskDistribution internal::EnterTaskSectionMask( const ThreadMask&, const ThreadId&, TaskSectionMaskLock& );
protected:
	ThreadMask( s32 workersUsed, bool taskList, bool mask );
	s32 WorkerMask() const { return TaskSection::WorkerMask(); }
};

class SingleThread : public ThreadMask
{
public:
	SingleThread( int threadIndex=-1 );
	SingleThread( const SingleThread& );

	static SingleThread CurrentThread();

	bool IsCurrent() const;

	uint WorkerIndex() const;
};


class TaskList : private TaskSection
{
public:
	TaskList( int maxThreads=1 );

	operator SectionBlob() const { eiSTATIC_ASSERT(sizeof(*this)==sizeof(SectionBlob)); return *reinterpret_cast<const SectionBlob*>(this); }

	friend TaskDistribution internal::EnterTaskSectionTaskList( TaskList&, const ThreadId&, TaskSectionLock& );
	friend             void internal::ExitTaskSectionTaskList( TaskList&, const TaskDistribution&, const ThreadId& );
};


inline TaskSection TaskSection::CurrentThread() { return TaskSection( SingleThread::CurrentThread() ); }

eiSTATIC_ASSERT( sizeof(SectionBlob) == sizeof(TaskSection) );
eiSTATIC_ASSERT( sizeof(SectionBlob) == sizeof(ThreadMask) );
eiSTATIC_ASSERT( sizeof(SectionBlob) == sizeof(TaskList) );

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------