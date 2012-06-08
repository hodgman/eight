#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/thread/taskloop.h>
#include <eight/core/debug.h>

namespace eight
{
	struct LoopTasks
	{
		TaskSection     frameTask;
		TaskSection     startedThisFrame;
		Semaphore prepareNextFrame;
		void*           userTask;
	};
}

using namespace eight;

TaskLoop::TaskLoop( Scope& a, void* user, FnUserInitThread i,
				    uint totalStackSize, uint frameScratchSize )
        : m_memSize(totalStackSize), m_mem(eiAllocArray(a,u8,m_memSize))
		, m_scratchSize(frameScratchSize), m_scratchMem(eiAllocArray(a,u8,m_scratchSize))
{
	eiASSERT( m_mem && m_memSize );
	eiASSERT( m_scratchMem && m_scratchSize );
	m_config.user = user;
	m_config.userTask = 0;
	m_config.userPrepare = 0;
	m_userInitThread = i;
	m_scratchBuffers = 0;
}

void TaskLoop::Run(uint workerIdx, uint numWorkers)
{
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

	EnterLoop(a, userThread, &frame, workerIdx, numWorkers);
	eiASSERT( m_config.userTask && m_config.userPrepare );
	while( m_tasks[allocId]->userTask )
	{
		LoopTasks& tasks = *m_tasks[allocId];
		eiBeginSectionTask( tasks.startedThisFrame );
		eiEndSection( tasks.startedThisFrame );

		//todo - kick a PrepareNextFrame job here (with depend on last frame completing) and wait at the end of this frame.
/*!*/		PrepareNextFrame( a, tasks.prepareNextFrame, frame, userThread, tasks.userTask, workerIdx, numWorkers );

		eiBeginSectionTask( tasks.frameTask );
		{
			eiInfo(TaskLoop, "%d > begin frame\n", internal::_ei_thread_id->index);
			m_config.userTask( a, GetScratch(allocId, workerIdx), m_config.user, userThread, tasks.userTask );
		}
		eiEndSection( tasks.frameTask );
		
		//eiWaitForSection( tasks.frameTask );//debug sync
		eiWaitForSection( tasks.startedThisFrame );//dont leave a frame until all threads have started on it.
		//dont prepare next frame until last frame is complete
//!		PrepareNextFrame( a, tasks.prepareNextFrame, frame, userThread, tasks.userTask, workerIdx, numWorkers );
		eiWaitForSection( tasks.prepareNextFrame );//dont start next frame until it's been prepared

		++frame;
		allocId = frame % numAllocs;
	}
	ExitLoop();
}

uint TaskLoop::Frame() const
{
	return *m_tlsFrame[internal::_ei_thread_id->index];
}

void TaskLoop::PrepareNextFrame(Scope& a, Semaphore& s, u32 thisFrame, void* userThread, void* prevTask, uint worker, uint numWorkers)
{
	u32 frame = thisFrame + 1;//fetch next frame's allocator from the ring and clear it
	uint allocId = frame % numAllocs;
	eiBeginSectionSemaphore( s );
	{
		eiInfo(TaskLoop, "%d > prepare next frame\n", internal::_ei_thread_id->index);
		for(int i=0; i!=numWorkers; ++i)
			GetScratch(allocId, i).Unwind();
		PrepareFrame(a, worker, frame, userThread, prevTask);
	}
	eiEndSection( s );
}

void TaskLoop::EnterLoop(Scope& a, void* userThread, uint* frame, uint workerIdx, uint numWorkers)
{
	bool mainThread = false;
	eiBeginSectionThread( m_initialTask );
	{
		m_tlsFrame = eiAllocArray(a, uint*, numWorkers);
		uint numScratchBuffers = numAllocs * numWorkers;
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

void TaskLoop::PrepareFrame(Scope& a, uint workerIdx, uint frame, void* userThread, void* prevTask)
{
	uint allocId = frame % numAllocs;
	Scope& scratch = GetScratch(allocId, workerIdx);
	LoopTasks* tasks = eiNew(scratch, LoopTasks);
	tasks->userTask = m_config.userPrepare(a, scratch, m_config.user, userThread, prevTask);
	m_tasks[allocId] = tasks;
}
Scope& TaskLoop::GetScratch(uint allocId, uint thread)
{
	eiASSERT( allocId < numAllocs );
	return *m_scratchBuffers[allocId+thread*numAllocs];
}