#include <eight/core/thread/tasksection.h>
#include <eight/core/thread/thread.h>
#include <eight/core/thread/pool.h>
#include <eight/core/test.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/macro.h>
#include <math.h>
using namespace eight;

namespace
{
	static const int stress = 3;
	static const int s_workers = NumberOfHardwareThreads()*2;
	static const int s_tasks = 2;
	static const int s_itemsPerWorker = 10*stress;

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
		TaskTestContext() : initialTask(), tasks(), results(), criticalCount(0), alloc() { eiRASSERT( s_workers <= 32 ); }
		ThreadGroup initialTask;
		Atomic initialized;
		TestTasks* tasks;
		int* results;
		int criticalCount;
		int criticalData[64];
		Scope* alloc;
	};

	int TestTaskSections( void* arg, uint threadIndex, uint numThreads, uint systemId )
	{
		eiUsingTaskSections;
		const int fakeWork = 0x10;
		eiRASSERT( numThreads == s_workers );
		eiRASSERT( _ei_thread_id->index == threadIndex );
		eiRASSERT( _ei_thread_id->mask == 1<<threadIndex );
		eiRASSERT( _ei_thread_id->poolSize == numThreads );
		TaskTestContext& data = *(TaskTestContext*)arg;

		eiBeginSectionThread(data.initialTask);
		{
			data.tasks = eiNew( *data.alloc, TestTasks )();
			data.initialized = numThreads;
		}
		eiEndSection(data.initialTask);
		YieldThreadUntil( WaitForTrue(data.initialized) );	

		if( threadIndex == 2 )
		{
			eiBeginSectionRedundant(data.tasks->task2);
			{
				uint items = s_itemsPerWorker*s_workers;
				uint begin,end = eiDistributeTask( items, begin, end );
				for( int i=begin; i!=end; ++i )
				{
					eiRASSERT( data.results[items*1 + i] == 0 );
					data.results[items*1 + i] = 2;
				}
			}
			eiEndSection(data.tasks->task2);
		}

		eiBeginSectionTask(data.tasks->task1);
		{
			uint items = s_itemsPerWorker*s_workers;
			uint begin,end = eiDistributeTask( items, begin, end );
			for( int i=begin; i!=end; ++i )
			{
				float a = 1;
				for( int j=0; j!=fakeWork; ++j ) { a = (sqrt( a*i+j+1 ) - a+(3.25f+a)+a*24.1131f+a)*0.001f; }
				eiRASSERT( data.results[items*0 + i] == 0 );
				data.results[items*0 + i] = int(1 + (a/(a+1))*0.000001);
			}
		
			eiBeginSectionSemaphore(data.tasks->criticalTask);
			{
				data.criticalData[data.criticalCount++] = threadIndex;
			}
			eiEndSection(data.tasks->criticalTask);
			
			eiBeginSectionRedundant(data.tasks->task2);
			{
				uint items = s_itemsPerWorker*s_workers;
				uint begin,end = eiDistributeTask( items, begin, end );
				for( int i=begin; i!=end; ++i )
				{
					float a = 2;
					for( int j=0; j!=fakeWork; ++j ) { a = (sqrt( a*i+j+1 ) - a+(3.25f+a)+a*24.1131f+a)*0.001f; }
					eiRASSERT( data.results[items*1 + i] == 0 );
					data.results[items*1 + i] = int(2 + (a/(a+1))*0.000001);
				}
			}
			eiEndSection(data.tasks->task2);
		}
		eiEndSection(data.tasks->task1);
		
		eiBeginSectionSemaphore(data.tasks->criticalTask);
		{
			data.criticalData[data.criticalCount++] = threadIndex;
		}
		eiEndSection(data.tasks->criticalTask);

		eiWaitForSection(data.tasks->task1);
		eiAssertTaskDone(data.tasks->task2);
		
		data.initialized--;
		YieldThreadUntil( WaitForFalse(data.initialized) );	

		return 0;
	}
}

eiTEST( TaskSection )//todo test waiting on a critical section
{
	u32 arraySize = s_itemsPerWorker*s_workers*s_tasks;
	u32 stackSize = eiMiB(1)+sizeof(int)*arraySize;
	ScopedMalloc stackMem( stackSize );
	for( int i=0; i!=1*stress; ++i )
	{
		eiInfo(TaskSection, "%d.", i);
		StackAlloc stack( stackMem, stackSize );
		Scope a( stack, "Test" );

		TaskTestContext data;
		data.results = eiAllocArray( a, int, arraySize );
		memset( data.results, 0, sizeof(int)*arraySize );
		{
			Scope ap( a, "Pool" );
			data.alloc = &ap;
			StartThreadPool( ap, &TestTaskSections, &data, s_workers );
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