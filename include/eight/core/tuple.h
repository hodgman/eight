//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------

template<class A=Nil, class B=Nil, class C=Nil, class D=Nil, class E=Nil>
                                             struct Tuple                      { A a; B b; C c; D d; E e; template<class F> void ForEach(F& f) { f(a); f(b); f(c); f(d); f(e); } template<class F> void ForEach(F& f) const { f(a); f(b); f(c); f(d); f(e); } const static uint length = 5; typedef A   A; typedef B   B; typedef C   C; typedef D   D; typedef E   E; };
template<class A, class B, class C, class D> struct Tuple<  A,  B,  C,  D,Nil> { A a; B b; C c; D d;      template<class F> void ForEach(F& f) { f(a); f(b); f(c); f(d);       } template<class F> void ForEach(F& f) const { f(a); f(b); f(c); f(d);       } const static uint length = 4; typedef A   A; typedef B   B; typedef C   C; typedef D   D; typedef Nil E; };
template<class A, class B, class C>          struct Tuple<  A,  B,  C,Nil,Nil> { A a; B b; C c;           template<class F> void ForEach(F& f) { f(a); f(b); f(c);             } template<class F> void ForEach(F& f) const { f(a); f(b); f(c);             } const static uint length = 3; typedef A   A; typedef B   B; typedef C   C; typedef Nil D; typedef Nil E; };
template<class A, class B>                   struct Tuple<  A,  B,Nil,Nil,Nil> { A a; B b;                template<class F> void ForEach(F& f) { f(a); f(b);                   } template<class F> void ForEach(F& f) const { f(a); f(b);                   } const static uint length = 2; typedef A   A; typedef B   B; typedef Nil C; typedef Nil D; typedef Nil E; };
template<class A>                            struct Tuple<  A,Nil,Nil,Nil,Nil> { A a;                     template<class F> void ForEach(F& f) { f(a);                         } template<class F> void ForEach(F& f) const { f(a);                         } const static uint length = 1; typedef A   A; typedef Nil B; typedef Nil C; typedef Nil D; typedef Nil E; };
template<>                                   struct Tuple<Nil,Nil,Nil,Nil,Nil> {                          template<class F> void ForEach(F& f) {                               } template<class F> void ForEach(F& f) const {                               } const static uint length = 0; typedef Nil A; typedef Nil B; typedef Nil C; typedef Nil D; typedef Nil E; };
																																												 

template<class T> struct ToPointer      { typedef T*  Type; };
template<>        struct ToPointer<Nil> { typedef Nil Type; };
template<class T> struct TupleToTupleOfPointers
{ typedef Tuple<
	typename ToPointer<typename T::A>::Type, 
	typename ToPointer<typename T::B>::Type,
	typename ToPointer<typename T::C>::Type,
	typename ToPointer<typename T::D>::Type,
	typename ToPointer<typename T::E>::Type
	> Type;
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
