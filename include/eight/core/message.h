//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/blob/types.h>
#include <eight/core/alloc/stack.h>
#include <eight/core/macro.h>
#include <eight/core/bind.h>
#include "stddef.h"
namespace eight {
//------------------------------------------------------------------------------
	
#define eiPush(q,o,N,...) Push<(memfuncptr)&N>(q, o, &N, __VA_ARGS__)

struct ArgHeader;

struct FutureCall
{
	FnGeneric* call;
	void* obj;
	u16 argSize;
	u16 returnIndex;
	u32 returnOffset;
	Offset<ArgHeader, s16> args;
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
//	template<class F>
//	FutureCall* Push( void* obj, FnGeneric* call, uint argSize, F fn );
	FutureCall* Push( FnGeneric* call, void* obj, uint argSize, uint returnSize );
	FutureCall* First()             { return m_first; }
	FutureCall* Last()              { return m_last; }
	uint        ReturnValuesCount() { return m_nextReturnIndex; }
	uint        ReturnValuesSize()  { return m_returnSize.Bytes(); }
	void        Clear()             { m_first = m_last = 0; m_nextReturnIndex = 0; m_returnSize.Clear(); }
};



template<class A=Nil, class B=Nil, class C=Nil>
                           struct Tuple              { A a; B b; C c; template<class F> void ForEach(F& f) { f(a); f(b); f(c); } const static uint length = 3; };
template<class A, class B> struct Tuple<  A,  B,Nil> { A a; B b;      template<class F> void ForEach(F& f) { f(a); f(b);       } const static uint length = 2; };
template<class A>          struct Tuple<  A,Nil,Nil> { A a;           template<class F> void ForEach(F& f) { f(a);             } const static uint length = 1; };
template<>                 struct Tuple<Nil,Nil,Nil> {                template<class F> void ForEach(F& f) {                   } const static uint length = 0; };


template<class T> struct IsPointer           { const static bool value = false; };
template<class T> struct IsPointer<      T*> { const static bool value = true;  };
//template<class T> struct IsPointer<const T*> { const static bool value = true;  };

template<class T> struct IsReference           { const static bool value = false; };
template<class T> struct IsReference<      T&> { const static bool value = true;  };
//template<class T> struct IsReference<const T&> { const static bool value = true;  };

template<class T> struct IsPointerOrRef           { const static bool value = false; };
template<class T> struct IsPointerOrRef<      T*> { const static bool value = true;  };
//template<class T> struct IsPointerOrRef<const T*> { const static bool value = true;  };
template<class T> struct IsPointerOrRef<      T&> { const static bool value = true;  };
//template<class T> struct IsPointerOrRef<const T&> { const static bool value = true;  };

template<class T> struct StripPointer           { typedef T Type; };//todo - all the combinations...
template<class T> struct StripPointer<      T*> { typedef T Type; };
template<class T> struct StripPointer<const T*> { typedef T Type; };

template<class T> struct StripRef           { typedef T Type; };
template<class T> struct StripRef<      T&> { typedef T Type; };
template<class T> struct StripRef<const T&> { typedef T Type; };

template<class T> struct RefToPointer           { typedef       T  Type; static       T& Deref(      T& t){return  t;} static const T& Deref(const T& t){return  t;} };
template<class T> struct RefToPointer<      T&> { typedef       T* Type; static       T& Deref(      T* t){return *t;}};
template<class T> struct RefToPointer<const T&> { typedef const T* Type; static const T& Deref(const T* t){return *t;}};

template<class T> struct ConstRef           { typedef const T& Type; };
template<class T> struct ConstRef<      T&> { typedef const T& Type; };
template<class T> struct ConstRef<const T&> { typedef const T& Type; };
template<       > struct ConstRef<void>     { typedef Nil Type; };

template<class T> struct Ref           { typedef       T& Type; };
template<class T> struct Ref<      T&> { typedef       T& Type; };
template<class T> struct Ref<const T&> { typedef const T& Type; };

//Pads arguments out to 32-bits to allow them to be overwritten by FutureIndex values.
//Also converts references to be stored as pointers.
template<class T> struct MsgArg
{
	MsgArg(){}
	MsgArg(typename ConstRef<T>::Type v) : value(v) {}
	MsgArg(const FutureIndex& i) : index(i) {}
	typedef typename RefToPointer<T>::Type StorageType;
	union
	{
		StorageType value;
		FutureIndex index;
	};
//	operator typename const Ref<T>::Type() const { return RefToPointer<T>::Deref(value); }
	operator typename       Ref<T>::Type()       { return RefToPointer<T>::Deref(value); }
};

template<class T> struct ReturnType       { typedef T Type; static uint Size() { return sizeof(T); } T value; operator T&() { return value; } void operator=(const T& i){value = i;}};
template<>        struct ReturnType<void> { typedef Nil Type; static uint Size() { return 0; } Nil value; };

const static uint MaxArgs = 3;
struct ArgHeader { uint futureMask; void (*info)(uint& n, uint o[MaxArgs], uint s[MaxArgs]); };
template<class R, class A=Nil, class B=Nil, class C=Nil>
                                    struct ArgStore                { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>> Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b); o[2]=offsetof(Type,c); s[0]=sizeof(A); s[1]=sizeof(B); s[2]=sizeof(C); } };
template<class R, class A, class B> struct ArgStore<R,  A,  B,Nil> { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>>            Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b);                        s[0]=sizeof(A); s[1]=sizeof(B);                 } };     
template<class R, class A>          struct ArgStore<R,  A,Nil,Nil> { ArgHeader b; typedef Tuple<MsgArg<A>>                       Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a);                                               s[0]=sizeof(A);                                 } };                 
template<class R>                   struct ArgStore<R,Nil,Nil,Nil> { ArgHeader b; typedef Tuple<>                                Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length;                                                                                                                      } };
						   
template<class R, class T, class F, class A, class B, class C> void Call(ReturnType<R>& out, Tuple<A,B,C>& t, T* obj, F fn) { out = ((*obj).*(fn))( t.a, t.b, t.c ); } 
template<class R, class T, class F, class A, class B>          void Call(ReturnType<R>& out, Tuple<A,B  >& t, T* obj, F fn) { out = ((*obj).*(fn))( t.a, t.b      ); } 
template<class R, class T, class F, class A>                   void Call(ReturnType<R>& out, Tuple<A    >& t, T* obj, F fn) { out = ((*obj).*(fn))( t.a           ); } 
template<class R, class T, class F>                            void Call(ReturnType<R>& out, Tuple<     >& t, T* obj, F fn) { out = ((*obj).*(fn))(               ); } 

template<         class T, class F, class A, class B, class C> void Call(ReturnType<void>&,  Tuple<A,B,C>& t, T* obj, F fn) {       ((*obj).*(fn))( t.a, t.b, t.c ); } 
template<         class T, class F, class A, class B>          void Call(ReturnType<void>&,  Tuple<A,B  >& t, T* obj, F fn) {       ((*obj).*(fn))( t.a, t.b      ); } 
template<         class T, class F, class A>                   void Call(ReturnType<void>&,  Tuple<A    >& t, T* obj, F fn) {       ((*obj).*(fn))( t.a           ); } 
template<         class T, class F>                            void Call(ReturnType<void>&,  Tuple<     >& t, T* obj, F fn) {       ((*obj).*(fn))(               ); } 


template<class F> struct ArgFromSig;
template<class R, class T>                            struct ArgFromSig<R(T::*)(void )> { typedef ArgStore<R,Nil,Nil,Nil> Storage; typedef R Return; };
template<class R, class T, class A>                   struct ArgFromSig<R(T::*)(A    )> { typedef ArgStore<R,  A,Nil,Nil> Storage; typedef R Return; typedef A A; };
template<class R, class T, class A, class B>          struct ArgFromSig<R(T::*)(A,B  )> { typedef ArgStore<R,  A,  B,Nil> Storage; typedef R Return; typedef A A; typedef B B; };
template<class R, class T, class A, class B, class C> struct ArgFromSig<R(T::*)(A,B,C)> { typedef ArgStore<R,  A,  B,  C> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; };


//template<memfuncptr> struct CallFromMemFunc;
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


// see eiPush
/*
template<class F> 
FutureCall* CallBuffer::Push( void* obj, FnGeneric* task, uint argSize, F fn )
{
	return Push( task, obj, argSize, ReturnType<ArgFromSig<F>::Return>::Size() );
}*/

template<memfuncptr fn, class F, class T> 
FutureIndex Push(CallBuffer& q, T& user, F)
{
	typedef ArgFromSig<F> Arg;
	typedef Arg::Storage Args;
	typedef ReturnType<Arg::Return> Return;
	FutureCall* msg = q.Push( GetCallWrapper<T,fn,F>(), &user, 0, Return::Size() );
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
	FutureCall* msg = q.Push( GetCallWrapper<T,fn,F>(), &user, sizeof(Args), Return::Size() );
	Args& out = *(Args*)msg->args.Ptr();
	Args args = { FutureMask<A>::value, &Args::Info, {MsgArg<Arg::A>(a)} };
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
	FutureCall* msg = q.Push( GetCallWrapper<T,fn,F>(), &user, sizeof(Args), Return::Size() );
	Args& out = *(Args*)msg->args.Ptr();
	Args args = { FutureMask<A,B>::value, &Args::Info, {MsgArg<Arg::A>(a), MsgArg<Arg::B>(b)} };
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
	FutureCall* msg = q.Push( GetCallWrapper<T,fn,F>(), &user, sizeof(Args), Return::Size() );
	Args& out = *(Args*)msg->args.Ptr();
	Args args = { FutureMask<A,B,C>::value, &Args::Info, {MsgArg<Arg::A>(a), MsgArg<Arg::B>(b), MsgArg<Arg::C>(c)} };
	out = args;
	return msg;
}



template<class T, memfuncptr fn, class F>
void CallWrapper(void* user, void* argBlob, void* outBlob)
{
	typedef ArgFromSig<F>::Storage Args;
	typedef ReturnType<ArgFromSig<F>::Return> Return;
	Args aDefault = {};
	Return rDefault;
	Args& a = argBlob ? *((Args*)argBlob) : aDefault;
	Return& r = outBlob ? *((Return*)outBlob) : rDefault;
	eiASSERT( user );
	Call( r, a.t, (T*)user, (F)fn );
}

template<class T, memfuncptr fn, class F>
FutureIndex PushWrapper(CallBuffer& q, void* user, void* argBlob, uint size)
{
	FutureCall* msg = q.Push( &CallWrapper<T,fn,F>, user, size, ReturnType<ArgFromSig<F>::Return>::Size() );
	ArgHeader* out = msg->args.Ptr(); eiASSERT(size >= sizeof(ArgHeader) && size <= msg->argSize);
	memcpy( out, argBlob, size );
	FutureIndex fi = { msg->returnIndex, 0 };
	return fi;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------




//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
