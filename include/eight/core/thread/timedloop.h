//------------------------------------------------------------------------------
#pragma once
#include <eight/core/typeinfo.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/thread/taskloop.h>
#include <eight/core/timer/timer.h>
#include <eight/core/types.h>
#include <eight/core/blob/types.h>
namespace eight {

//------------------------------------------------------------------------------

	

typedef void (FnTask)( void* o, void* b, uint size );

//-------------------------------------------------------------------
// todo - move binding system
//-------------------------------------------------------------------
/*	namespace Binding
	{
		enum Type { Field, Method, Function };
	}*/

	struct FutureCall
	{
		FnTask* task;
		void* obj;
		Offset<u8> args;
		uint argSize;
		uint returnIndex;
		uint returnSize;
		Offset<FutureCall> next;
	};
	class TaskBuffer
	{
		uint m_nextReturnIndex;
		StackMeasure m_returnSize;
		FutureCall* m_first;
		FutureCall* m_last;
		StackAlloc& a;
		void Insert(FutureCall* data)
		{
			eiASSERT( (!m_first) == (!m_last) );
			eiASSERT( !data->next );
			if( !m_first )
			{
				m_first = m_last = data;
			}
			else
			{
				eiASSERT( !m_last->next );
				m_last->next = data;
				m_last = data;
			}
		}
	public:
		TaskBuffer( StackAlloc& a ) : m_nextReturnIndex(), m_returnSize(), m_first(), m_last(), a(a) {} 
		template<class F>
		FutureCall* Push( void* obj, FnTask* task, uint argSize, F fn );
		FutureCall* Push( FnTask* task, void* obj, uint argSize, uint returnSize );
		FutureCall* First()             { return m_first; }
		FutureCall* Last()              { return m_last; }
		uint        ReturnValuesCount() { return m_nextReturnIndex; }
		uint        ReturnValuesSize()  { return m_returnSize.Bytes(); }
		void        Clear()             { m_first = m_last = 0; m_nextReturnIndex = 0; m_returnSize.Clear(); }
	};
	typedef void (FnPush)( TaskBuffer& q, void* o, void* b, uint s );
	struct TypeBinding;
	typedef const TypeBinding& (FnReflect)(void);

	struct DataBinding
	{
		const char* name;
		memptr memvar;
		uint offset, size;
	};
	struct MethodBinding
	{
		const char* name;
		memfuncptr memfuncptr;
		FnTask* task;
		FnPush* push;
	};
	struct FunctionBinding
	{
		const char* name;
		callback func;
	};

	struct TypeBinding
	{
		const DataBinding*     data;
		uint                   dataCount;
		const MethodBinding*   method;
		uint                   methodCount;
		const FunctionBinding* function;
		uint                   functionCount;
	};



template<class T> struct Reflect;
#define eiBindStruct( a )																\
	const TypeBinding& Reflect##a();													\
	template<> struct Reflect<a> {														\
		static const TypeBinding& Binding() { return Reflect##a(); } };					\
	const TypeBinding& Reflect##a()														\
	eiBind_(a)																			//

#define eiBindClass( a )																\
	const TypeBinding& Reflect##a()														\
	{ return a::Reflect(); }															\
	template<> struct Reflect<a> {														\
		static const TypeBinding& Binding() { return Reflect##a(); } };					\
	const TypeBinding& a::Reflect()														\
	eiBind_(a)																			//

#define eiBind_( a )																	\
	{																					\
		typedef a T;																	\
		const DataBinding*     s_data = 0; uint s_dataSize = 0;	/* defaults */			\
		const MethodBinding*   s_meth = 0; uint s_methSize = 0;							\
		const FunctionBinding* s_func = 0; uint s_funcSize = 0;							\
		{																				//
# define eiBeginData()																	\
			const static DataBinding s_data[] = {	/* overwrites name of default */	//
#   define eiBindData( a )																\
				{ #a, (memptr)&T::a, offsetof(T,a), MemberSize(&T::a) },				//
#   define eiBindReference( a )															\
				{ #a,             0,             0, sizeof(void*) },					//
# define eiEndData()																	\
			};																			\
			const static uint s_dataSize = ArraySize(s_data);							//

# define eiBeginMethods()																\
			const static MethodBinding s_meth[] = {										//
#   define eiBindMethod( a )															\
				{ #a, (memfuncptr)&T::a,												\
				&MsgFuncs<Tag##a,T>::Call, &MsgFuncs<Tag##a,T>::Push },					//
# define eiEndMethods()																	\
			};																			\
			const static uint s_methSize = ArraySize(s_meth);							//

# define eiBeginFunctions()																\
			const static DataBinding s_func[] = {										//
#   define eiBindFunction( a )															\
				{ #a, (callback)&a },													//
# define eiEndFunctions()																\
			};																			\
			const static uint s_funcSize = ArraySize(s_func);							//

#define eiEndBind( )																	\
			const static TypeBinding s_binding =										\
			{																			\
				s_data, s_dataSize,														\
				s_meth, s_methSize,														\
				s_func, s_funcSize,														\
			};																			\
			return s_binding;															\
		}																				\
	}																					//

#define eiBind( a )																		\
	static const TypeBinding& Reflect();												//

template<class M, class C> uint MemberSize(M C::*) { return sizeof(M); };
/*
template<class T>          struct IsCallbacktMemFunc                  { static const bool value = false; }
template<class R, class C> struct IsCallbacktMemFunc<R (C::*)(void*)> { static const bool value = true; }
template<class T>          struct SizeOfCallbackReturn                     { static const uint value = 0; }
template<class C>          struct SizeOfCallbackReturn<void (C::*)(void*)> { static const uint value = 0; }
template<class R, class C> struct SizeOfCallbackReturn<R    (C::*)(void*)> { static const uint value = sizeof(R); }
template<class T>          struct IsMemFunc                  { static const bool value = false; };
template<class R, class C> struct IsMemFunc<R (C::*)(...)>   { static const bool value = true; };
*/
/*
template<class T>          bool IsMemFunc(T)               { return false; }
template<class R, class C> bool IsMemFunc(R (C::*)(...)) { return true; }

template<class T>          bool IsCallbackMemFunc(T)               { return false; }
template<class R, class C> bool IsCallbackMemFunc(R (C::*)(void*)) { return true; }
template<class T>          uint SizeOfCallbackReturn(T)                  { return 0; }
template<class C>          uint SizeOfCallbackReturn(void (C::*)(void*)) { return 0; }
template<class R, class C> uint SizeOfCallbackReturn(R    (C::*)(void*)) { return sizeof(R); }
template<class R>          uint SizeOfCallbackReturn(R(*)(void*)) { return sizeof(R); }
*/
//	namespace internal { class class_ { char member; void func(); };


eiInfoGroup(TimedLoop, true);

class TimedLoop 
{
	static void  s_Task       (Scope& a, Scope& frame, void* shared, void* thread, void* task)     {        ((Args*)shared)->result->ExecuteTask(a, frame, thread, task); }
	static void* s_PrepareTask(Scope& a, Scope& frame, void* shared, void* thread, void* prevTask) { return ((Args*)shared)->result->PrepareTask(a, frame, thread, prevTask); }
public:
	eiBind(TimedLoop);
	typedef void* (FnAllocThread)(Scope& a, void* user);
	typedef void* (FnPrepareTask)(Scope& a, Scope& frame, void* user, void* thread);//return 0 to break
	typedef void  (FnExecuteTask)(Scope& a, Scope& frame, void* user, void* thread, void* task);
	static void Start( Scope& scope, void* user, FnAllocThread* pfnA, FnPrepareTask* pfnP, FnExecuteTask* pfnE );
private:
	struct Args {Scope* a; void* user; FnAllocThread* pfnA; FnPrepareTask* pfnP; FnExecuteTask* pfnE; TimedLoop* result;};
	TimedLoop(Scope& a, void* user, FnPrepareTask* pfnP, FnExecuteTask* pfnE);

	struct ThreadData
	{
//		int frame;
		double prevTime;
		void* userThreadLocal;
	};

	struct FrameData
	{
		int frame;
		double time;
		void* userFrame;
	};
	
	static void* s_InitThread(Scope& thread, uint idx, TaskLoop& loop, TaskLoop::Config* cfg);
	void* PrepareTask(Scope& a, Scope& frame, void* thread, const void* prevFrame);
	void ExecuteTask(Scope& a, Scope& frame, void* threadData, void* taskData);
	
	Timer& m_timer;
	void* m_user;
	FnAllocThread* m_allocThreadLocal;
	FnPrepareTask* m_prepareTask;
	FnExecuteTask* m_executeTask;
};

//------------------------------------------------------------------------------
//#include ".hpp"
} // namespace eight
//------------------------------------------------------------------------------
