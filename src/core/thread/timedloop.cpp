
#include <eight/core/alloc/new.h>
#include <eight/core/debug.h>
#include <eight/core/thread/timedloop.h>
#include <eight/core/thread/pool.h>
#include <eight/core/timer/timer_impl.h>
#include <eight/core/id.h>

using namespace eight;

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/*
template<class T, class N> struct _        { T data; N next; };
template<class T>          struct _<T,Nil> { T data; };

template<class T> struct ArgList { T data; };
template<>        struct ArgList<Nil> {};
*/

template<class A=Nil, class B=Nil, class C=Nil>
                           struct Tuple              { A a; B b; C c; template<class F> void ForEach(F& f) { f(a); f(b); f(c); } };
template<class A, class B> struct Tuple<  A,  B,Nil> { A a; B b;      template<class F> void ForEach(F& f) { f(a); f(b);       } };
template<class A>          struct Tuple<  A,Nil,Nil> { A a;           template<class F> void ForEach(F& f) { f(a);             } };
template<>                 struct Tuple<Nil,Nil,Nil> {                template<class F> void ForEach(F& f) {                   } };


// When sending a message, if a value is not known immediately, the index of and byte-offset into a future/return value
//   can be written instead. All arguments are padded to 32bits to allow this.
// N.B. the corresponding bit in the parent ArgStore::futureMask should be set when writing these future values.
struct FutureIndex
{
	u16 index;
	u16 offset;
};

//Pads arguments out to 32-bits to allow them to be overwritten by FutureIndex values.
template<class T> struct FutureArg
{
	FutureArg(){}
	FutureArg(const T& v) : value(v) {}
	FutureArg(const FutureIndex&) : index(index) {}
	union
	{
		T           value;
		FutureIndex index;
	};
	operator const T&() const { return value; }
};

template<class T> struct ReturnType       { static uint Size() { return sizeof(T); } T value; operator T&() { return value; } void operator=(int i){value = i;}};
template<>        struct ReturnType<void> { static uint Size() { return 0; } };

template<class R, class A=Nil, class B=Nil, class C=Nil>
                                    struct ArgStore                { Tuple<FutureArg<A>, FutureArg<B>, FutureArg<C>> t; uint futureMask; ReturnType<R> r; };
template<class R, class A, class B> struct ArgStore<R,  A,  B,Nil> { Tuple<FutureArg<A>, FutureArg<B>>               t; uint futureMask; ReturnType<R> r; };
template<class R, class A>          struct ArgStore<R,  A,Nil,Nil> { Tuple<FutureArg<A>>                             t; uint futureMask; ReturnType<R> r; };
template<class R>                   struct ArgStore<R,Nil,Nil,Nil> { Tuple<>                                         t;                  ReturnType<R> r; };

template<class R, class T, class F, class A, class B, class C> void Call(ReturnType<R>& out, const Tuple<A,B,C>& t, T* obj, F fn) { out = ((*obj).*(fn))( t.a, t.b, t.c ); } 
template<class R, class T, class F, class A, class B>          void Call(ReturnType<R>& out, const Tuple<A,B  >& t, T* obj, F fn) { out = ((*obj).*(fn))( t.a, t.b      ); } 
template<class R, class T, class F, class A>                   void Call(ReturnType<R>& out, const Tuple<A    >& t, T* obj, F fn) { out = ((*obj).*(fn))( t.a           ); } 
template<class R, class T, class F>                            void Call(ReturnType<R>& out, const Tuple<     >& t, T* obj, F fn) { out = ((*obj).*(fn))(               ); } 

template<class T, class F, class A, class B, class C> void Call(ReturnType<void>&, const Tuple<A,B,C>& t, T* obj, F fn) { ((*obj).*(fn))( t.a, t.b, t.c ); } 
template<class T, class F, class A, class B>          void Call(ReturnType<void>&, const Tuple<A,B  >& t, T* obj, F fn) { ((*obj).*(fn))( t.a, t.b      ); } 
template<class T, class F, class A>                   void Call(ReturnType<void>&, const Tuple<A    >& t, T* obj, F fn) { ((*obj).*(fn))( t.a           ); } 
template<class T, class F>                            void Call(ReturnType<void>&, const Tuple<     >& t, T* obj, F fn) { ((*obj).*(fn))(               ); } 


template<class T> struct ArgFromSig;
template<class R, class T>                            struct ArgFromSig<R(T::*)(void )> { typedef ArgStore<R,Nil,Nil,Nil> Storage; typedef R Return; };
template<class R, class T, class A>                   struct ArgFromSig<R(T::*)(A    )> { typedef ArgStore<R,  A,Nil,Nil> Storage; typedef R Return; };
template<class R, class T, class A, class B>          struct ArgFromSig<R(T::*)(A,B  )> { typedef ArgStore<R,  A,  B,Nil> Storage; typedef R Return; };
template<class R, class T, class A, class B, class C> struct ArgFromSig<R(T::*)(A,B,C)> { typedef ArgStore<R,  A,  B,  C> Storage; typedef R Return; };


void testy()
{
	FutureArg<int> one( 1 );
	const FutureArg<int>& two = FutureArg<int>( FutureIndex() );
	ArgStore<void, int, int, float> a = {{one,two,3.0f}};
	ArgStore<void, int, int> b = {{1,2}};
	ArgStore<void, int> c = {{1}};
	ArgStore<void> d = {};
}

template<class F>
FutureCall* TaskBuffer::Push( void* obj, FnTask* task, uint argSize, F fn )
{
	return Push( task, obj, argSize, ReturnType<ArgFromSig<F>::Return>::Size() );
}
FutureCall* TaskBuffer::Push( FnTask* task, void* obj, uint argSize, uint returnSize )
{
	FutureCall* data = a.Alloc<FutureCall>();
	data->task = task;
	data->obj = obj;
	data->args = argSize ? a.Alloc(argSize) : 0;
	data->argSize = argSize;
	data->next = 0;
	data->returnSize = returnSize;
	if( !returnSize )
		data->returnIndex = 0;
	else
	{
		data->returnIndex = m_nextReturnIndex++;
		m_returnSize.Align( 16 );//todo
		m_returnSize.Alloc( returnSize );
	}
	Insert(data);
	return data;
};

template<class T, class F>
void Call(void* user, void* blob, uint size, F fn)
{
	typedef ArgFromSig<F>::Storage TArgs;
	eiASSERT( sizeof(TArgs) == size ); eiUNUSED(size);
	TArgs n = {};										
	TArgs& a = blob ? *((TArgs*)blob) : n;	
	eiASSERT( user );
	Call( a.r, a.t, (T*)user, fn );
}

struct FetchLuaArg
{
	FetchLuaArg(void* luaState) : luaState(luaState) {} void* luaState;
	template<class T> void operator()(T& arg)
	{
		//arg = LuaFetch<T>(luaState)  /  LuaFetch(luaState, &arg)
	}
};

template<class F>
FutureIndex LuaPush(TaskBuffer& q, FnTask* call, void* user, void* luaState, F fn)
{
	typedef ArgFromSig<F>::Storage TArgs;
	typedef ArgFromSig<F>::Return TReturn;
	FutureCall* msg = q.Push( call, user, sizeof(TArgs), ReturnType<TReturn>::Size() );
	TArgs& a = *(TArgs*)msg->args.Ptr();
	a.t.ForEach( FetchLuaArg(luaState) );
	FutureIndex retval = { msg->returnIndex };
	return retval;
}


#define eiUserMsgEntry( T, NAME )																		\
FutureIndex LuaPush_##T##_##NAME(TaskBuffer& q, void* user, void* luaState)								\
{																										\
	return LuaPush( q, &Call_##T##_##NAME, user, luaState, &T::NAME );									\
}																										//

#define eiUserMsgFuncs( T, NAME )																		\
									  static void LuaPush(TaskBuffer& q, void* user, void* luaState) {	\
															LuaPush_##T##_##NAME(q, user, luaState); }	//
#ifndef eiUserMsgEntry
#define eiUserMsgEntry(...)
#endif
#ifndef eiUserMsgFuncs
#define eiUserMsgFuncs(...)
#endif


template<memfuncptr> struct CallFromMemFunc;

#define eiMessage( T, NAME )																					\
void Call_##T##_##NAME(void* user, void* blob, uint size)														\
{																												\
	Call<T>(user, blob, size, &T::NAME);																		\
}																												\
void Push_##T##_##NAME(TaskBuffer& q, void* user, void* blob, uint size)										\
{																												\
	q.Push( user, &Call_##T##_##NAME, size, &T::NAME );															\
}																												\
template<> struct CallFromMemFunc<(memfuncptr)&T::NAME> { static FnTask* value; };								\
FnTask* CallFromMemFunc<(memfuncptr)&T::NAME>::value = &Call_##T##_##NAME;										\
eiUserMsgEntry( T, NAME )																						\
template<class N, class t> struct MsgFuncs;																		\
class Tag##NAME;																								\
template<> struct MsgFuncs<Tag##NAME,T> { static void Call(void* user, void* blob, uint size) {					\
															Call_##T##_##NAME(user, blob, size); }				\
										  static void Push(TaskBuffer& q, void* user, void* blob, uint size) {	\
															Push_##T##_##NAME(q, user, blob, size); }			\
										  eiUserMsgFuncs( T, NAME )												\
										};																		//

//struct Tag_Future {};
//typedef PrimitiveWrap<u32, Tag_Future> Future;
template<class T>
struct Future : public FutureIndex
{
};

template<class T> struct IsFuture              { static const bool value = false; };
template<class T> struct IsFuture<Future<T>>   { static const bool value = true; };
template<       > struct IsFuture<FutureIndex> { static const bool value = true; };
template<class A=Nil, class B=Nil, class C=Nil>
struct FutureMask
{
	static const uint value = (IsFuture<A>::value ? 1<<0 : 0)
	                        | (IsFuture<B>::value ? 1<<1 : 0)
	                        | (IsFuture<C>::value ? 1<<2 : 0);
};


#define eiPush(q,o,N,...) Push<(memfuncptr)&N>(q, o, &N, __VA_ARGS__)

template<memfuncptr fn, class F, class T> 
void Push(TaskBuffer& q, T& user, F)
{
	q.Push( CallFromMemFunc<fn>::value, &user, 0, 0 );
}
template<memfuncptr fn, class F, class T, class A>
void Push(TaskBuffer& q, T& user, F, A a)
{
	typedef ArgFromSig<F> Info;
	Info::Storage args = { {FutureArg<A>(a)}, FutureMask<A>::value };
	FutureCall* msg = q.Push( CallFromMemFunc<fn>::value, &user, sizeof(Info::Storage), ReturnType<Info::Return>::Size() );
	Info::Storage& out = *(Info::Storage*)msg->args.Ptr();
	out = args;
}
template<memfuncptr fn, class F, class T, class A, class B>
void Push(TaskBuffer& q, T& user, F, A a, B b)
{
	typedef ArgFromSig<F> Info;
	Info::Storage args = { {FutureArg<A>(a), FutureArg<B>(b)}, FutureMask<A,B>::value };
	FutureCall* msg = q.Push( CallFromMemFunc<fn>::value, &user, sizeof(Info::Storage), ReturnType<Info::Return>::Size() );
	Info::Storage& out = *(Info::Storage*)msg->args.Ptr();
	out = args;
}
template<memfuncptr fn, class F, class T, class A, class B, class C>
void Push(TaskBuffer& q, T& user, F, A a, B b, C c)
{
	typedef ArgFromSig<F> Info;
	Info::Storage args = { {FutureArg<A>(a), FutureArg<B>(b), FutureArg<C>(c)}, FutureMask<A,B,C>::value };
	FutureCall* msg = q.Push( CallFromMemFunc<fn>::value, &user, sizeof(Info::Storage), ReturnType<Info::Return>::Size() );
	Info::Storage& out = *(Info::Storage*)msg->args.Ptr();
	out = args;
}

class Test
{
public:
	eiBind(Test);
	int DoStuff() { return 0; }
	void FooBar(int*, const float*) { }
};

eiMessage( Test, DoStuff );
eiMessage( Test, FooBar );

eiBindClass( Test )
	eiBeginMethods()
		eiBindMethod( DoStuff )
	eiEndMethods();
eiEndBind();

eiBindClass( TimedLoop )
	eiBeginData()
		eiBindReference( m_timer ) //todo
		eiBindData( m_user )
		eiBindData( m_allocThreadLocal )
		eiBindData( m_prepareTask )
		eiBindData( m_executeTask )
	eiEndData();
eiEndBind();

template<class T, class F, class A>
void Call( F& f, T* t, A* a )
{
//	(t.*f)();
}

void testBind()
{
	const TypeBinding& a = ReflectTimedLoop();
	const TypeBinding& b = ReflectTest();

	Test t;
	char buf[1024];
	StackAlloc s(buf,1024);
	TaskBuffer q(s);
	Push_Test_DoStuff( q, &t, 0, 0 );
	ArgStore<void, int> args = { 1 };
	Push_Test_FooBar( q, &t, &args, sizeof(ArgStore<int>) );
	Call_Test_FooBar(    &t, &args, sizeof(ArgStore<int>) );
	LuaPush_Test_FooBar( q, &t, 0 );

	eiPush( q, t, Test::DoStuff );
	eiPush( q, t, Test::FooBar, (int*)0, (float*)0 );
	

	eiASSERT( 0==strcmp(b.method[0].name,"DoStuff") );
	(t.*((void(Test::*)(void*))b.method[0].memfuncptr))(0);
	(void)(b.dataCount + a.dataCount);
}
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

TimedLoop::TimedLoop(Scope& a, void* user, FnPrepareTask* pfnP, FnExecuteTask* pfnE)
	: m_timer(*eiNew(a, Timer))
	, m_prepareTask(pfnP)
	, m_executeTask(pfnE)
	, m_user(user)
{}

void TimedLoop::Start( Scope& scope, void* user, FnAllocThread* pfnA, FnPrepareTask* pfnP, FnExecuteTask* pfnE )
{
	Scope a( scope, "stack" );
	Args args = { &a, user, pfnA, pfnP, pfnE };
	TaskLoop* loop = eiNew(a, TaskLoop)( a, &args, &TimedLoop::s_InitThread, eiMiB(128), eiMiB(1) );
	StartThreadPool( a, &TaskLoop::ThreadMain, loop );//calls eiNew(a, TimedLoop) in s_InitThread
}

void* TimedLoop::s_InitThread(Scope& thread, uint idx, TaskLoop& loop, TaskLoop::Config* cfg)
{
	Args& args = *(Args*)cfg->user;
	if( idx == 0 )
	{
		Scope& a = *args.a;
		args.result      = eiNew(a, TimedLoop)(a, args.user, args.pfnP, args.pfnE);
		cfg->userPrepare = &s_PrepareTask;
		cfg->userTask    = &s_Task;
	}
	ThreadData* data = eiAlloc( thread, ThreadData );
//		data->frame = 0;
	data->prevTime = 0;
	data->userThreadLocal = args.pfnA( thread, args.user );
	return data;
}

void* TimedLoop::PrepareTask(Scope& a, Scope& frame, void* thread, const void* prevFrame)
{
	ThreadData& data = *(ThreadData*)thread;
	FrameData* frameData = eiAlloc(frame, FrameData);
	frameData->time = m_timer.Elapsed();
	frameData->frame = prevFrame ? ((FrameData*)prevFrame)->frame + 1 : 0;
	uint worker = CurrentPoolThread().index;
	double elapsed = frameData->time;
	double dt = prevFrame ? elapsed - ((FrameData*)prevFrame)->time : 0;
	eiInfo(TimedLoop, "%d P> %d, %.9f, %.9f, %.0ffps %s", worker, frameData->frame, m_timer.Elapsed(), elapsed, 1/dt, worker==0?"*":"");
	bool done = frameData->time >= 1;
	if(done)
	{
		return 0;
	}
	frameData->userFrame = m_prepareTask( a, frame, m_user, data.userThreadLocal );
	return frameData;
}
void TimedLoop::ExecuteTask(Scope& a, Scope& frame, void* threadData, void* taskData)
{
	ThreadData& data = *(ThreadData*)threadData;
	FrameData& frameData = *(FrameData*)taskData;

	int f = frameData.frame;

	double elapsed = frameData.time;
	double dt = elapsed - data.prevTime;
	data.prevTime = elapsed;
	uint worker = CurrentPoolThread().index;
//	if( worker == 0 && f % 100 == 0 )
	eiInfo(TimedLoop, "%d E> %d, %.9f, %.9f, %.0ffps %s", worker, f,  m_timer.Elapsed(), elapsed, 1/dt, worker==0?"*":"");

	m_executeTask( a, frame, m_user, data.userThreadLocal, frameData.userFrame );
}

/*

int* PlusOne( int* input, int count )
{
	int temp = 
}


list<MemoryBlock> g_heap;

void test( void* stack, int a )
{
}



int g_referenceCounts[numThreads][maxObjects] = {};

int g_maybeGarbageCount[numThreads] = {};
int g_maybeGarbageIds[numThreads][bufferSize];



void AddRef( int id )
{
	++g_referenceCounts[ThreadId()][id];
}

void DecRef( int id )
{
	--g_referenceCounts[ThreadId()][id];

	int slot = g_maybeGarbageCount[ThreadId()]++;
	g_maybeGarbageIds[ThreadId()][slot] = id;
}


int CollectGarbage()
{
	int maybeGarbageCount = 0;
	int* maybeGarbageIds = Merge( g_maybeGarbageIds, g_maybeGarbageCount, NumThreads(), &maybeGarbageCount );
	maybeGarbageIds = SortAndRemoveDuplicates( maybeGarbageIds, &maybeGarbageCount );
	for( int i=0; i!=maybeGarbageCount; ++i )
	{
		int id = maybeGarbageIds[i];
		int count = 0;
		for( int t=0, end=NumThreads(); t!=end; ++t )
		{
			count += g_referenceCounts[t][id];
		}
		if( count == 0 )
			toDestruct.append(id);
	}
}*/