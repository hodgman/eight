//------------------------------------------------------------------------------
#pragma once
#include <eight/core/thread/atomic.h>
#include <eight/core/debug.h>
#include <eight/core/types.h>
#include <eight/core/noncopyable.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T>
class FifoSpsc : NonCopyable
{
public:
	FifoSpsc( Scope&, uint size );//size must be greater than 1

	bool Push( const T& );
	bool Pop( T& );
private:
	FifoSpsc();
	T* m_data;
	const uint m_size;
	u32 Next( u32 );
	Atomic m_read, m_write;
};

#include "fifo_spsc.hpp"

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
