//------------------------------------------------------------------------------
#pragma once
#include <eight/core/noncopyable.h>
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------
class Scope;

uint NumberOfHardwareThreads();
uint NumberOfPhysicalCores(Scope& temp);

typedef int(*ThreadEntry)(void*, int systemId);

class Thread : NonCopyable
{
public:
	Thread(Scope&, ThreadEntry, void* arg);
	~Thread();
private:
	void* pimpl;
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
