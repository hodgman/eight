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
class FifoMpmc : NonCopyable
{
public:
	FifoMpmc( Scope&, uint size );//size must be greater than 1

	bool Push( const T& );
	bool Pop( T& );
	bool Empty() const;
private:
	FifoMpmc();
	struct Node
	{
		Atomic state;
		T data;
	};
	Node* m_data;
	const uint m_size;
	u32 Next( u32 );
	Atomic m_read, m_write;

	const static u32 freeForWrite = 0;
	const static u32 allocForWrite = 1;
	const static u32 freeForRead = 2;
	const static u32 allocForRead = 3;
	const static u32 stateMask = 3;
	const static u32 abaShift = 2;
};

#include "fifo_mpmc.hpp"

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
