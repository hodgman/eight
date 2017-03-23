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
	ScopeLock(T& l, bool doJobs=true) : mutex(l) { mutex.Lock(doJobs); }
	~ScopeLock()                                 { mutex.Unlock(); }
private:
	T& mutex;
};

class Futex : NonCopyable
{
public:
	void Lock(bool doJobs=true);
	bool TryLock();
	void Unlock();

	eiDEBUG( ~Futex() { eiASSERT(value==0); } )
private:
	Atomic value;
};


inline void Futex::Lock(bool doJobs)
{
	//eiProfile("Futex::Lock");
	struct TryLock { TryLock( Futex* self ) : self(self) {} Futex* self;
		bool operator()()
		{
			return self->TryLock();
		}
	};
	YieldThreadUntil( TryLock(this), 0, doJobs );
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
