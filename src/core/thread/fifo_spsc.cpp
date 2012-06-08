#include <eight/core/thread/fifo_spsc.h>
#include <eight/core/thread/pool.h>
#include <eight/core/alloc/stack.h>
#include <eight/core/test.h>
#include <malloc.h>

using namespace eight;

eiTEST(FifoSpsc)
{
	void* buffer = alloca( 1024 );
	StackAlloc stack( buffer, 1024 );
	Scope a( stack, "test" );

	bool ok;
	FifoSpsc<int> queue( a, 4 );
	ok = queue.Push( 42 );
	eiRASSERT( ok );
	int fourtyTwo;
	ok = queue.Pop( fourtyTwo );
	eiRASSERT( ok );
	eiRASSERT( fourtyTwo == 42 );
}