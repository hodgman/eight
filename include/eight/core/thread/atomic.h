//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/debug.h>
namespace eight {
//------------------------------------------------------------------------------

void WriteBarrier();
void ReadBarrier();
void ReadWriteBarrier();

void YieldHardThread();         //Switch to another hardware thread (on the same core)
void YieldSoftThread();         //Switch to another thread
void YieldToOS(bool sleep=true);//Switch to the OS kernel. If sleep is true, will block for at least 1ms
template<class F> void YieldThreadUntil( F&, uint spin=0 );

struct BusyWait
{
	BusyWait() : sleep(), i(), j() {}
	uint i, j;
	bool sleep;
	static int defaultSpin;
	template<class F> inline bool Try( F& func, uint spin=0 )
	{
		spin = !spin ? defaultSpin : spin;
		if( func() )
			return true;
		if( j++ >= spin )
		{
			j = 0;
			if( i++ >= spin )
			{
				i = 0;
				YieldToOS( sleep );
				sleep = !sleep;
			}
			else
				YieldSoftThread();
		}
		else
			YieldHardThread();
		return false;
	}
};

struct WaitFor1;
struct WaitForTrue;
struct WaitForFalse;
struct WaitForValue;

void AtomicWrite(u32* out, u32 in);

class Atomic
{
public:
	Atomic( const Atomic& );
	Atomic& operator=( const Atomic& );
	explicit Atomic( s32 i=0 );
	operator const s32() const;
	Atomic& operator=( s32 );
	const s32 operator++();
	const s32 operator--();
	const s32 operator++(int);///< N.B. slower than prefix
	const s32 operator--(int);///< N.B. slower than prefix
	void Increment( s32& before, s32& after );
	void Decrement( s32& before, s32& after );
	void operator+=( s32 );
	void operator-=( s32 );

	bool SetIfEqual( s32 newValue, s32 oldValue );///< sets to new if currently equal to old. returns true if succeeded. N.B. can be misleading if ABA problem is applicable.
private:
	volatile s32 value;
};
eiSTATIC_ASSERT( sizeof(u32) == sizeof(Atomic) );

//------------------------------------------------------------------------------
#include "atomic.hpp"
} // namespace eight
//------------------------------------------------------------------------------
