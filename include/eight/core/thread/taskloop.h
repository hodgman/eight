//------------------------------------------------------------------------------
#pragma once
#include <eight/core/debug.h>
#include <eight/core/types.h>
#include <eight/core/thread/tasksection.h>
#include <eight/core/thread/latent.h>
namespace eight {
//------------------------------------------------------------------------------

class Timer;
class Scope;
class TaskSection;
struct LoopTasks;

eiInfoGroup( TaskLoop, false );

class TaskLoop
{
public:
	typedef void (FnUserTask       )(Scope& a, Scope& scratch, void* shared, void* thread, void* preparedTask, uint worker);
	typedef void*(FnUserPrepareTask)(Scope& a, Scope& scratch, void* shared, void* thread, const void* prevTask);//return null to end loop
	struct Config
	{
		void*              userArgs;
		void*              userShared;
		FnUserPrepareTask* userPrepare;
		FnUserTask*        userTask;
	};
	typedef void*(FnUserInitThread)(Scope& thread, uint idx, TaskLoop&, Config*);

	TaskLoop( Scope& a, void* user, FnUserInitThread* i, uint totalStackSize, uint frameScratchSize, ConcurrentFrames::Type );

	ConcurrentFrames::Type MaxConcurrentFrames() const { return (ConcurrentFrames::Type)m_numAllocs; }

	void Run( uint workerIdx, uint numWorkers );

	uint Frame() const;//returns the frame that the current thread is executing
	
	static int ThreadMain( void* arg, uint workerIdx, uint numWorkers, uint systemId )
	{
		TaskLoop& loop = *reinterpret_cast<TaskLoop*>(arg);
		loop.Run(workerIdx, numWorkers);
		return 0;
	}
private:
	void PrepareNextFrameGate( Scope& thread, Semaphore& section, TaskSection& task, u32 thisFrame, void* userThread, void* prevTask, uint worker, uint numWorkers );
	void PrepareNextFrameConcurrent( Scope& thread, Semaphore& section, u32 thisFrame, void* userThread, void* prevTask, uint worker, uint numWorkers );
	void EnterLoop( Scope& thread, void* userThread, uint* frame, uint workerIdx, uint numWorkers );
	void ExitLoop();
	void PrepareFrameAndUnwind( Scope& thread, uint workerIdx, uint frame, void* userThread, void* prevTask, uint numWorkers );
	void PrepareFrame( Scope& thread, uint workerIdx, uint frame, void* userThread, void* prevTask );
	Scope& GetScratch( uint allocId, uint thread );
	uint**            m_tlsFrame;
	u32               m_memSize;
	u32               m_scratchSize;
	u8*               m_mem;
	u8*               m_scratchMem;
	ThreadGroup       m_initialTask;
	Atomic            m_initialized;
	const u32         m_numAllocs;
	LoopTasks*        m_tasks;
	Scope**           m_scratchBuffers;
	FnUserInitThread* m_userInitThread;
	Config            m_config;
};

//------------------------------------------------------------------------------
//#include ".hpp"
} // namespace eight
//------------------------------------------------------------------------------
