#pragma once
#include <eight/core/alloc/new.h>
#include <eight/core/noncopyable.h>
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
	~Interface();
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

#define eiInterfaceConstructor( interface, call, ... )														\
	interface::interface(__VA_ARGS__) { new(this) eight::InterfaceType<interface>::Type call; }				//

#define eiInterfaceFunction(      ret, interface, name, call, ... )													\
	ret interface::name(__VA_ARGS__)       { return ((eight::InterfaceType<interface>::Type*)this)->name call; }	//
#define eiInterfaceFunctionConst( ret, interface, name, call, ... )													\
	ret interface::name(__VA_ARGS__) const { return ((eight::InterfaceType<interface>::Type*)this)->name call; }	//

//internals:
class Scope;
template<class T> void* InterfaceAlloc(Scope& a)
{
	u8* mem = a.Alloc( Interface<T>::Instance_Size() );
	a.OnUnwind( mem, &Interface<T>::Instance_Destruct );//todo - shouldn't add destructor until after constructor runs...
	return mem;
}
template<class T> struct InterfaceType {};


template<class T> typename InterfaceType<T>::Type& Cast( T& o ) { return (InterfaceType<T>::Type&)o; }

//------------------------------------------------------------------------------
}//namespace
//------------------------------------------------------------------------------
