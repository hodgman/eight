//------------------------------------------------------------------------------
#pragma once
#include <new>

/*
	Engine-specific alternatives to new and malloc are provided.

	Objects created via eiNew/eiNewArray will have their constructors called on
	 creation, and have their destructors called when the allocated memory goes
	 out of scope.

	Objects created via eiAlloc/eiAllocArray will not have constructors or
	 destructors called.

	The eiConstructAs macro can be used to change 
 */

#define eiConstructAs( Base, Derived )								\
	class Base;														\
	class Derived;													\
	template<> struct ConstructAs<Base> { typedef Derived Type; };	//

template<class T> struct ConstructAs { typedef T Type; };

//#define eiNew( alloc, type, ... ) new((alloc).New<type>()) type (__VA_ARGS__)

#define eiNew(        alloc, type    )     new((alloc).New  <ConstructAs<type>::Type>( )) ConstructAs<type>::Type
#define eiAlloc(      alloc, type    ) ((type*)(alloc).Alloc<ConstructAs<type>::Type>( ))
#define eiNewArray(   alloc, type, n ) ((type*)(alloc).New  <ConstructAs<type>::Type>(n))
#define eiAllocArray( alloc, type, n ) ((type*)(alloc).Alloc<ConstructAs<type>::Type>(n))

namespace eight {
	template<class T> struct HasDestructor    { static const bool value = true; };
}
#define eiNoDestructor(T)																\
	template<       > struct HasDestructor<T> { static const bool value = false; };	//

namespace eight {
eiNoDestructor(int);
eiNoDestructor(unsigned int);
//TODO - etc
}

//------------------------------------------------------------------------------
