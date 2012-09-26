//------------------------------------------------------------------------------
#pragma once
#include "eight/core/types.h"
namespace eight {
//------------------------------------------------------------------------------
class Scope;
class Atomic;

typedef int(FnPoolThreadEntry)( void* arg, uint threadIndex, uint numThreads, uint systemId );

void StartThreadPool( Scope&, FnPoolThreadEntry*, void* arg, int numWorkers=0 );

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

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------