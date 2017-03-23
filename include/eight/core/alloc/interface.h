#pragma once
#include <eight/core/alloc/new.h>
#include <eight/core/noncopyable.h>
#include <eight/core/debug.h>
namespace eight {
//------------------------------------------------------------------------------

struct TagInterface {};
template<class T>
class Interface : NonCopyable, public TagInterface
{
public:
	static unsigned int Instance_Size();
	static void Instance_Destruct(void*);
private:
	friend T;
	~Interface() { eiASSERT(false);  }
};

template<class T>
class Implementation : NonCopyable
{
public:
	operator T&()       { return *(T*)this; }
	operator T&() const { return *(T*)this; }
};

#define eiImplementInterface( interface, implementation )													\
	uint Interface<interface>::Instance_Size()            { return sizeof(implementation); }				\
	void Interface<interface>::Instance_Destruct(void* p) { eiASSERT(p);									\
	                                                        ((implementation*)p)->~implementation(); }		\
	template<> struct eight::InterfaceType<interface> { typedef implementation Type; };						//

#define eiInterfaceConstructor( interface, call, ... )																\
	interface::interface(__VA_ARGS__) { new((void*)this) eight::InterfaceType<interface>::Type call; }				//

#define eiInterfaceFunction(      ret, interface, name, call, ... )													\
	ret interface::name(__VA_ARGS__)       { return ((eight::InterfaceType<interface>::Type*)this)->name call; }	//
#define eiInterfaceFunctionConst( ret, interface, name, call, ... )													\
	ret interface::name(__VA_ARGS__) const { return ((eight::InterfaceType<interface>::Type*)this)->name call; }	//

#define eiImplementNullInterface( interface )																\
	uint Interface<interface>::Instance_Size()            { eiASSERT(false); return 0; }					\
	void Interface<interface>::Instance_Destruct(void* p) { eiASSERT(p);               }					\
	template<> struct eight::InterfaceType<interface> { typedef void Type; }		;						//

//internals:
class Scope;
template<class T> void* InterfaceAlloc(Scope& a)
{
	u8* mem = a.Alloc( Interface<T>::Instance_Size() );
	a.OnUnwind( mem, &Interface<T>::Instance_Destruct, sizeof(Interface<T>::Instance_Size()) );//todo - shouldn't add destructor until after constructor runs...
	return mem;
}
template<class T> void* InterfaceAlloc(const ei_GlobalHeap& a)
{
	u8* mem = a.Alloc( Interface<T>::Instance_Size() );
	return mem;
}
template<class T> struct InterfaceType {};


template<class T> typename InterfaceType<T>::Type& Cast( T& o ) { return (InterfaceType<T>::Type&)o; }

//------------------------------------------------------------------------------
}//namespace
//------------------------------------------------------------------------------
