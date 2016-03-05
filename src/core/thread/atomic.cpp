#include <eight/core/thread/atomic.h>
#include <eight/core/thread/pool.h>
#include <eight/core/thread/tasksection.h>
#include <eight/core/profiler.h>

#include <eight/core/os/win32.h>
#include <Winnt.h>
#include <intrin.h>
#pragma intrinsic(_WriteBarrier)
#pragma intrinsic(_ReadBarrier)
#pragma intrinsic(_ReadWriteBarrier)

using namespace eight;

#define eiALIGNED( ptr, value ) ( (((u32)ptr) & (value-1)) == 0 )

typedef volatile LONG* AtomicIntPtr;

int BusyWait<true >::defaultSpin = 1;
int BusyWait<false>::defaultSpin = 150;

eiInfoGroup( Yield, false );

void eight::WriteBarrier()
{
	_WriteBarrier();
	MemoryBarrier();
}
void eight::ReadBarrier()
{
	_ReadBarrier();
	MemoryBarrier();
}
void eight::ReadWriteBarrier()
{
	_ReadWriteBarrier();
	MemoryBarrier();
}
void eight::YieldHardThread()
{
//	eiInfo(Yield, "HardThread");
	YieldProcessor();
}
void eight::YieldSoftThread()
{
//	eiProfile("YieldSoftThread");
//	eiInfo(Yield, "SoftThread");
	SwitchToThread();
}
void eight::YieldToOS(bool sleep)
{
//	eiProfile( sleep ? "YieldToOS(1)" : "YieldToOS(0)" );
//	eiInfo(Yield, sleep?"Sleep(1)":"Sleep(0)");
//	Sleep(sleep ? DWORD(1) : DWORD(0));
	SleepEx(sleep ? DWORD(1) : DWORD(0), TRUE);
}


bool eight::WaitForWakeEvent( const ThreadId& thread )
{
	return thread.Jobs().WaitForWakeEvent();
}

void eight::FireWakeEvent( const ThreadId& thread )
{
	thread.Jobs().FireWakeEvent( thread.NumThreadsInPool() );
}

void eight::AtomicWrite(u32* out, u32 in)
{
	InterlockedExchange( (AtomicIntPtr)out, (LONG)in );
}

//N.B. Plain load has acquire semantics on x86, plain store has release semantics
Atomic::Atomic( s32 i )
{
	eiASSERT( eiALIGNED(&value,4) );
	InterlockedExchange( (AtomicIntPtr)&value, (LONG)i );
}
Atomic::Atomic( const Atomic& other )
{
	eiASSERT( eiALIGNED(&value,4) );
	InterlockedExchange( (AtomicIntPtr)&value, (LONG)other.value );
}
Atomic& Atomic::operator=( const Atomic& other )
{
	eiASSERT( eiALIGNED(&value,4) );
	InterlockedExchange( (AtomicIntPtr)&value, (LONG)other.value );
	return *this;
}
Atomic::operator const s32() const
{
//	s32 i = value;
//	_ReadBarrier();
	s32 i;
	eiASSERT( eiALIGNED(&i,4) );
	InterlockedExchange( (AtomicIntPtr)&i, (LONG)value );
	return i;
}
Atomic& Atomic::operator=( s32 i )
{
	eiASSERT( eiALIGNED(&value,4) );
	InterlockedExchange( (AtomicIntPtr)&value, (LONG)i );
	return *this;
}
const s32 Atomic::operator++(int)
{
	return (s32)InterlockedExchangeAdd( (AtomicIntPtr)&value, 1 );//Returns original value before addition
}
const s32 Atomic::operator--(int)
{
	return (s32)InterlockedExchangeAdd( (AtomicIntPtr)&value, -1 );//Returns original value before subtraction
}

const s32 Atomic::operator++()
{
	eiASSERT( eiALIGNED(&value,4) );
	return InterlockedIncrement( (AtomicIntPtr)&value );//returns new value after incrementing
}
const s32 Atomic::operator--()
{
	eiASSERT( eiALIGNED(&value,4) );
	return InterlockedDecrement( (AtomicIntPtr)&value );//returns new value after decrementing
}
/*
void Atomic::Increment( s32& before, s32& after )
{
	s32 local;
	do
	{
		local = value;
	} while( SetIfEqual(local+1, local) );
	before = local;
	after = local+1;
}

void Atomic::Decrement( s32& before, s32& after )
{
	s32 local;
	do
	{
		local = value;
	} while( SetIfEqual(local-1, local) );
	before = local;
	after = local-1;
}*/

void Atomic::operator+=( s32 add )
{
	eiASSERT( eiALIGNED(&value,4) );
	InterlockedExchangeAdd( (AtomicIntPtr)&value, (LONG)add );//Returns value before subtraction
}
void Atomic::operator-=( s32 sub )
{ 
	eiASSERT( eiALIGNED(&value,4) );
	InterlockedExchangeAdd( (AtomicIntPtr)&value, -(LONG)sub );//Returns value before subtraction
}

bool Atomic::SetIfEqual( s32 newValue, s32 oldValue )
{
	eiASSERT( eiALIGNED(&value,4) );
	s32 result = (s32)InterlockedCompareExchange( (AtomicIntPtr)&value, (LONG)newValue, (LONG)oldValue );//returns value before the operation
	return (result == oldValue);
}
