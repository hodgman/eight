//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/blob/types.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/macro.h>
#include <eight/core/bind.h>
#include <eight/core/tuple.h>
#include <eight/core/traits.h>
#include "stddef.h"
namespace eight {
//------------------------------------------------------------------------------
	
#define eiPush(q,o,N,...) Push<(memfuncptr)&N>(#o"."#N"("#__VA_ARGS__")", q, o, &N, __VA_ARGS__)

struct ArgHeader;

struct FutureCall
{
	FnMethod* call;
	void* object;
	u16 argSize;
	u16 returnIndex;
	u32 returnOffset;
	u16 returnSize;
	u16 objectSize;
	bool methodIsConst;
	ArgHeader* args;
	FutureCall* next;
	const char* tag;
};

class CallBuffer
{
	StackMeasure m_returnSize;
	FutureCall* m_first;
	FutureCall* m_last;
	Scope& a;
	uint m_count;
	uint m_nextReturnIndex;
	void Insert(FutureCall* data);
public:
	CallBuffer( Scope& a ) : m_returnSize(), m_first(), m_last(), a(a), m_count(), m_nextReturnIndex() {} 
//	template<class F>
//	FutureCall* Push( void* obj, FnMethod* call, uint argSize, F fn );
	FutureCall* Push( const char* tag, FnMethod* call, void* obj, uint argSize, uint returnSize, uint objectSize, bool methodIsConst );
	FutureCall* First() const             { return m_first; }
	FutureCall* Last()  const             { return m_last; }
	uint        Count() const             { return m_count; }
	uint        ReturnValuesCount() const { return m_nextReturnIndex; }
	uint        ReturnValuesSize()  const { return (uint)m_returnSize.Bytes(); }
	void        Clear()                   { m_first = m_last = 0; m_count = 0; m_nextReturnIndex = 0; m_returnSize.Clear(); }
};

//Pads arguments out to 32-bits to allow them to be overwritten by FutureIndex values.
//Also converts references to be stored as pointers.
template<class T> struct MsgArg
{
	MsgArg(){}
	MsgArg(typename ConstRef<T>::Type v) : value(v) {}
//	MsgArg(const FutureIndex& i) : index(i) {}
	MsgArg(const FutureIndex&) {}
	typedef typename RefToPointer<T>::Type StorageType;
//	union
//	{
		StorageType value;
//		FutureIndex index;
//	};
//	operator typename const Ref<T>::Type() const { return RefToPointer<T>::Deref(value); }
	operator typename       Ref<T>::Type()       { return RefToPointer<T>::Deref(value); }
};

struct FutureStorage : public FutureIndex
{
	FutureStorage(){}
	FutureStorage(const FutureIndex& i) { index = i.index; offset = i.offset; }
	template<class T>
	FutureStorage(const T&) {}
};

template<class T> struct ReturnType       { typedef   T Type; static uint Size() { return sizeof(T); }   T value; operator T&() { return value; } void operator=(const T& i){value = i;}};
template<>        struct ReturnType<void> { typedef Nil Type; static uint Size() { return 0; }         Nil value; };

const static uint MaxArgs = 10;
struct ArgHeader { u16 futureMask; u16 pointerMask; u16 constMask; void (*info)(uint& n, const uint*& o, const uint*& s); };
template<class R, class A=Nil, class B=Nil, class C=Nil, class D=Nil, class E=Nil, class F=Nil, class G=Nil, class H=Nil, class I=Nil, class J=Nil>
                                                      struct ArgStore                      { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>, MsgArg<F>, MsgArg<G>, MsgArg<H>, MsgArg<I>, MsgArg<J>> Type; Type t; FutureStorage futures[10]; static void Info(uint& n, const uint*& o, const uint*& s) { n = Type::length; const static uint _o[] = { offsetof(Type,a), offsetof(Type,b), offsetof(Type,c), offsetof(Type,d), offsetof(Type,e), offsetof(Type,f), offsetof(Type,g), offsetof(Type,h), o[8]=offsetof(Type,i), o[9]=offsetof(Type,j) }; const static uint _s[] = { sizeof(A), sizeof(B), sizeof(C), sizeof(D), sizeof(E), sizeof(F), sizeof(G), sizeof(H), sizeof(I),sizeof(J) }; o=_o; s=_s; } typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef H H; typedef I I; typedef J J; }; 
template<class R, class A, class B, class C, class D, class E, 																																																																																																																																																														
                  class F, class G, class H, class I> struct ArgStore<R,A,B,C,D,E,F,G,H,I> { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>, MsgArg<F>, MsgArg<G>, MsgArg<H>, MsgArg<I>>            Type; Type t; FutureStorage futures[ 9]; static void Info(uint& n, const uint*& o, const uint*& s) { n = Type::length; const static uint _o[] = { offsetof(Type,a), offsetof(Type,b), offsetof(Type,c), offsetof(Type,d), offsetof(Type,e), offsetof(Type,f), offsetof(Type,g), offsetof(Type,h), o[8]=offsetof(Type,i)                        }; const static uint _s[] = { sizeof(A), sizeof(B), sizeof(C), sizeof(D), sizeof(E), sizeof(F), sizeof(G), sizeof(H), sizeof(I)    		}; o=_o; s=_s; } typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef H H; typedef I I;   		   }; 
template<class R, class A, class B, class C, class D,																																																						  																																																																																																																																										   
                  class E, class F, class G, class H> struct ArgStore<R,A,B,C,D,E,F,G,H>   { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>, MsgArg<F>, MsgArg<G>, MsgArg<H>>                       Type; Type t; FutureStorage futures[ 8]; static void Info(uint& n, const uint*& o, const uint*& s) { n = Type::length; const static uint _o[] = { offsetof(Type,a), offsetof(Type,b), offsetof(Type,c), offsetof(Type,d), offsetof(Type,e), offsetof(Type,f), offsetof(Type,g), offsetof(Type,h)                                               }; const static uint _s[] = { sizeof(A), sizeof(B), sizeof(C), sizeof(D), sizeof(E), sizeof(F), sizeof(G), sizeof(H)  					}; o=_o; s=_s; } typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef H H; 					       }; 
template<class R, class A, class B, class C, class D,																																							                      									 								  											 																																                                            																																																																											       
                  class E, class F, class G>          struct ArgStore<R,A,B,C,D,E,F,G>     { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>, MsgArg<F>, MsgArg<G>>                                  Type; Type t; FutureStorage futures[ 7]; static void Info(uint& n, const uint*& o, const uint*& s) { n = Type::length; const static uint _o[] = { offsetof(Type,a), offsetof(Type,b), offsetof(Type,c), offsetof(Type,d), offsetof(Type,e), offsetof(Type,f), offsetof(Type,g)                                                                 }; const static uint _s[] = { sizeof(A), sizeof(B), sizeof(C), sizeof(D), sizeof(E), sizeof(F), sizeof(G)     							}; o=_o; s=_s; } typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G;    							           }; 
template<class R, class A, class B, class C, class D,																																				                                  									 							  																																							                                                              																																																																											           
                  class E, class F>                   struct ArgStore<R,A,B,C,D,E,F>       { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>, MsgArg<F>>                                             Type; Type t; FutureStorage futures[ 6]; static void Info(uint& n, const uint*& o, const uint*& s) { n = Type::length; const static uint _o[] = { offsetof(Type,a), offsetof(Type,b), offsetof(Type,c), offsetof(Type,d), offsetof(Type,e), offsetof(Type,f)                                                                                   }; const static uint _s[] = { sizeof(A), sizeof(B), sizeof(C), sizeof(D), sizeof(E), sizeof(F)   											}; o=_o; s=_s; } typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F;  									               }; 
template<class R, class A, class B, class C, class D,																																	                                              				 					 									  																																		                                                                                  																																																																										           
                  class E>                            struct ArgStore<R,A,B,C,D,E>         { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>, MsgArg<E>>                                                        Type; Type t; FutureStorage futures[ 5]; static void Info(uint& n, const uint*& o, const uint*& s) { n = Type::length; const static uint _o[] = { offsetof(Type,a), offsetof(Type,b), offsetof(Type,c), offsetof(Type,d), offsetof(Type,e)                                                                                                     }; const static uint _s[] = { sizeof(A), sizeof(B), sizeof(C), sizeof(D), sizeof(E)														}; o=_o; s=_s; } typedef A A; typedef B B; typedef C C; typedef D D; typedef E E;													               }; 
template<class R, class A, class B, class C, class D> struct ArgStore<R,A,B,C,D>           { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>, MsgArg<D>>                                                                   Type; Type t; FutureStorage futures[ 4]; static void Info(uint& n, const uint*& o, const uint*& s) { n = Type::length; const static uint _o[] = { offsetof(Type,a), offsetof(Type,b), offsetof(Type,c), offsetof(Type,d)                                                                                                                       }; const static uint _s[] = { sizeof(A), sizeof(B), sizeof(C), sizeof(D) 																	}; o=_o; s=_s; } typedef A A; typedef B B; typedef C C; typedef D D;																	           }; 
template<class R, class A, class B, class C>          struct ArgStore<R,A,B,C>             { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>, MsgArg<C>>                                                                              Type; Type t; FutureStorage futures[ 3]; static void Info(uint& n, const uint*& o, const uint*& s) { n = Type::length; const static uint _o[] = { offsetof(Type,a), offsetof(Type,b), offsetof(Type,c)                                                                                                                                         }; const static uint _s[] = { sizeof(A), sizeof(B), sizeof(C) 																			}; o=_o; s=_s; } typedef A A; typedef B B; typedef C C;																			                   }; 
template<class R, class A, class B>                   struct ArgStore<R,A,B>               { ArgHeader b; typedef Tuple<MsgArg<A>, MsgArg<B>>                                                                                         Type; Type t; FutureStorage futures[ 2]; static void Info(uint& n, const uint*& o, const uint*& s) { n = Type::length; const static uint _o[] = { offsetof(Type,a), offsetof(Type,b)                                                                                                                                                           }; const static uint _s[] = { sizeof(A), sizeof(B)    																					}; o=_o; s=_s; } typedef A A; typedef B B;   																					                   }; 
template<class R, class A>                            struct ArgStore<R,A>                 { ArgHeader b; typedef Tuple<MsgArg<A>>                                                                                                    Type; Type t; FutureStorage futures[ 1]; static void Info(uint& n, const uint*& o, const uint*& s) { n = Type::length; const static uint _o[] = { offsetof(Type,a)                                                                                                                                                                             }; const static uint _s[] = { sizeof(A)                                                                                                   }; o=_o; s=_s; } typedef A A;                                                                                                                      };                 
template<class R>                                     struct ArgStore<R>                   { ArgHeader b; typedef Tuple<>                                                                                                             Type; Type t;                          static void Info(uint& n, const uint*& o, const uint*& s) { n = Type::length; o=0; s=0; } };
						   
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
template<class R, class T>                                     struct ArgFromSig<R(T::*)(void   )> { typedef ArgStore<R> Storage; typedef R Return; typedef Nil A; typedef Nil B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; const static bool constThis = false; };
template<class R, class T, class A>                            struct ArgFromSig<R(T::*)(A      )> { typedef ArgStore<R,A> Storage; typedef R Return; typedef A A; typedef Nil B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; const static bool constThis = false; };
template<class R, class T, class A, class B>                   struct ArgFromSig<R(T::*)(A,B    )> { typedef ArgStore<R,A,B> Storage; typedef R Return; typedef A A; typedef B B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; const static bool constThis = false; };
template<class R, class T, class A, class B, class C>          struct ArgFromSig<R(T::*)(A,B,C  )> { typedef ArgStore<R,A,B,C> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; const static bool constThis = false; };
template<class R, class T, class A, class B, class C, class D> struct ArgFromSig<R(T::*)(A,B,C,D)> { typedef ArgStore<R,A,B,C,D> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; const static bool constThis = false; };

template<class R, class T>                                     struct ArgFromSig<R(T::*)(void   )const> { typedef ArgStore<R> Storage; typedef R Return; typedef Nil A; typedef Nil B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; const static bool constThis = true; };
template<class R, class T, class A>                            struct ArgFromSig<R(T::*)(A      )const> { typedef ArgStore<R,A> Storage; typedef R Return; typedef A A; typedef Nil B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; const static bool constThis = true; };
template<class R, class T, class A, class B>                   struct ArgFromSig<R(T::*)(A,B    )const> { typedef ArgStore<R,A,B> Storage; typedef R Return; typedef A A; typedef B B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; const static bool constThis = true; };
template<class R, class T, class A, class B, class C>          struct ArgFromSig<R(T::*)(A,B,C  )const> { typedef ArgStore<R,A,B,C> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; const static bool constThis = true; };
template<class R, class T, class A, class B, class C, class D> struct ArgFromSig<R(T::*)(A,B,C,D)const> { typedef ArgStore<R,A,B,C,D> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; const static bool constThis = true; };

template<class R>                                              struct ArgFromSig<R(   *)(void   )> { typedef ArgStore<R> Storage; typedef R Return; typedef Nil A; typedef Nil B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class R,          class A>                            struct ArgFromSig<R(   *)(A      )> { typedef ArgStore<R,A> Storage; typedef R Return; typedef A A; typedef Nil B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class R,          class A, class B>                   struct ArgFromSig<R(   *)(A,B    )> { typedef ArgStore<R,A,B> Storage; typedef R Return; typedef A A; typedef B B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class R,          class A, class B, class C>          struct ArgFromSig<R(   *)(A,B,C  )> { typedef ArgStore<R,A,B,C> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class R,          class A, class B, class C, class D> struct ArgFromSig<R(   *)(A,B,C,D)> { typedef ArgStore<R,A,B,C,D> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class R,          class A, class B, class C, class D, class E>
                                                      struct ArgFromSig<R(   *)(A,B,C,D,E        )> { typedef ArgStore<R,A,B,C,D,E> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class R,          class A, class B, class C, class D, class E, class F>
                                                      struct ArgFromSig<R(   *)(A,B,C,D,E,F      )> { typedef ArgStore<R,A,B,C,D,E,F> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class R,          class A, class B, class C, class D, class E, class F, class G>
                                                      struct ArgFromSig<R(   *)(A,B,C,D,E,F,G    )> { typedef ArgStore<R,A,B,C,D,E,F,G> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class R,          class A, class B, class C, class D, class E, class F, class G, class H>
                                                      struct ArgFromSig<R(   *)(A,B,C,D,E,F,G,H  )> { typedef ArgStore<R,A,B,C,D,E,F,G,H> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef H H; typedef Nil I; typedef Nil J; };
template<class R,          class A, class B, class C, class D, class E, class F, class G, class H, class I>
                                                      struct ArgFromSig<R(   *)(A,B,C,D,E,F,G,H,I)> { typedef ArgStore<R,A,B,C,D,E,F,G,H,I> Storage; typedef R Return; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef H H; typedef I I; typedef Nil J; };
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
template<class Args, class A=Nil, class B=Nil, class C=Nil, class D=Nil, class E=Nil, class F=Nil, class G=Nil, class H=Nil, class I=Nil, class J=Nil>
struct CallArgMasks
{
	static const u16 isFuture  = (IsFuture<A>::value ? 1<<0 : 0)
	                           | (IsFuture<B>::value ? 1<<1 : 0)
	                           | (IsFuture<C>::value ? 1<<2 : 0)
	                           | (IsFuture<D>::value ? 1<<3 : 0)
	                           | (IsFuture<E>::value ? 1<<4 : 0)
	                           | (IsFuture<F>::value ? 1<<5 : 0)
	                           | (IsFuture<G>::value ? 1<<6 : 0)
	                           | (IsFuture<H>::value ? 1<<7 : 0)
	                           | (IsFuture<I>::value ? 1<<8 : 0)
	                           | (IsFuture<J>::value ? 1<<9 : 0);
	static const u16 isPointer = (IsPointer<RefToPointer<typename Args::A>::Type>::value ? 1<<0 : 0)
	                           | (IsPointer<RefToPointer<typename Args::B>::Type>::value ? 1<<1 : 0)
	                           | (IsPointer<RefToPointer<typename Args::C>::Type>::value ? 1<<2 : 0)
	                           | (IsPointer<RefToPointer<typename Args::D>::Type>::value ? 1<<3 : 0)
	                           | (IsPointer<RefToPointer<typename Args::E>::Type>::value ? 1<<4 : 0)
	                           | (IsPointer<RefToPointer<typename Args::F>::Type>::value ? 1<<5 : 0)
	                           | (IsPointer<RefToPointer<typename Args::G>::Type>::value ? 1<<6 : 0)
	                           | (IsPointer<RefToPointer<typename Args::H>::Type>::value ? 1<<7 : 0)
	                           | (IsPointer<RefToPointer<typename Args::I>::Type>::value ? 1<<8 : 0)
	                           | (IsPointer<RefToPointer<typename Args::J>::Type>::value ? 1<<9 : 0);
	static const u16 isConst   = (IsConstPointer<RefToPointer<typename Args::A>::Type>::value ? 1<<0 : 0)
	                           | (IsConstPointer<RefToPointer<typename Args::B>::Type>::value ? 1<<1 : 0)
	                           | (IsConstPointer<RefToPointer<typename Args::C>::Type>::value ? 1<<2 : 0)
	                           | (IsConstPointer<RefToPointer<typename Args::D>::Type>::value ? 1<<3 : 0)
	                           | (IsConstPointer<RefToPointer<typename Args::E>::Type>::value ? 1<<4 : 0)
	                           | (IsConstPointer<RefToPointer<typename Args::F>::Type>::value ? 1<<5 : 0)
	                           | (IsConstPointer<RefToPointer<typename Args::G>::Type>::value ? 1<<6 : 0)
	                           | (IsConstPointer<RefToPointer<typename Args::H>::Type>::value ? 1<<7 : 0)
	                           | (IsConstPointer<RefToPointer<typename Args::I>::Type>::value ? 1<<8 : 0)
	                           | (IsConstPointer<RefToPointer<typename Args::J>::Type>::value ? 1<<9 : 0);
};


// see eiPush
/*
template<class F> 
FutureCall* CallBuffer::Push( void* obj, FnMethod* task, uint argSize, F fn )
{
	return Push( task, obj, argSize, ReturnType<ArgFromSig<F>::Return>::Size() );
}*/

template<memfuncptr fn, class F, class T> 
FutureIndex Push(const char* tag, CallBuffer& q, T& user, F)
{
	typedef ArgFromSig<F> Arg; typedef Arg::Storage Args; typedef ReturnType<Arg::Return> Return;
	FutureCall* msg = q.Push( tag, GetCallWrapper<T,fn,F>(), &user, 0, Return::Size(), sizeof(T), Arg::constThis );
	eiASSERT( !msg->args );
	FutureIndex fi = { msg->returnIndex, 0 };
	return fi;
}
template<memfuncptr fn, class F, class T, class A>
FutureIndex Push(const char* tag, CallBuffer& q, T& user, F, A a)
{
	typedef ArgFromSig<F> Arg; typedef Arg::Storage Args; typedef ReturnType<Arg::Return> Return; typedef CallArgMasks<Arg, A> Masks;
	FutureCall* msg = q.Push( tag, GetCallWrapper<T,fn,F>(), &user, sizeof(Args), Return::Size(), sizeof(T), Arg::constThis );
	Args& out = *(Args*)msg->args;
	Args args = { {Masks::isFuture, Masks::isPointer, Masks::isConst, &Args::Info}, {a}, {a} };
	out = args;
	FutureIndex fi = { msg->returnIndex, 0 };
	return fi;
}
template<memfuncptr fn, class F, class T, class A, class B>
FutureIndex Push(const char* tag, CallBuffer& q, T& user, F, A a, B b)
{
	typedef ArgFromSig<F> Arg; typedef Arg::Storage Args; typedef ReturnType<Arg::Return> Return; typedef CallArgMasks<Arg, A,B> Masks;
	FutureCall* msg = q.Push( tag, GetCallWrapper<T,fn,F>(), &user, sizeof(Args), Return::Size(), sizeof(T), Arg::constThis );
	Args& out = *(Args*)msg->args;
	Args args = { {Masks::isFuture, Masks::isPointer, Masks::isConst, &Args::Info}, {a,b}, {a,b} };
	out = args;
	FutureIndex fi = { msg->returnIndex, 0 };
	return fi;
}
template<memfuncptr fn, class F, class T, class A, class B, class C>
FutureIndex Push(const char* tag, CallBuffer& q, T& user, F, A a, B b, C c)
{
	typedef ArgFromSig<F> Arg; typedef Arg::Storage Args; typedef ReturnType<Arg::Return> Return; typedef CallArgMasks<Arg, A,B,C> Masks;
	FutureCall* msg = q.Push( tag, GetCallWrapper<T,fn,F>(), &user, sizeof(Args), Return::Size(), sizeof(T), Arg::constThis );
	Args& out = *(Args*)msg->args;
	Args args = { {Masks::isFuture, Masks::isPointer, Masks::isConst, &Args::Info}, {a,b,c}, {a,b,c} };
	out = args;
	FutureIndex fi = { msg->returnIndex, 0 };
	return fi;
}



template<class T, memfuncptr fn, class F>
void CallWrapper(void* user, void* argBlob, void* outBlob)
{
	typedef typename ArgFromSig<F>::Storage Args;
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
	typedef typename ArgFromSig<F>::Storage Args;
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
	FutureCall* msg = q.Push( "Lua - todo", &CallWrapper<T,fn,F>, user, size, ReturnType<ArgFromSig<F>::Return>::Size(), sizeof(T), ArgFromSig<F>::constThis );
	ArgHeader* out = msg->args; eiASSERT(size >= sizeof(ArgHeader) && size <= msg->argSize);
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
