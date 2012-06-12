//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/blob/types.h>
#include <eight/core/alloc/stack.h>
#include <eight/core/macro.h>
namespace eight {
//------------------------------------------------------------------------------

typedef void (FnTask)( void* o, void* b, uint size );

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

class CallBuffer
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
	CallBuffer( StackAlloc& a ) : m_nextReturnIndex(), m_returnSize(), m_first(), m_last(), a(a) {} 
	template<class F>
	FutureCall* Push( void* obj, FnTask* task, uint argSize, F fn );
	FutureCall* Push( FnTask* task, void* obj, uint argSize, uint returnSize );
	FutureCall* First()             { return m_first; }
	FutureCall* Last()              { return m_last; }
	uint        ReturnValuesCount() { return m_nextReturnIndex; }
	uint        ReturnValuesSize()  { return m_returnSize.Bytes(); }
	void        Clear()             { m_first = m_last = 0; m_nextReturnIndex = 0; m_returnSize.Clear(); }
};



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
	FutureArg(const FutureIndex& i) : index(i) {}
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



template<memfuncptr> struct CallFromMemFunc;

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

template<class F>
FutureCall* CallBuffer::Push( void* obj, FnTask* task, uint argSize, F fn )
{
	return Push( task, obj, argSize, ReturnType<ArgFromSig<F>::Return>::Size() );
}

template<memfuncptr fn, class F, class T> 
void Push(CallBuffer& q, T& user, F)
{
	q.Push( CallFromMemFunc<fn>::value, &user, 0, 0 );
}
template<memfuncptr fn, class F, class T, class A>
void Push(CallBuffer& q, T& user, F, A a)
{
	typedef ArgFromSig<F> Info;
	Info::Storage args = { {FutureArg<A>(a)}, FutureMask<A>::value };
	FutureCall* msg = q.Push( CallFromMemFunc<fn>::value, &user, sizeof(Info::Storage), ReturnType<Info::Return>::Size() );
	Info::Storage& out = *(Info::Storage*)msg->args.Ptr();
	out = args;
}
template<memfuncptr fn, class F, class T, class A, class B>
void Push(CallBuffer& q, T& user, F, A a, B b)
{
	typedef ArgFromSig<F> Info;
	Info::Storage args = { {FutureArg<A>(a), FutureArg<B>(b)}, FutureMask<A,B>::value };
	FutureCall* msg = q.Push( CallFromMemFunc<fn>::value, &user, sizeof(Info::Storage), ReturnType<Info::Return>::Size() );
	Info::Storage& out = *(Info::Storage*)msg->args.Ptr();
	out = args;
}
template<memfuncptr fn, class F, class T, class A, class B, class C>
void Push(CallBuffer& q, T& user, F, A a, B b, C c)
{
	typedef ArgFromSig<F> Info;
	Info::Storage args = { {FutureArg<A>(a), FutureArg<B>(b), FutureArg<C>(c)}, FutureMask<A,B,C>::value };
	FutureCall* msg = q.Push( CallFromMemFunc<fn>::value, &user, sizeof(Info::Storage), ReturnType<Info::Return>::Size() );
	Info::Storage& out = *(Info::Storage*)msg->args.Ptr();
	out = args;
}


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




//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
struct FetchLuaArg
{
	FetchLuaArg(void* luaState) : luaState(luaState) {} void* luaState;
	template<class T> void operator()(T& arg)
	{
		//arg = LuaFetch<T>(luaState)  /  LuaFetch(luaState, &arg)
	}
};
template<class F> FutureIndex LuaPush(CallBuffer& q, FnTask* call, void* user, void* luaState, F fn)
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
	FutureIndex LuaPush_##T##_##NAME(CallBuffer& q, void* user, void* luaState)							\
	{																									\
		return LuaPush( q, &Call_##T##_##NAME, user, luaState, &T::NAME );								\
	}																									//
#define eiUserMsgFuncs( T, NAME )																		\
									  static void LuaPush(CallBuffer& q, void* user, void* luaState) {	\
															LuaPush_##T##_##NAME(q, user, luaState); }	//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


#ifndef eiUserMsgEntry
#define eiUserMsgEntry(...)
#endif
#ifndef eiUserMsgFuncs
#define eiUserMsgFuncs(...)
#endif



#define eiMessage( T, NAME )																					\
void Call_##T##_##NAME(void* user, void* blob, uint size)														\
{																												\
	Call<T>(user, blob, size, &T::NAME);																		\
}																												\
void Push_##T##_##NAME(CallBuffer& q, void* user, void* blob, uint size)										\
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
										  static void Push(CallBuffer& q, void* user, void* blob, uint size) {	\
															Push_##T##_##NAME(q, user, blob, size); }			\
										  eiUserMsgFuncs( T, NAME )												\
										};																		//

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
