#pragma once
#include <new>
namespace eight {
//------------------------------------------------------------------------------
/*
	Engine-specific alternatives to new and malloc are provided.

	Objects created via eiNew/eiNewArray will have their constructors called on
	 creation, and have their destructors called when the allocated memory goes
	 out of scope.

	Objects created via eiAlloc/eiAllocArray will not have constructors or
	 destructors called.

	The eiConstructAs macro can be used to change 
 */

//#define eiNew( alloc, type, ... ) new((alloc).New<type>()) type (__VA_ARGS__)

#define eiNew(        alloc, type    )     new((alloc).New  <type>( )) type
#define eiAlloc(      alloc, type    ) ((type*)(alloc).Alloc<type>( ))
#define eiNewArray(   alloc, type, n ) ((type*)(alloc).New  <type>(n))
#define eiAllocArray( alloc, type, n ) ((type*)(alloc).Alloc<type>(n))
#define eiNewInterface( alloc, type )  new(eight::InterfaceAlloc<type>(alloc)) type

template<class T> struct HasDestructor    { static const bool value = true; };
#define eiNoDestructor(T)																\
	template<       > struct HasDestructor<T> { static const bool value = false; };	//
eiNoDestructor(int);
eiNoDestructor(unsigned int);
//TODO - etc

//------------------------------------------------------------------------------
}//namespace
//------------------------------------------------------------------------------
