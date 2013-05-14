//------------------------------------------------------------------------------
#include <eight/core/test.h>
#include <eight/core/debug.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/timer/timer.h>
#include <eight/core/thread/atomic.h>
#include <timer_lib/timer.h>

using namespace eight;
//------------------------------------------------------------------------------

class TimerImpl
{
public:
	TimerImpl();
	~TimerImpl();
	void Reset();
	double Elapsed() const;
	double Elapsed(bool reset);
	timer data;
};

eiImplementInterface( Timer, TimerImpl );
eiInterfaceConstructor( Timer, () );
eiInterfaceFunction(      void, Timer, Reset, () );
eiInterfaceFunctionConst( double, Timer, Elapsed, () );
eiInterfaceFunction(      double, Timer, Elapsed, (a), bool a );

static eight::Atomic g_TimerSystemInit;

static void InitSystem()
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

static void ShutdownSystem()
{
	eiASSERT( g_TimerSystemInit );
	if( --g_TimerSystemInit == 0 )
		timer_lib_shutdown();
}

TimerImpl::TimerImpl()
{
	InitSystem();
	eiASSERT( g_TimerSystemInit );
	timer_initialize( &data );
}

TimerImpl::~TimerImpl()
{
	ShutdownSystem();
}

void TimerImpl::Reset()
{
	timer_reset( &data );
}

double TimerImpl::Elapsed() const
{
	return timer_elapsed( (timer*)&data, 0 );
} 

double TimerImpl::Elapsed(bool reset)
{
	return timer_elapsed( &data, (int)reset );
}

//------------------------------------------------------------------------------
