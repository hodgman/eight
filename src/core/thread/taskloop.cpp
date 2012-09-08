#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/thread/taskloop.h>
#include <eight/core/debug.h>

namespace eight
{
	struct LoopTasks
	{
		TaskSection frameTask;
		TaskSection startedThisFrame;
		Semaphore   prepareNextFrame;
		void*       userTask;
	};
}

using namespace eight;

TaskLoop::TaskLoop( Scope& a, void* user, FnUserInitThread i,
				   uint totalStackSize, uint frameScratchSize, ConcurrentFrames::Type concurrentFrames )
        : m_memSize(totalStackSize), m_mem(eiAllocArray(a,u8,m_memSize))
		, m_scratchSize(frameScratchSize), m_scratchMem(eiAllocArray(a,u8,m_scratchSize))
		, m_numAllocs((uint)concurrentFrames), m_tasks(eiAllocArray(a,LoopTasks,m_numAllocs))
{
	eiASSERT( m_mem && m_memSize );
	eiASSERT( m_scratchMem && m_scratchSize );
	eiASSERT( m_numAllocs >= 1 && m_numAllocs <= 3 );
	m_config.userArgs = user;
	m_config.userShared = 0;
	m_config.userTask = 0;
	m_config.userPrepare = 0;
	m_userInitThread = i;
	m_scratchBuffers = 0;
}

void TaskLoop::Run(uint workerIdx, uint numWorkers)
{
	for( uint i=0, end=m_numAllocs; i!=end; ++i )
	{
		new(&m_tasks[i])LoopTasks;
		if( m_numAllocs == 1 )
		{
			eiSTATIC_ASSERT( sizeof(Semaphore) >= sizeof(Atomic) );
			Atomic& gate = *(Atomic*)&m_tasks[i].prepareNextFrame;
			gate = 0;
		}
	}
	eiASSERT( workerIdx == internal::_ei_thread_id->index );
	eiASSERT( numWorkers == internal::_ei_thread_id->poolSize );
	uint memBegin, memEnd;
	const uint memLine = 16;
	DistributeTask( workerIdx, numWorkers, m_memSize/memLine, memBegin, memEnd );
	eiASSERT(m_mem);
	StackAlloc stack( &m_mem[memBegin*memLine], (memEnd-memBegin)*memLine );
	Scope a( stack, "TaskLoop" );
	DistributeTask( workerIdx, numWorkers, m_scratchSize, memBegin, memEnd );

	void* userThread = m_userInitThread( a, workerIdx, *this, &m_config );
	uint frame = 0;
	uint allocId = 0;
	uint prevAllocId = 0;

	EnterLoop(a, userThread, &frame, workerIdx, numWorkers);
	eiASSERT( m_config.userTask && m_config.userPrepare );
	while( m_tasks[allocId].userTask )
	{
		LoopTasks& tasks = m_tasks[allocId];
		LoopTasks& prevTasks = m_tasks[prevAllocId];
		eiBeginSectionTask( tasks.startedThisFrame );
		eiEndSection( tasks.startedThisFrame );

	/*	if( m_numAllocs == 3 )
		{
			PrepareNextFrameConcurrent( a, tasks.prepareNextFrame, frame, userThread, tasks.userTask, workerIdx, numWorkers );
		}*/

		eiBeginSectionTask( tasks.frameTask );
		{
			eiInfo(TaskLoop, "%d > begin frame\n", internal::_ei_thread_id->index);
			m_config.userTask( a, GetScratch(allocId, workerIdx), m_config.userShared, userThread, tasks.userTask, workerIdx );
		}
		eiEndSection( tasks.frameTask );
		
		switch( m_numAllocs )
		{
		case 1:
			{
				//dont prepare next frame until this frame is complete
				PrepareNextFrameGate( a, tasks.prepareNextFrame, tasks.frameTask, frame, userThread, tasks.userTask, workerIdx, numWorkers );//N.B. also performs thread sync
			}
			break;
		case 2:
			{
				eiWaitForSection( tasks.startedThisFrame );//dont leave a frame until all threads have started on it.
				//dont prepare next frame until last frame is complete (true if all threads have started this frame)
				PrepareNextFrameConcurrent( a, tasks.prepareNextFrame, frame, userThread, tasks.userTask, workerIdx, numWorkers );
			}
			break;
		case 3:
			{
				eiWaitForSection( prevTasks.startedThisFrame );//dont leave a frame until all threads have started on the previous one.
				PrepareNextFrameConcurrent( a, tasks.prepareNextFrame, frame, userThread, tasks.userTask, workerIdx, numWorkers );
			}
			break;
		default:
			eiASSERT( false );
		}

		if( m_numAllocs != 1 )//syncing for 1-alloc-mode done inside PrepareNextFrameGate
		{
			eiWaitForSection( tasks.prepareNextFrame );//dont start next frame until it's been prepared
		}

		++frame;
		prevAllocId = allocId;
		allocId = frame % m_numAllocs;
	}
	ExitLoop();
}

uint TaskLoop::Frame() const
{
	return *m_tlsFrame[internal::_ei_thread_id->index];
}

void TaskLoop::PrepareNextFrameGate(Scope& a, Semaphore& s, TaskSection& frameTask, u32 thisFrame, void* userThread, void* prevTask, uint worker, uint numWorkers)
{
	eiASSERT( m_numAllocs == 1 );
	u32 nextFrame = thisFrame + 1;

	eiSTATIC_ASSERT( sizeof(Semaphore) >= sizeof(Atomic) );
	Atomic& gate = *(Atomic*)&s;
	eiDEBUG( s32 gateValue = (s32)gate );
	eiASSERT( gateValue == (s32)thisFrame || gateValue == (s32)nextFrame || gateValue == -1 );//not prepared, prepared, or preparing
	if( gate.SetIfEqual(-1, thisFrame) )//lock gate from 'not prepared' to 'preparing'
	{
		eiWaitForSection( frameTask );//wait for other threads to finish the task before unwinding
		PrepareFrameAndUnwind(a, worker, nextFrame, userThread, prevTask, numWorkers);
		gate = (s32)nextFrame;//mark as 'prepared'
	}
	else // if lock fails, it's either 'prepared' or 'preparing'
	{
		YieldThreadUntil( WaitForValue(gate, (s32)nextFrame) );//wait until 'prepared'
	}
}
void TaskLoop::PrepareNextFrameConcurrent(Scope& a, Semaphore& s, u32 thisFrame, void* userThread, void* prevTask, uint worker, uint numWorkers)
{
	eiASSERT( m_numAllocs != 1 );
	u32 nextFrame = thisFrame + 1;
//	uint allocId = nextFrame % m_numAllocs;
	eiBeginSectionSemaphore( s );
	{
		PrepareFrameAndUnwind(a, worker, nextFrame, userThread, prevTask, numWorkers);
	}
	eiEndSection( s );
}

void TaskLoop::EnterLoop(Scope& a, void* userThread, uint* frame, uint workerIdx, uint numWorkers)
{
	bool mainThread = false;
	eiBeginSectionThread( m_initialTask );
	{
		m_tlsFrame = eiAllocArray(a, uint*, numWorkers);
		uint numScratchBuffers = m_numAllocs * numWorkers;
		m_scratchBuffers = eiAllocArray(a, Scope*, numScratchBuffers);
		for( uint i=0; i!=numScratchBuffers; ++i )
		{
			uint beginMem, endMem;
			DistributeTask( i, numScratchBuffers, m_scratchSize, beginMem, endMem );
			StackAlloc* stack = eiNew( a, StackAlloc )( m_scratchMem+beginMem, endMem-beginMem );
			m_scratchBuffers[i] = eiNew( a, Scope )( *stack, "Scratch" );
		}
		PrepareFrame(a, workerIdx, 0, userThread, 0);
		m_initialized = numWorkers;
		mainThread = true;
	}
	eiEndSection( m_initialTask );
	if( !mainThread )
		YieldThreadUntil( WaitForTrue(m_initialized) );

	eiASSERT( frame );
	m_tlsFrame[workerIdx] = frame;
}

void TaskLoop::ExitLoop()
{
	m_initialized--;
	YieldThreadUntil( WaitForFalse(m_initialized) );
}

inline void TaskLoop::PrepareFrameAndUnwind(Scope& a, uint workerIdx, uint frame, void* userThread, void* prevTask, uint numWorkers)
{
	eiInfo(TaskLoop, "%d > prepare next frame\n", internal::_ei_thread_id->index);
	uint allocId = frame % m_numAllocs;
	for(int i=0; i!=numWorkers; ++i)
		GetScratch(allocId, i).Unwind();
	PrepareFrame(a, workerIdx, frame, userThread, prevTask);
}
inline void TaskLoop::PrepareFrame(Scope& a, uint workerIdx, uint frame, void* userThread, void* prevTask)
{
	uint allocId = frame % m_numAllocs;
	Scope& scratch = GetScratch(allocId, workerIdx);
	LoopTasks& tasks = m_tasks[allocId];
	eiResetTaskSection( tasks.frameTask );
	eiResetTaskSection( tasks.startedThisFrame );
	if( m_numAllocs != 1 )
		eiResetTaskSection( tasks.prepareNextFrame );
	tasks.userTask = m_config.userPrepare(a, scratch, m_config.userShared, userThread, prevTask);
}
Scope& TaskLoop::GetScratch(uint allocId, uint thread)
{
	eiASSERT( allocId < m_numAllocs );
	return *m_scratchBuffers[allocId+thread*m_numAllocs];
}