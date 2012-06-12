//------------------------------------------------------------------------------
#pragma once
#include <eight/core/typeinfo.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/thread/taskloop.h>
#include <eight/core/timer/timer.h>
#include <eight/core/types.h>
#include <eight/core/blob/types.h>
#include <eight/core/bind.h>
#include <eight/core/message.h>
namespace eight {

//------------------------------------------------------------------------------

eiInfoGroup(TimedLoop, true);

class TimedLoop
{
	static void  s_Task       (Scope& a, Scope& frame, void* shared, void* thread, void* task)     {        ((Args*)shared)->result->ExecuteTask(a, frame, thread, task); }
	static void* s_PrepareTask(Scope& a, Scope& frame, void* shared, void* thread, void* prevTask) { return ((Args*)shared)->result->PrepareTask(a, frame, thread, prevTask); }
public:
	eiBind(TimedLoop);
	typedef void* (FnAllocThread)(Scope& a, void* user);
	typedef void* (FnPrepareTask)(Scope& a, Scope& frame, void* user, void* thread);//return 0 to break
	typedef void  (FnExecuteTask)(Scope& a, Scope& frame, void* user, void* thread, void* task);
	static void Start( Scope& scope, void* user, FnAllocThread* pfnA, FnPrepareTask* pfnP, FnExecuteTask* pfnE );
private:
	struct Args {Scope* a; void* user; FnAllocThread* pfnA; FnPrepareTask* pfnP; FnExecuteTask* pfnE; TimedLoop* result;};
	TimedLoop(Scope& a, void* user, FnPrepareTask* pfnP, FnExecuteTask* pfnE);

	struct ThreadData
	{
//		int frame;
		double prevTime;
		void* userThreadLocal;
	};

	struct FrameData
	{
		int frame;
		double time;
		void* userFrame;
	};
	
	static void* s_InitThread(Scope& thread, uint idx, TaskLoop& loop, TaskLoop::Config* cfg);
	void* PrepareTask(Scope& a, Scope& frame, void* thread, const void* prevFrame);
	void ExecuteTask(Scope& a, Scope& frame, void* threadData, void* taskData);
	
	Timer& m_timer;
	void* m_user;
	FnAllocThread* m_allocThreadLocal;
	FnPrepareTask* m_prepareTask;
	FnExecuteTask* m_executeTask;
};

//------------------------------------------------------------------------------
//#include ".hpp"
} // namespace eight
//------------------------------------------------------------------------------
