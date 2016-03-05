//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------

template<class A=Nil, class B=Nil, class C=Nil, class D=Nil, class E=Nil, class F=Nil, class G=Nil, class H=Nil, class I=Nil, class J=Nil>
                                             struct Tuple                      { A a;B b;C c;D d;E e;F f;G g;H h;I i;J j; template<class F> void ForEach(F& _) { _(a);_(b);_(c);_(d);_(e);_(f);_(g);_(h);_(i);_(j); } template<class F> void ForEach(F&_) const { _(a);_(b);_(c);_(d);_(e);_(f);_(g);_(h);_(i);_(j); } const static uint length = 10; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef H H; typedef I I; typedef J J; };
template<class A, class B, class C, class D, class E, 
         class F, class G, class H, class I> struct Tuple<A,B,C,D,E,F,G,H,I>   { A a;B b;C c;D d;E e;F f;G g;H h;I i; template<class F> void ForEach(F&_) { _(a);_(b);_(c);_(d);_(e);_(f);_(g);_(h);_(i); } template<class F> void ForEach(F&_) const { _(a);_(b);_(c);_(d);_(e);_(f);_(g);_(h);_(i); } const static uint length = 9; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef H H; typedef I I; typedef Nil J; };
template<class A, class B, class C, class D, 
         class E, class F, class G, class H> struct Tuple<A,B,C,D,E,F,G,H>     { A a;B b;C c;D d;E e;F f;G g;H h; template<class F> void ForEach(F&_) { _(a);_(b);_(c);_(d);_(e);_(f);_(g);_(h); } template<class F> void ForEach(F&_) const { _(a);_(b);_(c);_(d);_(e);_(f);_(g);_(h); } const static uint length = 8; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef H H; typedef Nil I; typedef Nil J; };
template<class A, class B, class C, class D, 
         class E, class F, class G>          struct Tuple<A,B,C,D,E,F,G>       { A a;B b;C c;D d;E e;F f;G g; template<class F> void ForEach(F&_) { _(a);_(b);_(c);_(d);_(e);_(f);_(g); } template<class F> void ForEach(F&_) const { _(a);_(b);_(c);_(d);_(e);_(f);_(g); } const static uint length = 7; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef G G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class A, class B, class C, class D, 
         class E, class F>                   struct Tuple<A,B,C,D,E,F>         { A a;B b;C c;D d;E e;F f; template<class F> void ForEach(F&_) { _(a);_(b);_(c);_(d);_(e);_(f); } template<class F> void ForEach(F&_) const { _(a);_(b);_(c);_(d);_(e);_(f); } const static uint length = 6; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef F F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class A, class B, class C, class D, 
         class E>                            struct Tuple<A,B,C,D,E>           { A a;B b;C c;D d;E e; template<class F> void ForEach(F&_) { _(a);_(b);_(c);_(d);_(e); } template<class F> void ForEach(F&_) const { _(a);_(b);_(c);_(d);_(e); } const static uint length = 5; typedef A A; typedef B B; typedef C C; typedef D D; typedef E E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class A, class B, class C, class D> struct Tuple<  A,  B,  C,  D,Nil> { A a;B b;C c;D d; template<class F> void ForEach(F&_) { _(a);_(b);_(c);_(d);       } template<class F> void ForEach(F&_) const { _(a);_(b);_(c);_(d);       } const static uint length = 4; typedef A A; typedef B B; typedef C C; typedef D D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class A, class B, class C>          struct Tuple<  A,  B,  C,Nil,Nil> { A a;B b;C c; template<class F> void ForEach(F&_) { _(a);_(b);_(c);             } template<class F> void ForEach(F&_) const { _(a);_(b);_(c);             } const static uint length = 3; typedef A A; typedef B B; typedef C C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class A, class B>                   struct Tuple<  A,  B,Nil,Nil,Nil> { A a;B b; template<class F> void ForEach(F&_) { _(a);_(b);                   } template<class F> void ForEach(F&_) const { _(a);_(b);                   } const static uint length = 2; typedef A A; typedef B B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<class A>                            struct Tuple<  A,Nil,Nil,Nil,Nil> { A a; template<class F> void ForEach(F&_) { _(a);                         } template<class F> void ForEach(F&_) const { _(a);                         } const static uint length = 1; typedef A A; typedef Nil B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
template<>                                   struct Tuple<Nil,Nil,Nil,Nil,Nil> { template<class F> void ForEach(F&_) {                               } template<class F> void ForEach(F&_) const {                               } const static uint length = 0; typedef Nil A; typedef Nil B; typedef Nil C; typedef Nil D; typedef Nil E; typedef Nil F; typedef Nil G; typedef Nil H; typedef Nil I; typedef Nil J; };
																																												 

template<class T> struct ToPointer      { typedef T*  Type; };
template<>        struct ToPointer<Nil> { typedef Nil Type; };
template<class T> struct TupleToTupleOfPointers
{ typedef Tuple<
	typename ToPointer<typename T::A>::Type, 
	typename ToPointer<typename T::B>::Type,
	typename ToPointer<typename T::C>::Type,
	typename ToPointer<typename T::D>::Type,
	typename ToPointer<typename T::E>::Type,
	typename ToPointer<typename T::F>::Type,
	typename ToPointer<typename T::G>::Type,
	typename ToPointer<typename T::H>::Type,
	typename ToPointer<typename T::I>::Type
	> Type;
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
