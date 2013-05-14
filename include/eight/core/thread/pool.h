//------------------------------------------------------------------------------
#pragma once
#include "eight/core/types.h"
#include "eight/core/noncopyable.h"
namespace eight {
//------------------------------------------------------------------------------
class Scope;
class Atomic;

typedef int(FnPoolThreadEntry)( void* arg, uint threadIndex, uint numThreads, uint systemId );

void StartThreadPool( Scope&, FnPoolThreadEntry*, void* arg, int numWorkers=0 );

class JobPool : NoCreate
{
public:
	typedef void (FnJob)(void*, uint threadIndex, uint threadCount);
	struct Job
	{
		FnJob* fn;
		void* arg;
	};
	void PushJob( Job, uint threadIndex, uint threadCount );
	bool RunJob( uint threadIndex, uint threadCount );
};

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
JobPoolThread CurrentJobPool();

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------