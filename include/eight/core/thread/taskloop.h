//------------------------------------------------------------------------------
#pragma once
#include <eight/core/debug.h>
#include <eight/core/types.h>
#include <eight/core/thread/tasksection.h>
#include <eight/core/thread/latent.h>
#include <eight/core/thread/atomic.h>

extern eight::Atomic g_profilersRunning;//TODO - hax
extern int g_profileFramesLeft;
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
	typedef void (FnInterrupt      )(Scope& a, void* interrupt, void* shared, const ThreadId&);
	typedef void (FnUserTask       )(Scope& scratch, void* shared, void* thread, void* preparedTask, const ThreadId&);
	typedef void*(FnUserPrepareTask)(Scope& scratch, void* shared, void* thread, const void* prevTask, const ThreadId&, void** interrupt);//return null to end loop
	struct Config
	{
		void*              userArgs;
		void*              userShared;
		FnUserPrepareTask* userPrepare;
		FnUserTask*        userTask;
		FnInterrupt*       userInterrupt;
	};
	typedef void*(FnUserInitThread)(Scope& thread, uint idx, TaskLoop&, Config*);

	TaskLoop( Scope& a, void* user, FnUserInitThread* i, uint totalStackSize, uint frameScratchSize, ConcurrentFrames::Type );

	ConcurrentFrames::Type MaxConcurrentFrames() const { return (ConcurrentFrames::Type)m_numAllocs; }

	void Run( const ThreadId& );
	void RunBackground();

	uint Frame() const;//returns the frame that the current thread is executing

	static int BackgroundThreadMain( void* arg, ThreadId& thread, uint systemId )
	{
#if ENABLE_PROFILE_ON_STARTUP
		if( eight::ProfileBegin() )
		{
			g_profilersRunning += 1;
			g_profileFramesLeft = 30;
		}
#endif
		TaskLoop& loop = *reinterpret_cast<TaskLoop*>(arg);
		loop.RunBackground();
		return 0;
	}

	static int ThreadMain( void* arg, ThreadId& thread, uint systemId )
	{
#if ENABLE_PROFILE_ON_STARTUP
		if( eight::ProfileBegin() )
		{
			g_profilersRunning += 1;
			g_profileFramesLeft = 30;
		}
#endif
		TaskLoop& loop = *reinterpret_cast<TaskLoop*>(arg);
		loop.Run( thread );
		return 0;
	}
private:
	void PrepareNextFrameGate( TaskList& section, TaskSection& task, u32 thisFrame, void* userThread, void* prevTask, const ThreadId& t );
	void PrepareNextFrameConcurrent( TaskList& section, u32 thisFrame, void* userThread, void* prevTask, const ThreadId& t );
	void EnterLoop( Scope& thread, void* userThread, uint* frame, const ThreadId& t );
	void ExitLoop();
	void PrepareFrameAndUnwind( uint frame, void* userThread, void* prevTask, const ThreadId& t );
	void PrepareFrame( const ThreadId&, uint frame, void* userThread, void* prevTask );
	Scope& GetScratch( uint allocId, uint thread );
	uint**            m_tlsFrame;
	u32               m_memSize;
	u32               m_scratchSize;
	u8*               m_mem;
	u8*               m_scratchMem;
	SingleThread      m_initialTask;
	Atomic            m_initialized;
	Atomic            m_thatsAllFolks;
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
