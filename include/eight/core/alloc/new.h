#pragma once
#include <new>
#include "eight/core/types.h"
#include "eight/core/debug.h"
namespace eight {

struct TagInterface;
template<class T> class Interface;

template<class T, bool IsInterface> struct InterfaceCast
{
	typedef Interface<T> Type;
};
template<class T> struct InterfaceCast<T,false>
{
	typedef T Type;
};
template<class T> typename InterfaceCast<T, std::is_base_of<TagInterface,T>::value>::Type* InterfacePointer( T* ptr )
{
	return (typename InterfaceCast<T, std::is_base_of<TagInterface,T>::value>::Type*)(ptr);
}
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

	

#define eiNew(          alloc, type ) (alloc).OnConstructed<<new((alloc).New  <type>( )) type
#define eiAlloc(        alloc, type    )    ((type*)(alloc).Alloc<type>( ))
#define eiNewArray(     alloc, type, n )    ((type*)(alloc).New  <type>(n))
#define eiAllocArray(   alloc, type, n )    ((type*)(alloc).Alloc<type>(n))
#define eiNewInterface( alloc, type )  new(eight::InterfaceAlloc<type>(alloc)) type

#define eiDelete(      alloc, a )       (alloc).Delete(InterfacePointer(a))
#define eiDeleteArray( alloc, a, size ) (alloc).DeleteArray(a, size)
#define eiFree(        alloc, a )       (alloc).Free(a)

struct ei_GlobalHeap
{
	template<class T> static T* New();                          //no c,    d
	template<class T> static T* New( uint count );                 //   c,    d
	template<class T> static T* Alloc();                           //no c, no d
	template<class T> static T* Alloc( uint count );               //no c, no d
	                  static u8* Alloc( uint bytes );            
	template<class T> static void Delete(Interface<T>*);
	template<class T> static void DeleteArray(T*, uint size);
	template<class T> static void Delete(T*);
	static void Free(void*);
	
	const static struct PassThru
	{
		template<class T> T* operator<<( T* newObject ) const { return newObject; }
	} OnConstructed;
};

template<class T> struct HasDestructor    { static const bool value = true; };
#define eiNoDestructor(T)																\
	template<       > struct HasDestructor<T> { static const bool value = false; };	//
eiNoDestructor(int);
eiNoDestructor(unsigned int);
//TODO - etc



//TODO - move
const static ei_GlobalHeap GlobalHeap;
void* Malloc( size_t size );
void  Free( void* );
template<class T> T* ei_GlobalHeap::New()                          //no c,    d
{
	return (T*)Malloc(sizeof(T));
}
template<class T> T* ei_GlobalHeap::New( uint count )                 //   c,    d
{
	T* data = (T*)Malloc(sizeof(T)*count);
	for( uint i=0; i != count; ++i )
	{
		new(&data[i]) T();
	}
	return data;
}
template<class T> T* ei_GlobalHeap::Alloc()                           //no c, no d
{
	return (T*)Malloc(sizeof(T));
}
template<class T> T* ei_GlobalHeap::Alloc( uint count )               //no c, no d
{
	return (T*)Malloc(sizeof(T)*count);
}
inline u8* ei_GlobalHeap::Alloc( uint bytes )
{
	return (u8*)Malloc(bytes);
}           
template<class T> void ei_GlobalHeap::Delete(Interface<T>* ptr)
{
	if( !ptr )
		return;
	Interface<T>::Instance_Destruct(ptr);
	eight::Free(ptr);
}
template<class T> void ei_GlobalHeap::DeleteArray(T* ptr, uint size)
{
	eiSTATIC_ASSERT(!(std::is_base_of<TagInterface,T>::value));
	if( !ptr )
		return;
	for( uint i=size; i-->0; )
		ptr[i].~T();
	eight::Free(ptr);
}
template<class T> void ei_GlobalHeap::Delete(T* ptr)
{
	eiSTATIC_ASSERT(!(std::is_base_of<TagInterface,T>::value));
	if( !ptr )
		return;
	ptr->~T();
	eight::Free(ptr);
}
inline void ei_GlobalHeap::Free(void* ptr)
{
	eight::Free(ptr);
}

//------------------------------------------------------------------------------
}//namespace
//------------------------------------------------------------------------------
