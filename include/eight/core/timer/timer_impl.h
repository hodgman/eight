//------------------------------------------------------------------------------
#pragma once
#include <eight/core/timer/timer.h>
#include <timer_lib/timer.h>
namespace eight {
//------------------------------------------------------------------------------

class TimerImpl : public Timer
{
public:
	friend class Timer;
	TimerImpl();
	~TimerImpl();
	timer data;
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
