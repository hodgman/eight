#include <eight/core/thread/fifo_mpmc.h>
#include <eight/core/thread/pool.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/test.h>
#include <malloc.h>
#include <stdio.h>

using namespace eight;

namespace {

struct TestData
{
	FifoMpmc<int>* queue;
	Atomic result;
	Atomic numInvoked;
	Atomic invoked[16];
};

//const static int loops = 0x7fffffff/1600;
const static int loops = 0x1000;
const static int loopTotal = 0;
const static int s_numThreads = 4;

int ThreadEntry( void* arg, ThreadId& thread, uint systemId )
{
	const uint threadIndex = thread.ThreadIndex();
	const uint numThreads = thread.NumThreadsInPool();
	TestData& data = *(TestData*)arg;
	FifoMpmc<int>& queue = *data.queue;

	eiASSERT( threadIndex >= 0 && threadIndex < s_numThreads );
	eiASSERT( numThreads > 0 && numThreads <= s_numThreads );
	data.invoked[threadIndex] = 1;
	++data.numInvoked;

	const int testSize = 6;
	int testData[testSize] = { 1, -1, 900, 42, -900, -42 };

	int total = 0;

	int popped;
	for( int i=0 ; i < loops; ++i )
	{
		for( int j=0 ; j != testSize; ++j )
		{
			while( !queue.Push(testData[j] ) )
			{
				if( queue.Pop(popped) )
				{
					total += popped;
				}
			}
		}
	}

	for( int i=0 ; i < 0xff; ++i )
	while( queue.Pop(popped) )
	{
		total += popped;
	}

	data.result += total;

	return 0;
}

}//end namespace

eiTEST(FifoMpmc)
{
	MallocStack buffer( 1024000 );
	Scope a( buffer.stack, "test1" );
	FifoMpmc<int> queue( a, 100 );
	for( int i =0; i<1/*0000*/; ++i )
	{
//		printf( "attempt #%d\n", i );

		TestData data = { &queue, Atomic(42) };

		{
			Scope b( a, "test2" );
			StartThreadPool( b, &ThreadEntry, &data, s_numThreads );
		}

		eiRASSERT( data.numInvoked == s_numThreads );
		for( int i=0; i!=s_numThreads; ++i )
		{
			eiRASSERT( data.invoked[i] == 1 );
		}

		eiRASSERT( queue.Empty() );
		eiRASSERT( data.result == 42 + loops * loopTotal * s_numThreads );
	}
}