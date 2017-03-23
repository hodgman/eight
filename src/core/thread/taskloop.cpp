#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/thread/taskloop.h>
#include <eight/core/debug.h>
#include <stdlib.h>

namespace eight
{
	struct LoopTasks
	{
		TaskSection frameTask;
		TaskSection startedThisFrame;
		TaskList   prepareNextFrame;
		void*       userTask;
		void*       interrupt;
		TaskSection completedInterrupt;
	};
}

using namespace eight;

TaskLoop::TaskLoop( Scope& a, void* user, FnUserInitThread i,
				   uint totalStackSize, uint frameScratchSize, ConcurrentFrames::Type concurrentFrames )
        : m_memSize(totalStackSize), m_mem(eiAllocArray(a,u8,m_memSize))
		, m_scratchSize(frameScratchSize), m_scratchMem(eiAllocArray(a,u8,m_scratchSize))
		, m_numAllocs((uint)concurrentFrames), m_tasks(eiAllocArray(a,LoopTasks,m_numAllocs))
		, m_tlsFrame()
{
	eiASSERT( m_mem && m_memSize );
	eiASSERT( m_scratchMem && m_scratchSize );
	eiASSERT( m_numAllocs >= 1 && m_numAllocs <= 3 );
	m_config.userArgs = user;
	m_config.userShared = 0;
	m_config.userTask = 0;
	m_config.userPrepare = 0;
	m_config.userInterrupt = 0;
	m_userInitThread = i;
	m_scratchBuffers = 0;
}

void TaskLoop::Run(const ThreadId& t)
{
	if( t.ThreadIndex() == 0 )
	{
		for( uint i=0, end=m_numAllocs; i!=end; ++i )
		{
			new(&m_tasks[i])LoopTasks;
			if( m_numAllocs == 1 )
			{
				eiSTATIC_ASSERT( sizeof(TaskList) >= sizeof(Atomic) );
				Atomic& gate = *(Atomic*)&m_tasks[i].prepareNextFrame;
				gate = 0;
			}
		}
	}
	eiASSERT( t.ThreadIndex() == internal::_ei_thread_id->index );
	eiASSERT( t.NumThreadsInPool() == internal::_ei_thread_id->poolSize );
	uint memBegin, memEnd;
	const uint memLine = 16;
	DistributeTask( t.ThreadIndex(), t.NumThreadsInPool(), m_memSize/memLine, memBegin, memEnd );
	eiASSERT(m_mem);
	StackAlloc stack( &m_mem[memBegin*memLine], (memEnd-memBegin)*memLine );
	Scope a( stack, "TaskLoop Thread" );
	DistributeTask( t.ThreadIndex(), t.NumThreadsInPool(), m_scratchSize, memBegin, memEnd );

	void* userThread = m_userInitThread( a, t.ThreadIndex(), *this, &m_config );
	uint frame = 0;
	uint allocId = 0;
	uint prevAllocId = 0;

	EnterLoop(a, userThread, &frame, t);
	eiASSERT( m_config.userTask && m_config.userPrepare );
	while( m_tasks[allocId].userTask )
	{
		LoopTasks& tasks = m_tasks[allocId];
		LoopTasks& prevTasks = m_tasks[prevAllocId];
		eiBeginSectionTask( tasks.startedThisFrame );
		eiEndSection( tasks.startedThisFrame );

		if( tasks.interrupt )
		{
			eiASSERT( m_config.userInterrupt );
			eiWaitForSection( tasks.startedThisFrame );
			m_config.userInterrupt( a, tasks.interrupt, m_config.userShared, t );
			eiBeginSectionTask( tasks.completedInterrupt );
			eiEndSection( tasks.completedInterrupt );
			eiWaitForSection( tasks.completedInterrupt );
		}

		eiBeginSectionTask( tasks.frameTask, "TaskLoop Frame" );
		{
			eiInfo(TaskLoop, "%d > begin frame\n", internal::_ei_thread_id->index);
			m_config.userTask( GetScratch(allocId, t.ThreadIndex()), m_config.userShared, userThread, tasks.userTask, t );
		}
		eiEndSection( tasks.frameTask );
		
		switch( m_numAllocs )
		{
		case 1:
			{
				//dont prepare next frame until this frame is complete
				PrepareNextFrameGate( tasks.prepareNextFrame, tasks.frameTask, frame, userThread, tasks.userTask, t );//N.B. also performs thread sync
			}
			break;
		case 2:
			{
				eiWaitForSection( tasks.startedThisFrame, "Other threads" );//dont leave a frame until all threads have started on it.
				//dont prepare next frame until last frame is complete (true if all threads have started this frame)
				PrepareNextFrameConcurrent( tasks.prepareNextFrame, frame, userThread, tasks.userTask, t );
			}
			break;
		case 3:
			{
				eiWaitForSection( prevTasks.startedThisFrame, "Other threads" );//dont leave a frame until all threads have started on the previous one.
				PrepareNextFrameConcurrent( tasks.prepareNextFrame, frame, userThread, tasks.userTask, t );
			}
			break;
		default:
			eiASSERT( false );
		}

		if( m_numAllocs != 1 )//syncing for 1-alloc-mode done inside PrepareNextFrameGate
		{
			eiWaitForSection( tasks.prepareNextFrame, "Prepare next frame" );//dont start next frame until it's been prepared
		}

		++frame;
		prevAllocId = allocId;
		allocId = frame % m_numAllocs;
	}
	ExitLoop();
}
void TaskLoop::RunBackground()
{
	BusyWait<true> spinner;
	while( m_thatsAllFolks == 0 )
	{
		void HackHackLoader();
		HackHackLoader();
		spinner.Spin();
	}
}

uint TaskLoop::Frame() const
{
	return m_tlsFrame ? *m_tlsFrame[internal::_ei_thread_id->index] : 0;
}

void TaskLoop::PrepareNextFrameGate(TaskList& s, TaskSection& frameTask, u32 thisFrame, void* userThread, void* prevTask, const ThreadId& t)
{
	eiASSERT( m_numAllocs == 1 );
	u32 nextFrame = thisFrame + 1;

	eiSTATIC_ASSERT( sizeof(TaskList) >= sizeof(Atomic) );
	Atomic& gate = *(Atomic*)&s;
	eiDEBUG( s32 gateValue = (s32)gate );
	eiASSERT( gateValue == (s32)thisFrame || gateValue == (s32)nextFrame || gateValue == -1 );//not prepared, prepared, or preparing
	if( gate.SetIfEqual(-1, thisFrame) )//lock gate from 'not prepared' to 'preparing'
	{
		eiWaitForSection( frameTask, "Other threads" );//wait for other threads to finish the task before unwinding
		PrepareFrameAndUnwind(nextFrame, userThread, prevTask, t);
		gate = (s32)nextFrame;//mark as 'prepared'
	}
	else // if lock fails, it's either 'prepared' or 'preparing'
	{
		YieldThreadUntil( WaitForValue(gate, (s32)nextFrame) );//wait until 'prepared'
	}
}
void TaskLoop::PrepareNextFrameConcurrent(TaskList& s, u32 thisFrame, void* userThread, void* prevTask, const ThreadId& t)
{
	eiASSERT( m_numAllocs != 1 );
	u32 nextFrame = thisFrame + 1;
//	uint allocId = nextFrame % m_numAllocs;
	eiBeginSectionTaskList( s );
	{
		PrepareFrameAndUnwind(nextFrame, userThread, prevTask, t);
	}
	eiEndSection( s );
}

void TaskLoop::EnterLoop(Scope& a, void* userThread, uint* frame, const ThreadId& t)
{
	bool mainThread = false;
	eiBeginSectionThread( m_initialTask );
	{
		m_tlsFrame = eiAllocArray(a, uint*, t.NumThreadsInPool());
		uint numScratchBuffers = m_numAllocs * t.NumThreadsInPool();
		m_scratchBuffers = eiAllocArray(a, Scope*, numScratchBuffers);
		for( uint i=0; i!=numScratchBuffers; ++i )
		{
			uint beginMem, endMem;
			DistributeTask( i, numScratchBuffers, m_scratchSize/4, beginMem, endMem );
			StackAlloc* stack = eiNew( a, StackAlloc )( m_scratchMem+beginMem, endMem-beginMem );
			m_scratchBuffers[i] = eiNew( a, Scope )( *stack, "Scratch" );
		}
		m_tlsFrame[t.ThreadIndex()] = frame;
		PrepareFrame(t, 0, userThread, 0);
		m_initialized = t.NumThreadsInPool();
		mainThread = true;
	}
	eiEndSection( m_initialTask );
	if( !mainThread )
	{
		YieldThreadUntil( WaitForTrue(m_initialized) );
		eiASSERT( frame );
		m_tlsFrame[t.ThreadIndex()] = frame;
	}
}

inline void TaskLoop::PrepareFrameAndUnwind(uint frame, void* userThread, void* prevTask, const ThreadId& t)
{
	eiProfile("PrepareFrameAndUnwind");
	eiInfo(TaskLoop, "%d > prepare next frame\n", internal::_ei_thread_id->index);
	uint allocId = frame % m_numAllocs;
	for(int i=0; i!=t.NumThreadsInPool(); ++i)
		GetScratch(allocId, i).Unwind();
	PrepareFrame(t, frame, userThread, prevTask);
}
inline void TaskLoop::PrepareFrame(const ThreadId& t, uint frame, void* userThread, void* prevTask)
{
	uint allocId = frame % m_numAllocs;
	Scope& scratch = GetScratch(allocId, t.ThreadIndex());
	LoopTasks& tasks = m_tasks[allocId];
	eiResetTaskSection( tasks.frameTask );
	eiResetTaskSection( tasks.startedThisFrame );
	if( m_numAllocs != 1 )
		eiResetTaskSection( tasks.prepareNextFrame );
	tasks.interrupt = 0;
	eiResetTaskSection( tasks.completedInterrupt );
	tasks.userTask = m_config.userPrepare(scratch, m_config.userShared, userThread, prevTask, t, &tasks.interrupt);
}
Scope& TaskLoop::GetScratch(uint allocId, uint thread)
{
	eiASSERT( allocId < m_numAllocs );
	return *m_scratchBuffers[allocId+thread*m_numAllocs];
}

//#pragma optimize("",off)//!!!!!
void TaskLoop::ExitLoop()
{
	m_initialized--;
	YieldThreadUntil( WaitForFalse(m_initialized), 5, false );//TODO - drain job queue before leaving loop to begin with!?
	m_thatsAllFolks = 1;
}
