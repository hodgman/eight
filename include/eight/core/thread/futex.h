//------------------------------------------------------------------------------
#pragma once
#include <eight/core/thread/atomic.h>
#include <eight/core/debug.h>
#include <eight/core/noncopyable.h>
#include <eight/core/profiler.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T>
class ScopeLock : NonCopyable
{
public:
	ScopeLock(T& l) : mutex(l) { mutex.Lock(); }
	~ScopeLock()               { mutex.Unlock(); }
private:
	T& mutex;
};

class Futex : NonCopyable
{
public:
	void Lock();
	bool TryLock();
	void Unlock();

	eiDEBUG( ~Futex() { eiASSERT(value==0); } )
private:
	Atomic value;
};


inline void Futex::Lock()
{
	//eiProfile("Futex::Lock");
	struct TryLock { TryLock( Futex* self ) : self(self) {} Futex* self;
		bool operator()()
		{
			return self->TryLock();
		}
	};
	YieldThreadUntil( TryLock(this) );
}

inline bool Futex::TryLock()
{
	return value.SetIfEqual(1,0);
}

inline void Futex::Unlock()
{
	eiASSERT( value == 1 );
	value = 0;
}

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
