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

template<class T>
class AtomicPtr
{
public:
	explicit AtomicPtr( T* p0 );
	AtomicPtr( const AtomicPtr& );
	AtomicPtr& operator=( const AtomicPtr& );
	AtomicPtr& operator=( T* );
	operator T*();

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
