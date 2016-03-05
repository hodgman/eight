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

	T*   Push( const T& );//NULL indicates failure. Don't read/write from/to the returned pointer unless you're sure it hasn't yet been popped via external logic... 
	bool Pop( T& );
	bool Empty() const;//only a hint. Even if you get a false result, someone might have pushed again simultaneously.

	uint Capacity() const { return m_size; }

	T* PeekSC();//not safe with multiple consumers. Only use in a MPSC/SPSC setting.
	bool Peek( T*&, uint ahead=0 );//not entirely safe, as value may be concurrently popped. User must use appropriate lifetime management.
private:
	FifoMpmc();
	struct Node
	{
		Atomic state;
		T data;
	};
	Atomic m_read;
	u64 pad1[7];
	Node* m_data;
	const uint m_size;
	u32 Next( u32 );
	u64 pad2[7];
	Atomic m_write;
	u64 pad3[7];

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
