
#include <eight/core/debug.h>
#include <eight/core/profiler.h>
#include <eight/core/alloc/scopearray.h>
#include <eight/core/thread/pool.h>
#include <eight/core/thread/fifo_mpmc.h>
#include <eight/core/thread/threadlocal.h>
#include <eight/core/timer/timer.h>
#include <eight/core/math/arithmetic.h>
#include <eight/core/sort/radix.h>

using namespace eight;

struct Profiler
{
	static bool PushEvent(const char* name)
	{
		if( m_threadEnabled == 0 )
			return false;
		int threadIndex, poolIndex;
		ThreadId* threadId = GetThreadId();
		if( threadId )
		{
			threadIndex = threadId->ThreadIndex();
			poolIndex = threadId->PoolId();
		}
		else
		{
			threadIndex = GetOsThreadId();
			poolIndex = -1;
		}
		Event e ={ (s16)(threadIndex & 0xFFFF), (s16)(poolIndex & 0xFFFF), name, m_timer->Elapsed(), 0, m_top };
		Event* pushed = m_events->Push(e);
		if( pushed )
		{
			pushed->self = pushed;
			m_top = pushed;
		}
		return !!pushed;
	}
	static void PopEvent()
	{
		Event* top = m_top;
		eiASSERT(top);
		eiASSERT(top->stampEnd == 0);
		top->stampEnd = m_timer->Elapsed();
		eiASSERT(top->stampEnd != 0);
		m_top = top->parent;
	}

	struct Event
	{
		s16 tid;
		s16 pid;
		const char* name;
		double stampBegin;
		double stampEnd;
		Event* parent;
		Event* self;
		bool operator==(const Event& o) { return name==o.name && stampBegin==o.stampBegin && stampEnd==o.stampEnd && parent==o.parent; }
	};

	static double m_startTime;
	static Atomic m_enabled;
	static Timer* m_timer;
	static FifoMpmc<Event>* m_events;
	static ThreadLocalStatic<Event, Event> m_top;
	static ThreadLocalStatic<int, Profiler> m_threadEnabled;
};
double Profiler::m_startTime = 0;
Timer* Profiler::m_timer = 0;
Atomic Profiler::m_enabled;
FifoMpmc<Profiler::Event>* Profiler::m_events = 0;
ThreadLocalStatic<Profiler::Event, Profiler::Event> Profiler::m_top;
ThreadLocalStatic<int, Profiler> Profiler::m_threadEnabled;

void eight::InitProfiler(Scope& a, uint maxEvents)
{
	eiASSERT( Profiler::m_timer == 0 );
	Profiler::m_timer = eiNewInterface(a, Timer);
	Profiler::m_events = eiNew(a, FifoMpmc<Profiler::Event>)( a, maxEvents );
	Profiler::m_enabled = 0;
}

bool eight::PushProfileScope(const char* name)
{
	return Profiler::PushEvent(name);
}

void eight::PopProfileScope()
{
	Profiler::PopEvent();
}

bool eight::ProfileBegin()
{
	Profiler::m_threadEnabled = (int*)1;
	if( Profiler::m_enabled.SetIfEqual(1,0) )
	{
		Profiler::m_startTime = Profiler::m_timer->Elapsed();
		return true;
	}
	return false;
}
void eight::ProfileEnd()
{
	Profiler::m_threadEnabled = (int*)0;
}
void eight::ProfileEnd(const ProfileCallback& user, Scope& a_)
{
	Profiler::m_threadEnabled = (int*)0;
	if( !Profiler::m_enabled.SetIfEqual(2,1) )
		return;
	Scope temp(a_, "temp");

	double frameOrigin = Profiler::m_startTime;

	if( user.beginEvents )
		user.beginEvents( user.data, false );

	if( user.event )
	{
		Profiler::Event* buffer = eiAllocArray(temp, Profiler::Event, Profiler::m_events->Capacity());
		uint eventCount = 0;
		while( Profiler::Event* current = Profiler::m_events->PeekSC() )
		{
			YieldThreadUntil( WaitForTrue64(current->stampEnd) );
			bool ok = Profiler::m_events->Pop(buffer[eventCount]);
			eiASSERT( ok && buffer[eventCount] == *current );
			++eventCount;
			eiASSERT( eventCount <= Profiler::m_events->Capacity() );
		}
		eiASSERT( Profiler::m_events->Empty() );

		Profiler::Event* buffer2 = eiAllocArray(temp, Profiler::Event, eventCount);

		struct
		{
			typedef int Type;
			Type operator()( Profiler::Event* in ) const { return in->pid; }
		} sortByProcess; 
		Profiler::Event* sorted = RadixSort(buffer, buffer2, eventCount, sortByProcess);
		eiASSERT( sorted == buffer );

		struct
		{
			typedef int Type;
			Type operator()( Profiler::Event* in ) const { return in->tid; }
		} sortByThread; 
		sorted = RadixSort(buffer, buffer2, eventCount, sortByThread);
		eiASSERT( sorted == buffer );

		int parity = 0;
		uint nestCount = 0;
		Profiler::Event* nest[256];
		for( uint i=0, end=eventCount; i!=end; ++i )
		{
			Profiler::Event& e = buffer[i];
			while( nestCount && nest[nestCount-1]->self != e.parent)
			{
				Profiler::Event& prev = *nest[--nestCount];
				double endTime = prev.stampEnd - frameOrigin;
				user.event( user.data, prev.tid, prev.pid, prev.name, endTime, ProfileCallback::End );
				--parity;
			}
			user.event(user.data, e.tid, e.pid, e.name, e.stampBegin - frameOrigin, ProfileCallback::Begin);
			++parity;
			if( i+1 < end && buffer[i+1].parent == e.self )
			{
				nest[nestCount++] = &e;
				eiASSERT( nestCount < 256 );
			}
			else
			{
				double endTime = e.stampEnd - frameOrigin;
				user.event( user.data, e.tid, e.pid, e.name, endTime, ProfileCallback::End );
				--parity;
			}
		}
		while( nestCount )
		{
			Profiler::Event& prev = *nest[--nestCount];
			double endTime = prev.stampEnd - frameOrigin;
			user.event( user.data, prev.tid, prev.pid, prev.name, endTime, ProfileCallback::End );
			--parity;
		}
		eiASSERT( parity == 0 );
	}

	if( user.endEvents )
		user.endEvents( user.data );

	eiASSERT( Profiler::m_events->Empty() );

#if eiDEBUG_LEVEL > 1
	eiASSERT( Profiler::m_enabled.SetIfEqual(0,2) );
#else
	eiASSERT( Profiler::m_enabled == 2 );
	Profiler::m_enabled = 0;
#endif
}