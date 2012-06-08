//------------------------------------------------------------------------------
#include <eight/core/test.h>
#include <eight/core/debug.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/timer/timer_impl.h>
#include <eight/core/thread/atomic.h>

#define eiFILE_DERIVED_CLASS TimerImpl
#define eiDowncast      (*static_cast<eiFILE_DERIVED_CLASS*>(this))
#define eiDowncastConst (*static_cast<const eiFILE_DERIVED_CLASS*>(this))

using namespace eight;
//------------------------------------------------------------------------------

static eight::Atomic g_TimerSystemInit;

TimerImpl::TimerImpl()
{
	Timer::InitSystem();
	eiASSERT( g_TimerSystemInit );
	timer_initialize( &data );
}

TimerImpl::~TimerImpl()
{
	Timer::ShutdownSystem();
}

void Timer::InitSystem()
{
	if( 0 == g_TimerSystemInit++ )
		timer_lib_initialize();
	//todo - init/shutdown here supposed to be able to be stupid-proof, allowing user to init multiple times.
	//       not actually safe to use this feature yet:
	//thread A: g_TimerSystemInit++  -> true
	//thread B: g_TimerSystemInit++  -> false
	//thread B: return
	//thread B: ...goes on to use timer functions...
	//thread A: timer_lib_initialize(); -- too late
}

void Timer::ShutdownSystem()
{
	eiASSERT( g_TimerSystemInit );
	if( --g_TimerSystemInit == 0 )
		timer_lib_shutdown();
}

void Timer::Reset()
{
	timer& data = eiDowncast.data;
	timer_reset( &data );
}

double Timer::Elapsed() const
{
	const timer& data = eiDowncastConst.data;
	return timer_elapsed( (timer*)&data, 0 );
} 

double Timer::Elapsed(bool reset)
{
	timer& data = eiDowncast.data;
	return timer_elapsed( &data, (int)reset );
}

//------------------------------------------------------------------------------
