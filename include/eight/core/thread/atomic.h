//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/debug.h>
#include <eight/core/thread/pool.h>
#include <eight/core/profiler.h>
namespace eight {
class ThreadId;
//------------------------------------------------------------------------------

void WriteBarrier();
void ReadBarrier();
void ReadWriteBarrier();

void YieldHardThread();         //Switch to another hardware thread (on the same core)
void YieldSoftThread();         //Switch to another thread
void YieldToOS(bool sleep=true);//Switch to the OS kernel. If sleep is true, will likely block for at least 1ms
template<class F> void YieldThreadUntil( F&, uint spin=0, bool doJobs=true, bool willFireWakeEvent=false );

bool WaitForWakeEvent( const ThreadId& );
void FireWakeEvent( const ThreadId& );

template<bool WillFireWakeEvent>
struct BusyWait
{
	BusyWait( bool doJobs = true )
		: info( (doJobs || WillFireWakeEvent) ? GetThreadId() : 0 )
		, i()
		, j()
		, sleep()
	{
		eiASSERT( !WillFireWakeEvent || info );
	}
	ThreadId* info;
	uint i, j;
	bool sleep;
	static int defaultSpin;
	template<class F> inline bool Try( F& func, uint spin=0 )
	{
		if( func() )
			return true;
		Spin(spin);
		return false;
	}
	inline void Spin(uint spin=0)
	{
		JobPool* jobPool = info ? &info->Jobs() : 0;
		spin = !spin ? defaultSpin : spin;
		if( j >= spin )
		{
			j = 0;
			if( i++ >= spin )
			{
				i = 0;
				if( !jobPool || !jobPool->RunJob( *info ) )
				{
					if( WillFireWakeEvent && sleep )
						WaitForWakeEvent( *info );
					else
						YieldToOS( sleep );
					sleep = !sleep;
				}
				else if( WillFireWakeEvent )
				{
					for( int k=0; k!=4 && jobPool->RunJob( *info ); ++k ){};
				}
			}
			else
			{
				if( !jobPool || !jobPool->RunJob( *info ) )
					YieldSoftThread();
				else if( WillFireWakeEvent )
				{
					for( int k=0; k!=4 && jobPool->RunJob( *info ); ++k ){};
				}
			}
		}
		else if( !jobPool || !jobPool->RunJob( *info ) )
		{
			YieldHardThread();
			++j;
		}
		else if( WillFireWakeEvent )
		{
			for( int k=0; k!=4 && jobPool->RunJob( *info ); ++k ){};
		}
	}
};

struct WaitFor1;
struct WaitForTrue;
struct WaitForFalse;
struct WaitForValue;
struct WaitForTrue64;

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

template<class T>
class AtomicPtr
{
public:
	AtomicPtr(){}
	explicit AtomicPtr( T* );
	AtomicPtr( const AtomicPtr& );
	AtomicPtr& operator=( const AtomicPtr& );
	AtomicPtr& operator=( T* );
	operator T*();
	operator const Atomic&() const { return data; }

	bool SetIfEqual( T* newValue, T* oldValue );///< sets to new if currently equal to old. returns true if succeeded. N.B. can be misleading if ABA problem is applicable.
private:
	Atomic data;
};
eiSTATIC_ASSERT( sizeof(void*) == sizeof(Atomic) );

template<class T> AtomicPtr<T>::AtomicPtr( T* p ) : data(*(s32*)&value)
{
}
template<class T> AtomicPtr<T>::AtomicPtr( const AtomicPtr& other ) : data(other.data)
{
}
template<class T> AtomicPtr<T>& AtomicPtr<T>::operator=( const AtomicPtr& other )
{
	data = other.data;
	return *this;
}
template<class T> AtomicPtr<T>& AtomicPtr<T>::operator=( T* value )
{
	data = *(s32*)&value;
	return *this;
}
template<class T> bool AtomicPtr<T>::SetIfEqual( T* newValue, T* oldValue )
{
	return data.SetIfEqual( *(s32*)&newValue, *(s32*)&oldValue );
}
template<class T> AtomicPtr<T>::operator T*()
{
	s32 value = data;
	return *(T**)&value;
}

//------------------------------------------------------------------------------
#include "atomic.hpp"
} // namespace eight
//------------------------------------------------------------------------------
