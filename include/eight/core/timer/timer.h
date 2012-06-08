//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/noncopyable.h>
#include <eight/core/alloc/new.h>
namespace eight {
//------------------------------------------------------------------------------
class Scope;

eiConstructAs(Timer, TimerImpl);

class Timer : NonCopyable
{
public:
	static void InitSystem();
	static void ShutdownSystem();

	void Reset();
	double Elapsed() const;
	double Elapsed(bool reset = false);
protected:
	 Timer(){}
	~Timer(){}
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
