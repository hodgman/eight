//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/blob/types.h>
#include <eight/core/alloc/stack.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/macro.h>
#include <eight/core/bind.h>
#include <eight/core/tuple.h>
#include <eight/core/traits.h>
#include "stddef.h"
namespace eight {
//------------------------------------------------------------------------------
	
#define eiPush(q,o,N,...) Push<(memfuncptr)&N>(q, o, &N, __VA_ARGS__)

struct ArgHeader;

struct FutureCall
{
	FnMethod* call;
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
//	FutureCall* Push( void* obj, FnMethod* call, uint argSize, F fn );
	FutureCall* Push( FnMethod* call, void* obj, uint argSize, uint returnSize );
	FutureCall* First()             { return m_first; }
	FutureCall* Last()              { return m_last; }
	uint        ReturnValuesCount() { return m_nextReturnIndex; }
	uint        ReturnValuesSize()  { return m_returnSize.Bytes(); }
	void        Clear()             { m_first = m_last = 0; m_nextReturnIndex = 0; m_returnSize.Clear(); }
};

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

template<class T> struct ReturnType       { typedef   T Type; static uint Size() { return sizeof(T); }   T value; operator T&() { return value; } void operator=(const T& i){value = i;}};
template<>        struct ReturnType<void> { typedef Nil Type; static uint Size() { return 0; }         Nil value; };

const static uint MaxArgs = 9;
struct ArgHeader { uint futureMask; void (*info)(uint& n, uint o[MaxArgs], uint s[MaxArgs]); };
template<class R, class A=Nil, class B=Nil, class C=Nil, class D=Nil, class E=Nil, class F=Nil, class G=Nil, class H=Nil, class I=Nil, class J=Nil>
                                                      struct ArgStore                      { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>, MsgArg<F>, MsgArg<G>, MsgArg<H>, MsgArg<I>, MsgArg<J>> Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b); o[2]=offsetof(Type,c); o[3]=offsetof(Type,d); o[4]=offsetof(Type,e); o[5]=offsetof(Type,f); o[6]=offsetof(Type,g); o[7]=offsetof(Type,h); o[8]=offsetof(Type,i); o[9]=offsetof(Type,j); s[0]=sizeof(A); s[1]=sizeof(B); s[2]=sizeof(C); s[3]=sizeof(D); s[4]=sizeof(E); s[5]=sizeof(F); s[6]=sizeof(G); s[7]=sizeof(H); s[8]=sizeof(I); s[9]=sizeof(J); } };
template<class R, class A, class B, class C, class D, class E, 
                  class F, class G, class H, class I> struct ArgStore<R,A,B,C,D,E,F,G,H,I> { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>, MsgArg<F>, MsgArg<G>, MsgArg<H>, MsgArg<I>> Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b); o[2]=offsetof(Type,c); o[3]=offsetof(Type,d); o[4]=offsetof(Type,e); o[5]=offsetof(Type,f); o[6]=offsetof(Type,g); o[7]=offsetof(Type,h); o[8]=offsetof(Type,i);  s[0]=sizeof(A); s[1]=sizeof(B); s[2]=sizeof(C); s[3]=sizeof(D); s[4]=sizeof(E); s[5]=sizeof(F); s[6]=sizeof(G); s[7]=sizeof(H); s[8]=sizeof(I); } };     
template<class R, class A, class B, class C, class D,
                  class E, class F, class G, class H> struct ArgStore<R,A,B,C,D,E,F,G,H> { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>, MsgArg<F>, MsgArg<G>, MsgArg<H>> Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b); o[2]=offsetof(Type,c); o[3]=offsetof(Type,d); o[4]=offsetof(Type,e); o[5]=offsetof(Type,f); o[6]=offsetof(Type,g); o[7]=offsetof(Type,h);  s[0]=sizeof(A); s[1]=sizeof(B); s[2]=sizeof(C); s[3]=sizeof(D); s[4]=sizeof(E); s[5]=sizeof(F); s[6]=sizeof(G); s[7]=sizeof(H); } };     
template<class R, class A, class B, class C, class D,
                  class E, class F, class G>          struct ArgStore<R,A,B,C,D,E,F,G> { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>, MsgArg<F>, MsgArg<G>> Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b); o[2]=offsetof(Type,c); o[3]=offsetof(Type,d); o[4]=offsetof(Type,e); o[5]=offsetof(Type,f); o[6]=offsetof(Type,g);  s[0]=sizeof(A); s[1]=sizeof(B); s[2]=sizeof(C); s[3]=sizeof(D); s[4]=sizeof(E); s[5]=sizeof(F); s[6]=sizeof(G); } };     
template<class R, class A, class B, class C, class D,
                  class E, class F>                   struct ArgStore<R,A,B,C,D,E,F> { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>, MsgArg<F>> Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b); o[2]=offsetof(Type,c); o[3]=offsetof(Type,d); o[4]=offsetof(Type,e); o[5]=offsetof(Type,f);   s[0]=sizeof(A); s[1]=sizeof(B); s[2]=sizeof(C); s[3]=sizeof(D); s[4]=sizeof(E); s[5]=sizeof(F);  } };     
template<class R, class A, class B, class C, class D,
                  class E>                            struct ArgStore<R,A,B,C,D,E> { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>> Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b); o[2]=offsetof(Type,c); o[3]=offsetof(Type,d); o[4]=offsetof(Type,e);  s[0]=sizeof(A); s[1]=sizeof(B); s[2]=sizeof(C); s[3]=sizeof(D); s[4]=sizeof(E);; } };     
template<class R, class A, class B, class C, class D> struct ArgStore<R,A,B,C,D> { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>> Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b); o[2]=offsetof(Type,c); o[3]=offsetof(Type,d);  s[0]=sizeof(A); s[1]=sizeof(B); s[2]=sizeof(C); s[3]=sizeof(D); } };     
template<class R, class A, class B, class C>          struct ArgStore<R,A,B,C> { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>>            Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b); o[2]=offsetof(Type,c);  s[0]=sizeof(A); s[1]=sizeof(B); s[2]=sizeof(C); } };     
template<class R, class A, class B>                   struct ArgStore<R,A,B> { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>>                       Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a); o[1]=offsetof(Type,b);  s[0]=sizeof(A); s[1]=sizeof(B); } };     
template<class R, class A>                            struct ArgStore<R,A> { ArgHeader b; typedef Tuple<MsgArg<A>>                                  Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; o[0]=offsetof(Type,a);  s[0]=sizeof(A); } };                 
template<class R>                                     struct ArgStore<R> { ArgHeader b; typedef Tuple<>                                           Type; Type t; static void Info(uint& n, uint o[MaxArgs], uint s[MaxArgs]) { n = Type::length; } };
						   
template<class R, class T, class Fn, class A, class B, class C,
         class D                                              > void Call(ReturnType<R>& out, Tuple<A,B,C,D>& t, T* obj, Fn fn) { out = ((*obj).*(fn))( t.a, t.b, t.c, t.d ); } 
template<class R, class T, class Fn, class A, class B, class C> void Call(ReturnType<R>& out, Tuple<A,B,C  >& t, T* obj, Fn fn) { out = ((*obj).*(fn))( t.a, t.b, t.c      ); } 
template<class R, class T, class Fn, class A, class B>          void Call(ReturnType<R>& out, Tuple<A,B    >& t, T* obj, Fn fn) { out = ((*obj).*(fn))( t.a, t.b           ); } 
template<class R, class T, class Fn, class A>                   void Call(ReturnType<R>& out, Tuple<A      >& t, T* obj, Fn fn) { out = ((*obj).*(fn))( t.a                ); } 
template<class R, class T, class Fn>                            void Call(ReturnType<R>& out, Tuple<       >& t, T* obj, Fn fn) { out = ((*obj).*(fn))(                    ); } 
																																									      
template<class T, class Fn, class A, class B, class C, class D> void Call(ReturnType<void>&,  Tuple<A,B,C,D>& t, T* obj, Fn fn) {       ((*obj).*(fn))( t.a, t.b, t.c, t.d ); } 
template<class T, class Fn, class A, class B, class C>          void Call(ReturnType<void>&,  Tuple<A,B,C  >& t, T* obj, Fn fn) {       ((*obj).*(fn))( t.a, t.b, t.c      ); } 
template<class T, class Fn, class A, class B>                   void Call(ReturnType<void>&,  Tuple<A,B    >& t, T* obj, Fn fn) {       ((*obj).*(fn))( t.a, t.b           ); } 
template<class T, class Fn, class A>                            void Call(ReturnType<void>&,  Tuple<A      >& t, T* obj, Fn fn) {       ((*obj).*(fn))( t.a                ); } 
template<class T, class Fn>                                     void Call(ReturnType<void>&,  Tuple<       >& t, T* obj, Fn fn) {       ((*obj).*(fn))(                    ); } 
						   
template<class R, class Fn, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
                                                                void Call(ReturnType<R>& out, Tuple<A,B,C,D,E,F,G,H,I,J>& t, Fn fn) {out = (    fn )( t.a, t.b, t.c, t.d, t.e, t.f, t.g, t.h, t.i, t.j ); } 
template<class R, class Fn, class A, class B, class C, class D, class E, class F, class G, class H, class I>
                                                                void Call(ReturnType<R>& out, Tuple<A,B,C,D,E,F,G,H,I>& t, Fn fn) {out = (      fn )( t.a, t.b, t.c, t.d, t.e, t.f, t.g, t.h, t.i ); } 
template<class R, class Fn, class A, class B, class C, class D, class E, class F, class G, class H>
                                                                void Call(ReturnType<R>& out, Tuple<A,B,C,D,E,F,G,H>& t, Fn fn) {out = (        fn )( t.a, t.b, t.c, t.d, t.e, t.f, t.g, t.h ); } 
template<class R, class Fn, class A, class B, class C, class D, class E, class F, class G>
                                                                void Call(ReturnType<R>& out, Tuple<A,B,C,D,E,F,G>& t, Fn fn) { out = (         fn )( t.a, t.b, t.c, t.d, t.e, t.f, t.g ); } 
template<class R, class Fn, class A, class B, class C, class D, class E, class F>
                                                                void Call(ReturnType<R>& out, Tuple<A,B,C,D,E,F>& t, Fn fn) {   out = (         fn )( t.a, t.b, t.c, t.d, t.e, t.f ); } 
template<class R, class Fn, class A, class B, class C, class D, class E> 
                                                                void Call(ReturnType<R>& out, Tuple<A,B,C,D,E>& t,     Fn fn) { out = (         fn )( t.a, t.b, t.c, t.d, t.e ); } 
template<class R, class Fn, class A, class B, class C, class D> void Call(ReturnType<R>& out, Tuple<A,B,C,D>& t,       Fn fn) { out = (         fn )( t.a, t.b, t.c, t.d ); } 
template<class R, class Fn, class A, class B, class C>          void Call(ReturnType<R>& out, Tuple<A,B,C  >& t,       Fn fn) { out = (         fn )( t.a, t.b, t.c      ); } 
template<class R, class Fn, class A, class B>                   void Call(ReturnType<R>& out, Tuple<A,B    >& t,       Fn fn) { out = (         fn )( t.a, t.b           ); } 
template<class R, class Fn, class A>                            void Call(ReturnType<R>& out, Tuple<A      >& t,       Fn fn) { out = (         fn )( t.a                ); } 
template<class R, class Fn>                                     void Call(ReturnType<R>& out, Tuple<       >& t,       Fn fn) { out = (         fn )(                    ); } 
				          																					         				          	 
template<class Fn, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
                                                                void Call(ReturnType<void>&,  Tuple<A,B,C,D,E,F,G,H,I,J>& t, Fn fn) {   (       fn )( t.a, t.b, t.c, t.d, t.e, t.f, t.g, t.h, t.i, t.j ); } 
template<class Fn, class A, class B, class C, class D, class E, class F, class G, class H, class I>
                                                                void Call(ReturnType<void>&,  Tuple<A,B,C,D,E,F,G,H,I>& t, Fn fn) {   (         fn )( t.a, t.b, t.c, t.d, t.e, t.f, t.g, t.h, t.i ); } 
template<class Fn, class A, class B, class C, class D, class E, class F, class G, class H>
                                                                void Call(ReturnType<void>&,  Tuple<A,B,C,D,E,F,G,H>& t, Fn fn) {     (         fn )( t.a, t.b, t.c, t.d, t.e, t.f, t.g, t.h ); } 
template<class Fn, class A, class B, class C, class D, class E, class F, class G>
                                                                void Call(ReturnType<void>&,  Tuple<A,B,C,D,E,F,G>& t, Fn fn) {       (         fn )( t.a, t.b, t.c, t.d, t.e, t.f, t.g ); } 
template<class Fn, class A, class B, class C, class D, class E, class F>
                                                                void Call(ReturnType<void>&,  Tuple<A,B,C,D,E,F>& t, Fn fn) {         (         fn )( t.a, t.b, t.c, t.d, t.e, t.f ); } 
template<class Fn, class A, class B, class C, class D, class E> void Call(ReturnType<void>&,  Tuple<A,B,C,D,E>& t,     Fn fn) {       (         fn )( t.a, t.b, t.c, t.d, t.e ); } 
template<class Fn, class A, class B, class C, class D>          void Call(ReturnType<void>&,  Tuple<A,B,C,D>& t,       Fn fn) {       (         fn )( t.a, t.b, t.c, t.d ); } 
template<class Fn, class A, class B, class C>                   void Call(ReturnType<void>&,  Tuple<A,B,C  >& t,       Fn fn) {       (         fn )( t.a, t.b, t.c      ); } 
template<class Fn, class A, class B>                            void Call(ReturnType<void>&,  Tuple<A,B    >& t,       Fn fn) {       (         fn )( t.a, t.b           ); } 
template<class Fn, class A>                                     void Call(ReturnType<void>&,  Tuple<A      >& t,       Fn fn) {       (         fn )( t.a                ); } 
template<class Fn>                                              void Call(ReturnType<void>&,  Tuple<       >& t,       Fn fn) {       (         fn )(                    ); } 


template<class F> struct ArgFromSig;
template<class R, class T>                                     struct ArgFromSig<R(T::*)(void   )> { typedef ArgStore<R> Storage; typedef R Return; };
template<class R, class T, class A>                            struct ArgFromSig<R(T::*)(A      )> { typedef ArgStore<R,A> Storage; typedef R Return; typedef A A; };
template<class R, class T, class A, class B>                   struct ArgFromSig<R(T::*)(A,B    )> { typedef ArgStore<R,A,B> Storage; typedef R Return; typedef A A; typedef B B; };
template<class R, class T, class A, class B, class C>          struct ArgFromSig<R(T::*)(A,B,C  )> { typedef ArgStore<R,A,B,C> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; };
template<class R, class T, class A, class B, class C, class D> struct ArgFromSig<R(T::*)(A,B,C,D)> { typedef ArgStore<R,A,B,C,D> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; };

template<class R>                                              struct ArgFromSig<R(   *)(void   )> { typedef ArgStore<R> Storage; typedef R Return; };
template<class R,          class A>                            struct ArgFromSig<R(   *)(A      )> { typedef ArgStore<R,A> Storage; typedef R Return; typedef A A; };
template<class R,          class A, class B>                   struct ArgFromSig<R(   *)(A,B    )> { typedef ArgStore<R,A,B> Storage; typedef R Return; typedef A A; typedef B B; };
template<class R,          class A, class B, class C>          struct ArgFromSig<R(   *)(A,B,C  )> { typedef ArgStore<R,A,B,C> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; };
template<class R,          class A, class B, class C, class D> struct ArgFromSig<R(   *)(A,B,C,D)> { typedef ArgStore<R,A,B,C,D> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; };
template<class R,          class A, class B, class C, class D, class E>
                                                      struct ArgFromSig<R(   *)(A,B,C,D,E        )> { typedef ArgStore<R,A,B,C,D,E> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; };
template<class R,          class A, class B, class C, class D, class E, class F>
                                                      struct ArgFromSig<R(   *)(A,B,C,D,E,F      )> { typedef ArgStore<R,A,B,C,D,E,F> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; };
template<class R,          class A, class B, class C, class D, class E, class F, class G>
                                                      struct ArgFromSig<R(   *)(A,B,C,D,E,F,G    )> { typedef ArgStore<R,A,B,C,D,E,F,G> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; };
template<class R,          class A, class B, class C, class D, class E, class F, class G, class H>
                                                      struct ArgFromSig<R(   *)(A,B,C,D,E,F,G,H  )> { typedef ArgStore<R,A,B,C,D,E,F,G,H> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef H H; };
template<class R,          class A, class B, class C, class D, class E, class F, class G, class H, class I>
                                                      struct ArgFromSig<R(   *)(A,B,C,D,E,F,G,H,I)> { typedef ArgStore<R,A,B,C,D,E,F,G,H,I> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef H H; typedef I I; };
template<class R,          class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
                                                      struct ArgFromSig<R(   *)(A,B,C,D,E,F,G,H,I,J)> { typedef ArgStore<R,A,B,C,D,E,F,G,H,I,J> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef H H; typedef I I; typedef J J; };


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
FutureCall* CallBuffer::Push( void* obj, FnMethod* task, uint argSize, F fn )
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

template<callback fn, class F>
void FnCallWrapper(void* argBlob, void* outBlob)
{
	typedef ArgFromSig<F>::Storage Args;
	typedef ReturnType<ArgFromSig<F>::Return> Return;
	Args aDefault = {};
	Return rDefault;
	Args& a = argBlob ? *((Args*)argBlob) : aDefault;
	Return& r = outBlob ? *((Return*)outBlob) : rDefault;
	Call( r, a.t, (F)fn );
}

template<class T, memfuncptr fn, class F>
FutureIndex PushWrapper(CallBuffer& q, void* user, void* argBlob, uint size)
{
	FutureCall* msg = q.Push( &CallWrapper<T,fn,F>, user, size, ReturnType<ArgFromSig<F>::Return>::Size() );
	ArgHeader* out = msg->args.Ptr(); eiASSERT(size >= sizeof(ArgHeader) && size <= msg->argSize);
	eiMEMCPY( out, argBlob, size );
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
