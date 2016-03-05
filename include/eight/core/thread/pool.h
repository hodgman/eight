//------------------------------------------------------------------------------
#pragma once
#include "eight/core/types.h"
#include "eight/core/noncopyable.h"
namespace eight {
//------------------------------------------------------------------------------
class Scope;
class Atomic;
class JobPool;

class ThreadId : NoCreate
{
public:
	u32 ThreadIndex() const;
	u32 NumThreadsInPool() const;
	JobPool& Jobs() const;
	u32 PoolId() const;
};

typedef int(FnPoolThreadEntry)( void* arg, ThreadId&, uint systemId );

JobPool* StartBackgroundThreadPool( Scope&, FnPoolThreadEntry*, void* arg, int numWorkers=0, Atomic* runningCount=0 );
void StartThreadPool( Scope&, FnPoolThreadEntry*, void* arg, int numWorkers=0, Atomic* runningCount=0 );

class JobPool : NoCreate
{
public:
	typedef void (FnJob)( void*, ThreadId& );
	struct Job
	{
		FnJob* fn;
		void* arg;
	};
	struct JobList
	{
		Job* jobs;
		uint numJobs;
		Atomic* nextToExecute;
	};
	void PushJob( Job );
	void PushJob( Job, ThreadId& );
	void PushJobList( JobList, ThreadId& );
	bool RunJob( ThreadId& );

	bool WaitForWakeEvent();
	void FireWakeEvent(uint threadCount);

	u32 Id() const;
};

bool InPool();
ThreadId* GetThreadId();
uint GetOsThreadId();

/*
struct PoolThread
{
	u32 mask;
	u32 index;
	u32 poolSize;
	u32 poolMask;
};
bool InPool();
PoolThread CurrentPoolThread();
uint       CurrentPoolSize();

struct JobPoolThread
{
	JobPool* jobs;
	uint threadIndex;
	uint threadCount;
};
JobPoolThread CurrentJobPool();*/

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------