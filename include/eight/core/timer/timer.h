//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/noncopyable.h>
#include <eight/core/alloc/interface.h>
namespace eight {
//------------------------------------------------------------------------------
class Scope;

class Timer : Interface<Timer>
{
public:
	Timer();
	void Reset();
	double Elapsed() const;
	double Elapsed(bool reset = false);
private:
	~Timer();
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
