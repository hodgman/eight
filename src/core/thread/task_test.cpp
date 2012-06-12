
#include <eight/core/test.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/macro.h>
#include <eight/core/thread/timedloop.h>
#include <eight/core/thread/task.h>
#include <eight/core/typeinfo.h>
#include <math.h>

using namespace eight;


//-------------------------------------------------------------------
//-------------------------------------------------------------------



//-------------------------------------------------------------------
static const int stress = 3;
const static int s_scheduleStress = 1000;
// The test process generates a series of fibonacci numbers and performs
// several transforms on that series.
//
//                    fib = fibonacci numbers                          
//                          |             |                     
//                     a = fib * x        |                           
//                             |     b = fib + y                      
//                             |     |                               
//                          c = a / b                         
//

static int* ReferenceValues( Scope& a, int count, int x, int y )
{
	eiASSERT( count >= 2 );
	Scope temp( a, "temp" );
	int* fib = eiAllocArray( temp, int, count );
	int* mul = eiAllocArray( temp, int, count );
	int* add = eiAllocArray( temp, int, count );

	fib[0] = fib[1] = 1;
	for( uint i = 2; i != count; ++i )
		fib[i] = fib[i-1] + fib[i-2];

	for( uint i = 0; i != count; ++i )
		mul[i] = fib[i] * x;

	for( uint i = 0; i != count; ++i )
		add[i] = fib[i] + y;

	int* div = eiAllocArray( a, int, count );
	for( uint i = 0; i != count; ++i )
		div[i] = mul[i] / add[i];

	return div;
}

//-------------------------------------------------------------------
// The above decomposed into data transforms
//-------------------------------------------------------------------
static void Fib( int* fib, int count )
{
	eiASSERT( count >= 2 );
	fib[0] = fib[1] = 1;
	for( uint i = 2; i != count; ++i )
	{
		fib[i] = fib[i-1] + fib[i-2];
		eiASSERT( fib[i] );
	}
}

static void Mul( int* mul, const int* fib, int begin, int end, int x )
{
	eiASSERT( begin >= 0 && begin < end && end <= 42 );
	for( uint i = begin; i != end; ++i )
	{
		mul[i] = fib[i] * x;
		eiASSERT( (fib[i] * x) && fib[i] );
	//	eiASSERT( (mul[i] && fib[i]) );
	}
}

static void Add( int* add, const int* fib, int begin, int end, int y )
{
	eiASSERT( begin >= 0 && begin < end && end <= 42 );
	for( uint i = begin; i != end; ++i )
	{
		add[i] = fib[i] + y;
		eiASSERT( add[i] && fib[i] );
	}
}

static void Div( int* div, const int* add, const int* mul, int begin, int end )
{
	eiASSERT( begin >= 0 && begin < end && end <= 42 );
	for( uint i = begin; i != end; ++i )
	{
		div[i] = mul[i] / add[i];
	}
}
//-------------------------------------------------------------------
// Task kernels to run the above transforms
//-------------------------------------------------------------------
struct FibTaskParms{ int* fib; int count; };
static void FibTask(void*, void* blob, uint size)
{
	eiASSERT( blob && size == sizeof(FibTaskParms) );
	FibTaskParms& params = *(FibTaskParms*)blob;
	eiDEBUG( int begin; int end; eiDistributeTask(params.count, begin, end) );
	eiASSERT( begin == 0 && end == params.count );
	Fib( params.fib, params.count );
}

struct MulTaskParms{ int* mul; const int* fib; int count; int x; };
static void MulTask(void*, void* blob, uint size)
{
	eiASSERT( blob && size == sizeof(MulTaskParms) );
	MulTaskParms& params = *(MulTaskParms*)blob;
	int begin, end = eiDistributeTask(params.count, begin, end);
	Mul( params.mul, params.fib, begin, end, params.x );
}

struct AddTaskParms{ int* add; const int* fib; int count; int y; };
static void AddTask(void*, void* blob, uint size)
{
	eiASSERT( blob && size == sizeof(AddTaskParms) );
	AddTaskParms& params = *(AddTaskParms*)blob;
	int begin, end = eiDistributeTask(params.count, begin, end);
	Add( params.add, params.fib, begin, end, params.y );
}

struct DivTaskParms{ int* div; const int* add;  const int* mul; int count; };
static void DivTask(void*, void* blob, uint size)
{
	eiASSERT( blob && size == sizeof(DivTaskParms) );
	DivTaskParms& params = *(DivTaskParms*)blob;
	int begin, end = eiDistributeTask(params.count, begin, end);
	Div( params.div, params.add, params.mul, begin, end );
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
struct NumbersTest_Data
{
	Scope* a;
	TaskSchedule* s;
	uint numSchedules;
	int** buffers;

	FibTaskParms   fib;
	MulTaskParms   mul;
	AddTaskParms   add;
	DivTaskParms   div;

	void Schedule(Scope& a, int count=1, int x=2, int y=2)
	{
		numSchedules = s_scheduleStress;
		s = eiAllocArray( a, TaskSchedule, numSchedules );
		for( uint i=0, end=numSchedules; i!=end; ++i )
		{
			buffers    = eiAllocArray(a, int*, 4);
			int pad = 2;//todo - just debug code
			buffers[0] = eiAllocArray(a, int, count*pad);
			buffers[1] = eiAllocArray(a, int, count*pad);
			buffers[2] = eiAllocArray(a, int, count*pad);
			buffers[3] = eiAllocArray(a, int, count*pad);
			FibTaskParms f = { buffers[0],//fib
							   count//count
							 };
			fib = f;
			MulTaskParms m = { buffers[1],//mul
							   buffers[0],//fib
							   count,//count
							   x//x
							 };
			mul = m;
			AddTaskParms d = { buffers[2],//add
							   buffers[0],//fib
							   count,//count
							   y//y
							 };
			add = d;
			DivTaskParms v = { buffers[0],//div
							   buffers[2],//add
							   buffers[1],//mul
							   count//count
							 };
			div = v;

			TaskProto fib_task = {&FibTask, &fib, sizeof(fib) };
			TaskProto* fib_array[] = {&fib_task};
			TaskGroupProto fib_group = 
			{
				TaskSection(1),
				fib_array, ArraySize(fib_array),
				0, 0,
			};

			TaskProto mul_task = {&MulTask, &mul, sizeof(mul) };
			TaskProto* mul_array[] = {&mul_task};
			TaskGroupDepends mul_depend[] = { &fib_group };
			TaskGroupProto mul_group = 
			{
				TaskSection(),
				mul_array, ArraySize(mul_array),
				mul_depend, ArraySize(mul_depend),
			};

			TaskProto add_task = {&AddTask, &add, sizeof(add) };
			TaskProto* add_array[] = {&add_task};
			TaskGroupDepends add_depend[] = { &fib_group };
			TaskGroupProto add_group = 
			{
				TaskSection(),
				add_array, ArraySize(add_array),
				add_depend, ArraySize(add_depend),
			};

			TaskProto div_task = {&DivTask, &div, sizeof(div) };
			TaskProto* div_array[] = {&div_task};
			TaskGroupDepends div_depend[] = { &add_group, &mul_group };
			TaskGroupProto div_group = 
			{
				TaskSection(),
				div_array, ArraySize(div_array),
				div_depend, ArraySize(div_depend),
			};

			TaskGroupDepends fib_depend[] = { PreviousFrame(&div_group) };
			fib_group.dependencies = fib_depend;
			fib_group.numDependencies = ArraySize(fib_depend);

			TaskGroupProto* groups[] = { &mul_group, &fib_group, &div_group, &add_group };
			
			s[i] = MakeSchedule( a, groups, ArraySize(groups) );
		}
	}
};
//	struct FibTaskParms{ int* fib; int count; };
//	struct MulTaskParms{ int* mul; const int* fib; int begin; int end; int x; };
//	struct AddTaskParms{ int* add; const int* fib; int begin; int end; int y; };
//	struct FinalTaskParms{ int* final; const int* add;  const int* mul; int begin; int end; };

eiInfoGroup(Test, true);

void* NumbersTest_Init(Scope& a, void* user)
{
	ThreadGroup init;
	eiBeginSectionThread(init);
	{
		NumbersTest_Data& args = *(NumbersTest_Data*)user;

		const uint count = 42;
		const int x = 2;
		const int y = 2;
		args.Schedule(a, count, x, y);
	}
	eiEndSection(init);

	int* frame = eiNew(a, int);
	*frame = 0;
	return frame;
}
void* NumbersTest_Prepare(Scope& a, Scope& scratch, void* user, void* thread)
{
	int& frame = *(int*)thread;
	//eiInfo(Test, "Prep %d", frame);
	return (void*)1;
}
void NumbersTest_Worker(Scope& a, Scope& scratch, void* user, void* thread, void* task)
{
	NumbersTest_Data& args = *(NumbersTest_Data*)user;
	eiASSERT( task == (void*)1 );

	int& frame = *(int*)thread;
	//eiInfo(Test, "Work %d", frame);
	ExecuteSchedules( a, frame, args.s, args.numSchedules, ThreadGroup() );
	++frame;
}


//	typedef void* (FnAllocThread)(Scope& a, void* user);
//	typedef void* (FnPrepareTask)(Scope& a, Scope& frame, void* user, void* thread);//return 0 to break
//	typedef void  (FnExecuteTask)(Scope& a, Scope& frame, void* user, void* thread, void* task);

//void NumbersTest_Init(Scope& a, Scope& frame, void* user);
//void NumbersTest_Worker(Scope& thread, Scope& frame, void* user);

static void NumbersTest(Scope& a)
{
	NumbersTest_Data args = { &a, 0 };
	TimedLoop::Start( a, &args, &NumbersTest_Init, &NumbersTest_Prepare, &NumbersTest_Worker );
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------



struct BasicTest_Data { Scope* a; TaskSchedule* s; };
static int BasicTest_Worker(void* arg, uint threadIndex, uint numThreads, uint systemId)
{/*
	BasicTest_Data& args = *(BasicTest_Data*)arg;
	Scope& a = *args.a;
	TaskSchedule& s = *args.s;
	return 0;*/
}
static void BasicTest(Scope& a)
{/*
	TaskProto task1 = {};
	TaskProto task2 = {};
	TaskProto task3 = {};
	TaskProto* groupATasks[] = {&task1, &task2, &task3};
	TaskGroupProto groupA = 
	{
		TaskSection(),
		groupATasks,
		ArraySize(groupATasks),
		0,
		0,
	};
	TaskGroupProto* groups[] = { &groupA };
	TaskSchedule s = MakeSchedule( a, groups, ArraySize(groups) );

	BasicTest_Data args = { &a, &s };
	
	StartThreadPool( a, &BasicTest_Worker, &args );*/
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------

eiTEST( TaskSchedule )
{
	u32   const stackSize = eiGiB(1);
	void* const stackMem = malloc( stackSize );
	for( int i=0; i!=stress; ++i )
	{
		eiInfo(TaskSection, "%d.", i);
		StackAlloc stack( stackMem, stackSize );
		Scope a( stack, "Test" );

		BasicTest(a);

		NumbersTest(a);
	}
	free( stackMem );
}