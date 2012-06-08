//------------------------------------------------------------------------------
#pragma once
#include <eight/core/noncopyable.h>
namespace eight {
//------------------------------------------------------------------------------
class Scope;

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
