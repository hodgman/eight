
#include <eight/core/test.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/macro.h>
#include <eight/core/thread/timedloop.h>
#include <eight/core/thread/task.h>
#include <eight/core/typeinfo.h>
#include <eight/core/timer/timer.h>
#include <math.h>

using namespace eight;


//-------------------------------------------------------------------
//-------------------------------------------------------------------

#if 0

//-------------------------------------------------------------------
#if eiDEBUG_LEVEL != 0
static const int stress = 1;
const static int s_scheduleStressA = 1;
const static int s_scheduleStressB = 1;
#else
static const int stress = 6;
const static int s_scheduleStressA = 10;
const static int s_scheduleStressB = 10;
#endif
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
eiInfoGroup(NumbersTest, false);
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

private:
	void Schedule(Scope& a, int count=1, int x=2, int y=2)
	{
		numSchedules = s_scheduleStressA;
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
				fib_array, eiArraySize(fib_array),
				0, 0,
			};

			TaskProto mul_task = {&MulTask, &mul, sizeof(mul) };
			TaskProto* mul_array[] = {&mul_task};
			TaskGroupDepends mul_depend[] = { &fib_group };
			TaskGroupProto mul_group = 
			{
				TaskSection(),
				mul_array, eiArraySize(mul_array),
				mul_depend, eiArraySize(mul_depend),
			};

			TaskProto add_task = {&AddTask, &add, sizeof(add) };
			TaskProto* add_array[] = {&add_task};
			TaskGroupDepends add_depend[] = { &fib_group };
			TaskGroupProto add_group = 
			{
				TaskSection(),
				add_array, eiArraySize(add_array),
				add_depend, eiArraySize(add_depend),
			};

			TaskProto div_task = {&DivTask, &div, sizeof(div) };
			TaskProto* div_array[] = {&div_task};
			TaskGroupDepends div_depend[] = { &add_group, &mul_group };
			TaskGroupProto div_group = 
			{
				TaskSection(),
				div_array, eiArraySize(div_array),
				div_depend, eiArraySize(div_depend),
			};

			TaskGroupDepends fib_depend[] = { PreviousFrame(&div_group) };
			fib_group.dependencies = fib_depend;
			fib_group.numDependencies = eiArraySize(fib_depend);

			TaskGroupProto* groups[] = { &mul_group, &fib_group, &div_group, &add_group };
			
			s[i] = MakeSchedule( a, groups, eiArraySize(groups) );
		}
	}
	
	friend class SimLoop;
	typedef NumbersTest_Data Args;
	typedef int ThreadData;
	typedef void FrameData;
	typedef void InterruptData;

	static NumbersTest_Data* Create(Scope& a, Scope& thread, Args& args, const TaskLoop&)
	{
		const uint count = s_scheduleStressB;
		const int x = 2;
		const int y = 2;
		args.Schedule(a, count, x, y);
		return &args;
	}
	static ThreadData* InitThread(Scope& a, Args&)
	{
		int* frame = eiNew(a, int);
		*frame = 0;
		return frame;
	}
	FrameData* Prepare(Scope&, Scope&, ThreadData* thread, FrameData*, void**)
	{
		int& frame = *thread;
		eiInfo(NumbersTest, "Prep %d", frame);
		if( frame == 100 )
			return 0;
		return (void*)1;
	}
	void Execute(Scope&, Scope& scratch, ThreadData* thread, FrameData* task, uint worker)
	{
		eiASSERT( task == (void*)1 );

		int& frame = *thread;
		eiInfo(NumbersTest, "Work %d", frame);
		ExecuteSchedules( scratch, frame, s, numSchedules, SingleThread() );
		++frame;
	}
};
//	struct FibTaskParms{ int* fib; int count; };
//	struct MulTaskParms{ int* mul; const int* fib; int begin; int end; int x; };
//	struct AddTaskParms{ int* add; const int* fib; int begin; int end; int y; };
//	struct FinalTaskParms{ int* final; const int* add;  const int* mul; int begin; int end; };


static void NumbersTest(Scope& a, ConcurrentFrames::Type c)
{
	NumbersTest_Data args = { &a, 0 };
	SimLoop::Start<NumbersTest_Data>( a, &args, c, eiMiB(128), eiMiB(1) );
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------



struct BasicTest_Data { Scope* a; TaskSchedule* s; };
static int BasicTest_Worker(void* arg, uint threadIndex, uint numThreads, uint systemId)
{/*
	BasicTest_Data& args = *(BasicTest_Data*)arg;
	Scope& a = *args.a;
	TaskSchedule& s = *args.s;*/
	return 0;
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
		eiArraySize(groupATasks),
		0,
		0,
	};
	TaskGroupProto* groups[] = { &groupA };
	TaskSchedule s = MakeSchedule( a, groups, eiArraySize(groups) );

	BasicTest_Data args = { &a, &s };
	
	StartThreadPool( a, &BasicTest_Worker, &args );*/
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------

eiInfoGroup(Test, true);
eiTEST( TaskSchedule )
{
	u32   const stackSize = eiMiB(256);
	ScopedMalloc stackMem( stackSize );
	StackAlloc stack( stackMem, stackSize );
	Scope scope( stack, "main" );
	Timer& timer = *eiNewInterface(scope, Timer)();
	ConcurrentFrames::Type conc[] = { ConcurrentFrames::OneFrame, ConcurrentFrames::TwoFrames, ConcurrentFrames::ThreeFrames };
	double* totals = eiAllocArray(scope, double, eiArraySize(conc));
	for( int i=0; i!=eiArraySize(conc); ++i )
		totals[i] = 0;
	
	void TestAB(Timer& timer); TestAB(timer);
	timer.Reset();

	return;

	for( int i=0; i!=stress; ++i )
	{
		Scope a( scope, "Test" );

		BasicTest(a);

		int idx = i%eiArraySize(conc);

		timer.Reset();
		NumbersTest(a, conc[idx]);
		double time = timer.Elapsed(true);

		totals[idx] += time;
		
	//	eiInfo(Test, "%d. %.4f", i, time);
	}
	for( int i=0; i!=eiArraySize(conc); ++i )
	{
		eiInfo(Test, "type %d: %.2f", conc[i], totals[i]);
	}
}


#define FORCEINLINE __forceinline 
#define FASTCALL __fastcall 
namespace Version1
{
	struct A { virtual void DoStuff() {}; int a; };
	struct B : public A { void DoStuff() { b = a; } int b; };
}
namespace Version2 // translate Version1 into C
{
	struct A;
	struct A_VTable { typedef void (*FnDoStuff)(A*); FnDoStuff DoStuff; };
	struct A { A_VTable* vtable; int a; };
	FORCEINLINE void FASTCALL A_DoStuff( A* obj ) { (*obj->vtable->DoStuff)(obj); }
	struct B { A base; int b; };
	void B_DoStuff( B* obj ) { obj->b = obj->base.a; }
	A_VTable g_B_VTable = { (A_VTable::FnDoStuff)&B_DoStuff };
	FORCEINLINE void FASTCALL B_Construct( B* obj ) { obj->base.vtable = &g_B_VTable; }
}
namespace Version3 // optimize version2 by embedding the vtable in the object
{
	struct A {
		typedef void (A::*FnDoStuff)();
		FORCEINLINE A(FnDoStuff p) : pfnDoStuff(p) {}
		FnDoStuff pfnDoStuff;
		int a;
		FORCEINLINE void FASTCALL DoStuff() { (this->*pfnDoStuff)(); }
	};
	struct B : public A {
		FORCEINLINE B() : A(static_cast<FnDoStuff>(&B::DoStuff)) {}
		void DoStuff() { b = a; }
		int b;
	};
}

static const int cacheSize = eiMiB(10);
int FlushCache(void* cache)
{
	memset(cache, 0, cacheSize);
	int result = 0;
	int* data = (int*)cache;
	for( int i=0; i<(cacheSize/sizeof(int)); ++i )
		result += data[i];
	return result;
}

#include <stdio.h>
void TestAB(Timer& timer)
{
	void* cache = malloc(cacheSize);
	double v1Derived = 0, v2Derived = 0, v3Derived = 0;
	double v1Base = 0, v2Base = 0, v3Base = 0;
	double v1Total = 0, v2Total = 0, v3Total = 0;
	int dontOptimize1 = 0, dontOptimize2 = 0, dontOptimize3 = 0;
	int stress = 10;

	dontOptimize1 += FlushCache(cache);
	double v1Start = timer.Elapsed();
	for( int c=0; c!=stress; ++c )
	{
		Version1::B obj;
		Version1::A* base = &obj;
		double start = timer.Elapsed();
		for( int i=0; i!=stress; ++i )
		{
			obj.a = i;
			obj.DoStuff();
		}
		v1Derived += timer.Elapsed() - start;
		start = timer.Elapsed();
		for( int i=0; i!=stress; ++i )
		{
			base->a = i;
			base->DoStuff();
		}
		v1Base += timer.Elapsed() - start;
		dontOptimize1 += obj.b;
	}
	v1Total = timer.Elapsed() - v1Start;

	dontOptimize2 += FlushCache(cache);
	double v2Start = timer.Elapsed();
	for( int c=0; c!=stress; ++c )
	{
		Version2::B obj;
		B_Construct( &obj );
		Version2::A* base = &obj.base;
		double start = timer.Elapsed();
		for( int i=0; i!=stress; ++i )
		{
			obj.base.a = i;
			B_DoStuff( &obj );
		}
		v2Derived += timer.Elapsed() - start;
		start = timer.Elapsed();
		for( int i=0; i!=stress; ++i )
		{
			base->a = i;
			A_DoStuff( base );
		}
		v2Base += timer.Elapsed() - start;
		dontOptimize2 += obj.b;
	}
	v2Total = timer.Elapsed() - v2Start;

	dontOptimize3 += FlushCache(cache);
	double v3Start = timer.Elapsed();
	for( int c=0; c!=stress; ++c )
	{
		Version3::B obj;
		Version3::A* base = &obj;
		double start = timer.Elapsed();
		for( int i=0; i!=stress; ++i )
		{
			obj.a = i;
			obj.DoStuff();
		}
		v3Derived += timer.Elapsed() - start;
		start = timer.Elapsed();
		for( int i=0; i!=stress; ++i )
		{
			base->a = i;
			base->DoStuff();
		}
		v3Base += timer.Elapsed() - start;
		dontOptimize3 += obj.b;
	}
	v3Total = timer.Elapsed() - v3Start;

	eiASSERT( dontOptimize1 == dontOptimize2 );
	eiASSERT( dontOptimize1 == dontOptimize3 );

	free(cache);
/*
	printf( "   Total   Derived   Base\n" );
	printf( "1) %.2f    %.2f      %.2f\n", v1Total, v1Derived, v1Base );
	printf( "2) %.2f    %.2f      %.2f\n", v2Total, v2Derived, v2Base );
	printf( "3) %.2f    %.2f      %.2f\n", v3Total, v3Derived, v3Base );*/
}

#endif
