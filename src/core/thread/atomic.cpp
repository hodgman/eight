#include <eight/core/thread/atomic.h>

#include <eight/core/os/win32.h>
#include <Winnt.h>
#include <intrin.h>
#pragma intrinsic(_WriteBarrier)
#pragma intrinsic(_ReadBarrier)
#pragma intrinsic(_ReadWriteBarrier)

using namespace eight;

uint eight::g_defaultSpin = 500;

#define eiALIGNED( ptr, value ) ( (((u32)ptr) & (value-1)) == 0 )

typedef volatile LONG* AtomicPtr;

int BusyWait::defaultSpin = 5;

eiInfoGroup( Yield, true );

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
//	eiInfo(Yield, "SoftThread");
	SwitchToThread();
}
void eight::YieldToOS(bool sleep)
{
	eiInfo(Yield, sleep?"Sleep(1)":"Sleep(0)");
	Sleep(sleep ? DWORD(1) : DWORD(0));
}

void eight::AtomicWrite(u32* out, u32 in)
{
	InterlockedExchange( (AtomicPtr)out, (LONG)in );
}

//N.B. Plain load has acquire semantics on x86, plain store has release semantics
Atomic::Atomic( s32 i )
{
	eiASSERT( eiALIGNED(&value,4) );
	InterlockedExchange( (AtomicPtr)&value, (LONG)i );
}
Atomic::Atomic( const Atomic& other )
{
	eiASSERT( eiALIGNED(&value,4) );
	InterlockedExchange( (AtomicPtr)&value, (LONG)other.value );
}
Atomic& Atomic::operator=( const Atomic& other )
{
	eiASSERT( eiALIGNED(&value,4) );
	InterlockedExchange( (AtomicPtr)&value, (LONG)other.value );
	return *this;
}
Atomic::operator const s32() const
{
//	s32 i = value;
//	_ReadBarrier();
	s32 i;
	eiASSERT( eiALIGNED(&i,4) );
	InterlockedExchange( (AtomicPtr)&i, (LONG)value );
	return i;
}
Atomic& Atomic::operator=( s32 i )
{
	eiASSERT( eiALIGNED(&value,4) );
	InterlockedExchange( (AtomicPtr)&value, (LONG)i );
	return *this;
}
const s32 Atomic::operator++(int)
{
	return (s32)InterlockedExchangeAdd( (AtomicPtr)&value, 1 );//Returns original value before addition
}
const s32 Atomic::operator--(int)
{
	return (s32)InterlockedExchangeAdd( (AtomicPtr)&value, -1 );//Returns original value before subtraction
}

const s32 Atomic::operator++()
{
	eiASSERT( eiALIGNED(&value,4) );
	return InterlockedIncrement( (AtomicPtr)&value );//returns new value after incrementing
}
const s32 Atomic::operator--()
{
	eiASSERT( eiALIGNED(&value,4) );
	return InterlockedDecrement( (AtomicPtr)&value );//returns new value after decrementing
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
	InterlockedExchangeAdd( (AtomicPtr)&value, (LONG)add );//Returns value before subtraction
}
void Atomic::operator-=( s32 sub )
{ 
	eiASSERT( eiALIGNED(&value,4) );
	InterlockedExchangeAdd( (AtomicPtr)&value, -(LONG)sub );//Returns value before subtraction
}

bool Atomic::SetIfEqual( s32 newValue, s32 oldValue )
{
	eiASSERT( eiALIGNED(&value,4) );
	s32 result = (s32)InterlockedCompareExchange( (AtomicPtr)&value, (LONG)newValue, (LONG)oldValue );//returns value before the operation
	return (result == oldValue);
}
