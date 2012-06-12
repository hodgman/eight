
#include <eight/core/alloc/new.h>
#include <eight/core/debug.h>
#include <eight/core/thread/timedloop.h>
#include <eight/core/thread/pool.h>
#include <eight/core/timer/timer_impl.h>
#include <eight/core/id.h>
#include <eight/core/bind.h>
#include <eight/core/message.h>

using namespace eight;

eiBindClass( TimedLoop )
	eiBeginData()
		eiBindReference( m_timer ) //todo
		eiBindData( m_user )
		eiBindData( m_allocThreadLocal )
		eiBindData( m_prepareTask )
		eiBindData( m_executeTask )
	eiEndData();
eiEndBind();

TimedLoop::TimedLoop(Scope& a, void* user, FnPrepareTask* pfnP, FnExecuteTask* pfnE)
	: m_timer(*eiNew(a, Timer))
	, m_prepareTask(pfnP)
	, m_executeTask(pfnE)
	, m_user(user)
{}

void TimedLoop::Start( Scope& scope, void* user, FnAllocThread* pfnA, FnPrepareTask* pfnP, FnExecuteTask* pfnE )
{
	Scope a( scope, "stack" );
	Args args = { &a, user, pfnA, pfnP, pfnE };
	TaskLoop* loop = eiNew(a, TaskLoop)( a, &args, &TimedLoop::s_InitThread, eiMiB(128), eiMiB(1) );
	StartThreadPool( a, &TaskLoop::ThreadMain, loop );//calls eiNew(a, TimedLoop) in s_InitThread
}

void* TimedLoop::s_InitThread(Scope& thread, uint idx, TaskLoop& loop, TaskLoop::Config* cfg)
{
	Args& args = *(Args*)cfg->user;
	if( idx == 0 )
	{
		Scope& a = *args.a;
		args.result      = eiNew(a, TimedLoop)(a, args.user, args.pfnP, args.pfnE);
		cfg->userPrepare = &s_PrepareTask;
		cfg->userTask    = &s_Task;
	}
	ThreadData* data = eiAlloc( thread, ThreadData );
//		data->frame = 0;
	data->prevTime = 0;
	data->userThreadLocal = args.pfnA( thread, args.user );
	return data;
}

void* TimedLoop::PrepareTask(Scope& a, Scope& frame, void* thread, const void* prevFrame)
{
	ThreadData& data = *(ThreadData*)thread;
	FrameData* frameData = eiAlloc(frame, FrameData);
	frameData->time = m_timer.Elapsed();
	frameData->frame = prevFrame ? ((FrameData*)prevFrame)->frame + 1 : 0;
	uint worker = CurrentPoolThread().index;
	double elapsed = frameData->time;
	double dt = prevFrame ? elapsed - ((FrameData*)prevFrame)->time : 0;
	eiInfo(TimedLoop, "%d P> %d, %.9f, %.9f, %.0ffps %s", worker, frameData->frame, m_timer.Elapsed(), elapsed, 1/dt, worker==0?"*":"");
	bool done = frameData->time >= 1;
	if(done)
	{
		return 0;
	}
	frameData->userFrame = m_prepareTask( a, frame, m_user, data.userThreadLocal );
	return frameData;
}
void TimedLoop::ExecuteTask(Scope& a, Scope& frame, void* threadData, void* taskData)
{
	ThreadData& data = *(ThreadData*)threadData;
	FrameData& frameData = *(FrameData*)taskData;

	int f = frameData.frame;

	double elapsed = frameData.time;
	double dt = elapsed - data.prevTime;
	data.prevTime = elapsed;
	uint worker = CurrentPoolThread().index;
//	if( worker == 0 && f % 100 == 0 )
	eiInfo(TimedLoop, "%d E> %d, %.9f, %.9f, %.0ffps %s", worker, f,  m_timer.Elapsed(), elapsed, 1/dt, worker==0?"*":"");

	m_executeTask( a, frame, m_user, data.userThreadLocal, frameData.userFrame );
}

/*

int* PlusOne( int* input, int count )
{
	int temp = 
}


list<MemoryBlock> g_heap;

void test( void* stack, int a )
{
}



int g_referenceCounts[numThreads][maxObjects] = {};

int g_maybeGarbageCount[numThreads] = {};
int g_maybeGarbageIds[numThreads][bufferSize];



void AddRef( int id )
{
	++g_referenceCounts[ThreadId()][id];
}

void DecRef( int id )
{
	--g_referenceCounts[ThreadId()][id];

	int slot = g_maybeGarbageCount[ThreadId()]++;
	g_maybeGarbageIds[ThreadId()][slot] = id;
}


int CollectGarbage()
{
	int maybeGarbageCount = 0;
	int* maybeGarbageIds = Merge( g_maybeGarbageIds, g_maybeGarbageCount, NumThreads(), &maybeGarbageCount );
	maybeGarbageIds = SortAndRemoveDuplicates( maybeGarbageIds, &maybeGarbageCount );
	for( int i=0; i!=maybeGarbageCount; ++i )
	{
		int id = maybeGarbageIds[i];
		int count = 0;
		for( int t=0, end=NumThreads(); t!=end; ++t )
		{
			count += g_referenceCounts[t][id];
		}
		if( count == 0 )
			toDestruct.append(id);
	}
}*/