//------------------------------------------------------------------------------
#pragma once
#include <eight/core/typeinfo.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/timer/timer.h>
#include <eight/core/types.h>
#include <eight/core/blob/types.h>
#include <eight/core/bind.h>
#include <eight/core/message.h>
#include <eight/core/thread/taskloop.h>
#include <eight/core/thread/pool.h>
namespace eight {

//------------------------------------------------------------------------------

//eiInfoGroup(TimedLoop, true);

template<class T, class InterruptData>
struct SelectInterruptHandler
{
	template<class SimLoop>
	static TaskLoop::FnInterrupt* Get() { return &SimLoop::HandleInterrupt<T>; }
};
template<class T>
struct SelectInterruptHandler<T, void>
{
	static void Fn(Scope&, void*, void*, uint, uint)
	{
		eiASSERT( false );
	}
	template<class SimLoop>
	static TaskLoop::FnInterrupt* Get() { return &Fn; }
};

class SimLoop
{
	SimLoop(); ~SimLoop();
	template<class T> static void* PrepareGameLoop(Scope& a, Scope& frame, void* shared, void* thread, const void* prevFrame, void** interrupt)
	{
		return ((T*)shared)->Prepare( a, frame, (T::ThreadData*)thread, (T::FrameData*)prevFrame, (typename T::InterruptData**)interrupt );
	}
	template<class T> static void ExecuteGameLoop(Scope& a, Scope& frame, void* shared, void* thread, void* taskData, uint worker)
	{
		((T*)shared)->Execute( a, frame, (T::ThreadData*)thread, (T::FrameData*)taskData, worker );
	}
	template<class T, class Y> friend struct SelectInterruptHandler;
	template<class T> static void HandleInterrupt(Scope& a, void* interrupt, void* shared, uint worker, uint numWorkers)
	{
		((T*)shared)->HandleInterrupt( a, (T::InterruptData*)interrupt, worker, numWorkers );
	}
	template<class T> struct GameLoopArgs { Scope* a; typename T::Args* userArgs; };
	template<class T> static void* InitGameLoopThread(Scope& thread, uint idx, TaskLoop& loop, TaskLoop::Config* cfg)
	{
		GameLoopArgs<T>& args = *(GameLoopArgs<T>*)cfg->userArgs;
		if( idx == 0 )
		{
			Scope& a = *eiNew(thread, Scope)( *args.a, "TaskLoop Main" );//ensure the 'T' is destructed before the thread pool exits
			cfg->userShared  = T::Create(a, thread, *args.userArgs, loop);
			cfg->userPrepare = &PrepareGameLoop<T>;
			cfg->userTask    = &ExecuteGameLoop<T>;
			cfg->userInterrupt = SelectInterruptHandler<T, T::InterruptData>::Get<SimLoop>();
		}
		return T::InitThread( thread, *args.userArgs );
	}
public:
	template<class Loop>
	static void Start(Scope& a, typename Loop::Args* args, ConcurrentFrames::Type latency=ConcurrentFrames::TwoFrames, uint totalStackSize=eiMiB(128), uint frameScratchSize=eiMiB(1))
	{
		GameLoopArgs<Loop> cargs = { &a, args };
		TaskLoop* loop = eiNew(a, TaskLoop)( a, &cargs, &InitGameLoopThread<Loop>, totalStackSize, frameScratchSize, latency );
		StartThreadPool( a, &TaskLoop::ThreadMain, loop );
	}
};



template<class A, class B> struct SameType      { const static bool value = false; };//TODO - move
template<class T>          struct SameType<T,T> { const static bool value = true; };

template<class Self> class GameLoopComponent : NonCopyable //TODO - see if this pans out
{
	struct ThreadData {};
	struct FrameData  {};
public:
	void* InitThread(Scope& thread, uint idx, TaskLoop& loop, TaskLoop::Config* cfg)
	{
	}
	void PrepareFrame(Scope& a, Scope& frame, void* thread, const void* prevFrame)
	{
		return ((Self*)this)->PrepareFrame(a, frame, *(Self::ThreadData*)thread, (const Self::FrameData*)prevFrame);
	}
};

struct FrameTimeSample
{
	double time;
	double prevTime;
	float delta;
	int frame;

	void Tick(double t, const FrameTimeSample* previous)
	{
		int    previousFrame = previous ? previous->frame : -1;
		double previousTime  = previous ? previous->time  : 0.0;
		time     = t;
		prevTime = previousTime;
		frame    = previousFrame + 1;
		delta    = previous ? (float)(t - previousTime) : 0.0f;

	//	uint worker    = CurrentPoolThread().index;
	//	eiInfo(TimedLoop, "%d P> %d, %.9f, %.9f, %.0ffps %s", worker, newFrame.frame, m_timer.Elapsed(), time, 1/delta, worker==0?"*":"");
	}
};

/*
class TimedLoop
{
	friend class SimLoop;
	struct ThreadData
	{
		double prevTime;
		void* userThreadLocal;
	};
	struct FrameData
	{
		int frame;
		double time;
		void* userFrame;
	};
public:
	eiBind(TimedLoop);
	typedef void* (FnAllocThread)(Scope& a, void* user);
	typedef void* (FnPrepareTask)(Scope& a, Scope& frame, void* user, void* thread);//return 0 to break
	typedef void  (FnExecuteTask)(Scope& a, Scope& frame, void* user, void* thread, void* task);
	//static void Start( Scope& scope, void* user, FnAllocThread* pfnA, FnPrepareTask* pfnP, FnExecuteTask* pfnE );
	
	struct Args {Scope* a; void* user; FnAllocThread* pfnA; FnPrepareTask* pfnP; FnExecuteTask* pfnE; TimedLoop* result;};
private:
	static TimedLoop* Create(Scope& a, Scope& thread, Args&, const TaskLoop&);
	static ThreadData* InitThread(Scope& thread, Args&);
	FrameData* Prepare(Scope& thread, Scope& frame, ThreadData*, FrameData*);
	void Execute(Scope& thread, Scope& frame, ThreadData*, FrameData*);

	TimedLoop(Scope& a, void* user, FnPrepareTask* pfnP, FnExecuteTask* pfnE);
	
//	static void* s_InitThread(Scope& thread, uint idx, TaskLoop& loop, TaskLoop::Config* cfg);
//	void* PrepareTask(Scope& a, Scope& frame, void* thread, const void* prevFrame);
//	void ExecuteTask(Scope& a, Scope& frame, void* threadData, void* taskData);
	
	Timer& m_timer;
	void* m_user;
	FnAllocThread* m_allocThreadLocal;
	FnPrepareTask* m_prepareTask;
	FnExecuteTask* m_executeTask;
};
*/
//------------------------------------------------------------------------------
//#include ".hpp"
} // namespace eight
//------------------------------------------------------------------------------
