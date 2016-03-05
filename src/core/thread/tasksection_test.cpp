#include <eight/core/thread/tasksection.h>
#include <eight/core/thread/thread.h>
#include <eight/core/thread/pool.h>
#include <eight/core/test.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/macro.h>
#include <eight/core/timer/timer.h>
#include <eight/core/profiler.h>
#include <math.h>
#include <stdio.h>
using namespace eight;

//todo - cleanup
extern ProfileCallback g_JsonProfileOut;
extern int g_profilersRunning;
extern int g_profileFramesLeft;

namespace
{
#if eiDEBUG_LEVEL != 0
	static const int stress = 1;
	static const int s_itemsPerWorker = 1*stress;
#else
	static const int stress = 3;
	static const int s_itemsPerWorker = 100*stress;
#endif
	static const int s_workers = NumberOfHardwareThreads()-1;
	static const int s_tasks = 2;

	struct WaitForTrue
	{
		WaitForTrue( const Atomic& a ) : a(a) {} const Atomic& a; 
		bool operator()() const { return a!=0; }
	};

	struct TestTasks
	{
		TestTasks() : task1(), task2(), criticalTask( s_workers*2 ) {}
		TaskSection task1;
		TaskSection task2;
		Semaphore criticalTask;
	};

	struct TaskTestContext
	{
		TaskTestContext() : initialTask(), tasks(), results(), criticalCount(0), alloc(), timer() { eiRASSERT( s_workers <= 32 ); }
		SingleThread initialTask;
		Atomic initialized;
		Atomic working;
		TestTasks* tasks;
		int* results;
		int criticalCount;
		int criticalData[10*stress*64];
		Scope* alloc;
		Timer* timer;
	};

	int TestTaskSections( void* arg, ThreadId& thread, uint systemId )
	{
		const uint numThreads = thread.NumThreadsInPool();
		const uint threadIndex = thread.ThreadIndex();

		eiUsingTaskSections;
		const int fakeWork = 0x40;
		eiRASSERT( numThreads == s_workers );
		eiRASSERT( _ei_thread_id->index == threadIndex );
		eiRASSERT( _ei_thread_id->mask == 1<<threadIndex );
		eiRASSERT( _ei_thread_id->poolSize == numThreads );

		TaskTestContext& data = *(TaskTestContext*)arg;

		if( threadIndex == 0 )
			g_profilersRunning += 1;
		ProfileBegin();

		eiASSERT( data.timer );
		double beginTime = data.timer->Elapsed();

		eiProfile( "TestTaskSections" );
		for( int i=0; i!=10; ++i )
		{
			eiProfile( "Loop" );

			eiBeginSectionThread( data.initialTask );
			{
				{
					eiProfile( "Wait for last frame" );
					YieldThreadUntil( WaitForFalse( data.working ), 0, true, true );
				}
				eiProfile( "data.initialTask" );
				eiASSERT( data.initialized == i );
				eiASSERT( data.working == 0 );
				if( !data.tasks )
				{
					data.tasks = eiNew( *data.alloc, TestTasks )();
					uint items = s_itemsPerWorker*s_workers;
					for( int i=0; i!=items; ++i )
					{
						float a = 1;
						for( int j=0; j!=fakeWork; ++j ) { a = (sqrt( a*i+j+1 ) - a+(3.25f+a)+a*24.1131f+a)*0.001f; }
						//eiRASSERT( data.results[items*0 + i] == 0 );
						data.results[items*0 + i] = int( 1 + (a/(a+1))*0.000001 )*9000;
					}
					for( int i=items-1; i>=0; --i )
					{
						data.results[i] = 0;
					}
				}
				else
				{
					eiResetTaskSection( data.tasks->task1 );
					eiResetTaskSection( data.tasks->task2 );
					eiResetTaskSection( data.tasks->criticalTask );
				}
				data.working = numThreads;
				data.initialized = i+1;
				FireWakeEvent(thread);
			}
			eiEndSection( data.initialTask );
			
			{
				eiProfile( "Wait for init" );
				YieldThreadUntil( WaitForValue( data.initialized, i+1 ), 0, true, true );
			}

			if( threadIndex % 2 )
			{
				eiBeginSectionRedundant( data.tasks->task2 );
				{
					eiProfile( "data.tasks->task2" );
					uint items = s_itemsPerWorker*s_workers;
					uint begin, end = eiDistributeTask( items, begin, end );
					for( int i=begin; i!=end; ++i )
					{
						//	eiRASSERT( data.results[items*1 + i] == 0 );
						data.results[items*1 + i] = 2;
					}
				}
				eiEndSection( data.tasks->task2 );
			}

			eiBeginSectionTask( data.tasks->task1 );
			{
				eiProfile( "data.tasks->task1" );
				uint items = s_itemsPerWorker*s_workers;
				uint begin, end = eiDistributeTask( items, begin, end );
				for( int i=begin; i!=end; ++i )
				{
					float a = 1;
					for( int j=0; j!=fakeWork; ++j ) { a = (sqrt( a*i+j+1 ) - a+(3.25f+a)+a*24.1131f+a)*0.001f; }
				//	eiRASSERT( data.results[items*0 + i] == 0 );
					data.results[items*0 + i] = int( 1 + (a/(a+1))*0.000001 );
				}

				eiBeginSectionSemaphore( data.tasks->criticalTask );
				{
					eiProfile( "data.tasks->criticalTask" );
					data.criticalData[data.criticalCount++] = threadIndex;
				}
				eiEndSection( data.tasks->criticalTask );

				eiBeginSectionRedundant( data.tasks->task2 );
				{
					eiProfile( "data.tasks->task2" );
					uint items = s_itemsPerWorker*s_workers;
					uint begin, end = eiDistributeTask( items, begin, end );
					for( int i=begin; i!=end; ++i )
					{
						float a = 2;
						for( int j=0; j!=fakeWork; ++j ) { a = (sqrt( a*i+j+1 ) - a+(3.25f+a)+a*24.1131f+a)*0.001f; }
					//	eiRASSERT( data.results[items*1 + i] == 0 );
						data.results[items*1 + i] = int( 2 + (a/(a+1))*0.000001 );
					}
				}
				eiEndSection( data.tasks->task2 );
			}
			eiEndSection( data.tasks->task1 );

			eiBeginSectionSemaphore( data.tasks->criticalTask );
			{
				eiProfile( "data.tasks->criticalTask" );
				data.criticalData[data.criticalCount++] = threadIndex;
			}
			eiEndSection( data.tasks->criticalTask );

			eiWaitForSection( data.tasks->task1 );
			eiAssertTaskDone( data.tasks->task2 );
			eiWaitForSection( data.tasks->criticalTask );

			eiProfile( "loop sync" );
			if( 0 == --data.working )
				FireWakeEvent(thread);
		}

		eiASSERT( data.timer );
		double endTime = data.timer->Elapsed();
		printf( "%d - %.2fms\n", threadIndex, (endTime-beginTime)*1000 );

		eiProfile( "Wait for shutdown" );
		YieldThreadUntil( WaitForFalse( data.working ), 0, true, true );

		return 0;
	}
}

eiTEST( TaskSection )//todo test waiting on a critical section
{
	u32 arraySize = s_itemsPerWorker*s_workers*s_tasks;
	u32 stackSize = eiMiB(100)+sizeof(int)*arraySize;
	ScopedMalloc stackMem( stackSize );
	StackAlloc stack( stackMem, stackSize );
	Scope alloc( stack, "Test" );

	InitProfiler( alloc, 1024*1024 );//todo
	Timer* timer = eiNewInterface( alloc, Timer );
	eiASSERT( timer );

	for( int i=0; i!=10*stress; ++i )
	{
		eiInfo(TaskSection, "%d.", i);
		Scope a( alloc, "Test" );

		TaskTestContext data;
		data.tasks = 0;
		data.timer = timer;
		data.results = eiAllocArray( a, int, arraySize );
		memset( data.results, 0, sizeof(int)*arraySize );
		{
			Scope ap( a, "Pool" );
			data.alloc = &ap;//-506
			ReadWriteBarrier();
			Atomic running;
			StartThreadPool( ap, &TestTaskSections, &data, s_workers, &running );
			YieldThreadUntil( WaitForFalse( running ) );
			ProfileEnd( g_JsonProfileOut, ap );
		}

		for( int task=1; task<3; ++task )
		{
			for( int worker=0; worker!=s_workers; ++worker )
			{
				for( int item=0; item!=s_itemsPerWorker; ++item )
				{
					int i = (task-1)*s_workers*s_itemsPerWorker + worker*s_itemsPerWorker + item;
					eiRASSERT( data.results[i] == task );
				}
			}
		}
	/* test out of date
		eiRASSERT( data.criticalCount == s_workers*2 );
		for( int worker=0; worker!=s_workers; ++worker )
		{
			int found = 0;
			for( int i=0; i!=s_workers*2; ++i )
				found += (data.criticalData[i] == worker)?1:0;
			eiRASSERT( found == 2 );
		}*/
	}
}