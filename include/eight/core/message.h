//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/blob/types.h>
#include <eight/core/alloc/stack.h>
#include <eight/core/macro.h>
#include "stddef.h"
namespace eight {
//------------------------------------------------------------------------------

typedef void (FnTask)( void* o, void* b, uint size );

struct ArgHeader;

struct CallBlob
{
	void* arg;
	void* out;
};
struct FutureCall
{
	FnTask* call;
	void* obj;
	u16 argSize;
	u16 returnIndex;
	u32 returnOffset;
	Offset<ArgHeader,s16> args;
	Offset<FutureCall,s16> next;
};

class CallBuffer
{
	uint m_nextReturnIndex;
	StackMeasure m_returnSize;
	FutureCall* m_first;
	FutureCall* m_last;
	StackAlloc& a;
	void Insert(FutureCall* data);
public:
	CallBuffer( StackAlloc& a ) : m_nextReturnIndex(), m_returnSize(), m_first(), m_last(), a(a) {} 
	template<class F>
	FutureCall* Push( void* obj, FnTask* call, uint argSize, F fn );
	FutureCall* Push( FnTask* call, void* obj, uint argSize, uint returnSize );
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

const static uint MaxArgs = 3;
struct ArgHeader { uint futureMask; void (*info)(uint& n, uint o[MaxArgs], uint s[MaxArgs]); };
template<class R, class A=Nil, class B=Nil, class C=Nil>
                                    struct ArgStore                { ArgHeader b; typedef Tuple<FutureArg<A>, FutureArg<B>, FutureArg<C>> Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = 3; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b); o[2]=offsetof(Type,c); s[0]=sizeof(A); s[1]=sizeof(B); s[2]=sizeof(C); } };
template<class R, class A, class B> struct ArgStore<R,  A,  B,Nil> { ArgHeader b; typedef Tuple<FutureArg<A>, FutureArg<B>>               Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = 2; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b);                        s[0]=sizeof(A); s[1]=sizeof(B);                 } };     
template<class R, class A>          struct ArgStore<R,  A,Nil,Nil> { ArgHeader b; typedef Tuple<FutureArg<A>>                             Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = 1; o[0]=offsetof(Type,a);                                               s[0]=sizeof(A);                                 } };                 
template<class R>                   struct ArgStore<R,Nil,Nil,Nil> { ArgHeader b; typedef Tuple<>                                         Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = 0;                                                                                                                      } };
						   
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
template<class R, class T, class A>                   struct ArgFromSig<R(T::*)(A    )> { typedef ArgStore<R,  A,Nil,Nil> Storage; typedef R Return; typedef A A; };
template<class R, class T, class A, class B>          struct ArgFromSig<R(T::*)(A,B  )> { typedef ArgStore<R,  A,  B,Nil> Storage; typedef R Return; typedef A A; typedef B B; };
template<class R, class T, class A, class B, class C> struct ArgFromSig<R(T::*)(A,B,C)> { typedef ArgStore<R,  A,  B,  C> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; };



template<memfuncptr> struct CallFromMemFunc;
/*
template<class T>
struct Future : public FutureIndex
{
};*/

template<class T> struct IsFuture              { static const bool value = false; };
//template<class T> struct IsFuture<Future<T>>   { static const bool value = true; };
template<       > struct IsFuture<FutureIndex> { static const bool value = true; };
template<class A=Nil, class B=Nil, class C=Nil>
struct FutureMask
{
	static const uint value = (IsFuture<A>::value ? 1<<0 : 0)
	                        | (IsFuture<B>::value ? 1<<1 : 0)
	                        | (IsFuture<C>::value ? 1<<2 : 0);
};

template<class F> // see eiPush
FutureCall* CallBuffer::Push( void* obj, FnTask* task, uint argSize, F fn )
{
	return Push( task, obj, argSize, ReturnType<ArgFromSig<F>::Return>::Size() );
}

template<memfuncptr fn, class F, class T> 
FutureIndex Push(CallBuffer& q, T& user, F)
{
	typedef ArgFromSig<F> Arg;
	typedef Arg::Storage Args;
	typedef ReturnType<Arg::Return> Return;
	FutureCall* msg = q.Push( CallFromMemFunc<fn>::value, &user, 0, Return::Size() );
	Args& out = *(Args*)msg->args.Ptr();
	Args args = { FutureMask<>::value, &Args::Info, {} };
	out = args;
	FutureIndex fi = { msg->returnIndex, 0 };
	return fi;
}
template<memfuncptr fn, class F, class T, class A>
FutureIndex Push(CallBuffer& q, T& user, F, A a)
{
	typedef ArgFromSig<F> Arg;
	typedef Arg::Storage Args;
	typedef ReturnType<Arg::Return> Return;
	FutureCall* msg = q.Push( CallFromMemFunc<fn>::value, &user, sizeof(Args), Return::Size() );
	Args& out = *(Args*)msg->args.Ptr();
	Args args = { FutureMask<A>::value, &Args::Info, {FutureArg<Arg::A>(a)} };
	out = args;
	FutureIndex fi = { msg->returnIndex, 0 };
	return fi;
}
template<memfuncptr fn, class F, class T, class A, class B>
FutureIndex Push(CallBuffer& q, T& user, F, A a, B b)
{
	typedef ArgFromSig<F> Arg;
	typedef Arg::Storage Args;
	typedef ReturnType<Arg::Return> Return;
	FutureCall* msg = q.Push( CallFromMemFunc<fn>::value, &user, sizeof(Args), Return::Size() );
	Args& out = *(Args*)msg->args.Ptr();
	Args args = { FutureMask<A,B>::value, &Args::Info, {FutureArg<Arg::A>(a), FutureArg<Arg::B>(b)} };
	out = args;
	FutureIndex fi = { msg->returnIndex, 0 };
	return fi;
}
template<memfuncptr fn, class F, class T, class A, class B, class C>
FutureIndex Push(CallBuffer& q, T& user, F, A a, B b, C c)
{
	typedef ArgFromSig<F> Arg;
	typedef Arg::Storage Args;
	typedef ReturnType<Arg::Return> Return;
	FutureCall* msg = q.Push( CallFromMemFunc<fn>::value, &user, sizeof(Args), Return::Size() );
	Args& out = *(Args*)msg->args.Ptr();
	Args args = { FutureMask<A,B,C>::value, &Args::Info, {FutureArg<Arg::A>(a), FutureArg<Arg::B>(b), FutureArg<Arg::C>(c)} };
	out = args;
	return msg;
}


template<class T, class F>
void Call(F fn, void* user, void* argBlob, void* outBlob)
{
	typedef ArgFromSig<F>::Storage Args;
	typedef ReturnType<ArgFromSig<F>::Return> Return;
	Args aDefault = {};
	Return rDefault;
	Args& a = argBlob ? *((Args*)argBlob) : aDefault;
	Return& r = outBlob ? *((Return*)outBlob) : rDefault;
	eiASSERT( user );
	Call( r, a.t, (T*)user, fn );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
struct FetchLuaArg
{
	FetchLuaArg(void* l, uint* f) : luaState(l), futureMask(f), i() {} void* luaState; uint* futureMask, i;
	template<class T> void operator()(T& arg)
	{
		//arg = LuaFetch<T>(luaState)  /  LuaFetch(luaState, &arg)
		arg = T();
		*futureMask |= 0;
		++i;
	}
};
template<class F> FutureIndex LuaPush(CallBuffer& q, FnTask* call, void* user, void* luaState, F fn)
{
	typedef ArgFromSig<F>::Storage Args;
	typedef ArgFromSig<F>::Return Return;
	FutureCall* msg = q.Push( call, user, sizeof(Args), ReturnType<Return>::Size() );
	Args& args = *(Args*)msg->args.Ptr();
	ArgHeader header = { 0, &Args::Info };
	args.t.ForEach( FetchLuaArg(luaState, &header.futureMask) );
	args.b = header;
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


#define eiPush(q,o,N,...) Push<(memfuncptr)&N>(q, o, &N, __VA_ARGS__)

#define eiMessage( T, NAME )																					\
void Call_##T##_##NAME(void* user, void* blob, uint size)														\
{																												\
	eiASSERT( size == sizeof(CallBlob) );																		\
	CallBlob& data = *(CallBlob*)blob;																			\
	Call<T>(&T::NAME, user, data.arg, data.out );																\
}																												\
void Push_##T##_##NAME(CallBuffer& q, void* user, void* blob, uint size)										\
{																												\
	FutureCall* msg = q.Push( user, &Call_##T##_##NAME, size, &T::NAME );										\
	ArgHeader* out = msg->args.Ptr(); eiASSERT(size >= sizeof(ArgHeader) && size <= msg->argSize);				\
	memcpy( out, blob, size );																					\
}																												\
template<> struct CallFromMemFunc<(memfuncptr)&T::NAME> { static FnTask* value; };								\
FnTask* CallFromMemFunc<(memfuncptr)&T::NAME>::value = &Call_##T##_##NAME;										\
eiUserMsgEntry( T, NAME )																						\
template<class N, class t> struct MsgFuncs; class Tag##NAME;													\
template<> struct MsgFuncs<Tag##NAME,T> { static void Call(void* user, void* blob, uint size) {					\
															Call_##T##_##NAME(user, blob, size); }				\
										  static void Push(CallBuffer& q, void* user, void* blob, uint size) {	\
															Push_##T##_##NAME(q, user, blob, size); }			\
										  eiUserMsgFuncs( T, NAME )												\
										};																		//

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
