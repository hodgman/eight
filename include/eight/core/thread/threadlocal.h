//------------------------------------------------------------------------------
#pragma once
#include "eight/core/debug.h"
#include "eight/core/types.h"
#include "eight/core/noncopyable.h"
#include "eight/core/thread/atomic.h"
namespace eight {
//------------------------------------------------------------------------------

#define eiBUILD_NO_STATIC_TLS
#if defined(eiBUILD_MSVC)
# define eiDYNAMIC_TLS_TYPE( T ) u32
# define eiSTATIC_TLS_TYPE( T ) __declspec(thread) T
#elif defined(eiBUILD_GCC)
# error "GCC port not complete"
# define eiSTATIC_TLS_TYPE( T ) __thread T
#else
# error "TLS not ported to this compiler"
#endif

#if defined(eiBUILD_NO_STATIC_TLS)
# undef eiSTATIC_TLS_TYPE
# define eiSTATIC_TLS_TYPE( T ) ThreadLocal<T>
#endif

u32   TlsAlloc();
void  TlsFree( u32 index );
void* TlsGet( u32 index );
void  TlsSet( u32 index, void* value );

template<class T>
class ThreadLocal : NonCopyable
{
public:
	 ThreadLocal() : index(eight::TlsAlloc()) {}
	~ThreadLocal() { eight::TlsFree( index ); }
	operator       T*()            { return  (T*)eight::TlsGet( index ); }
	operator const T*() const      { return  (T*)eight::TlsGet( index ); }
	T& operator*()                 { return *(T*)eight::TlsGet( index ); }
	T* operator->()                { return  (T*)eight::TlsGet( index ); }
	ThreadLocal& operator=(       T* p ) { return eight::TlsSet( index, p ), *this; }
	ThreadLocal& operator=( const T* p ) { return eight::TlsSet( index, p ), *this; }
private:
	u32 index;
};

template<class T, class Tag>
class ThreadLocalStatic
{
public:
	ThreadLocalStatic& operator=(       T* t ) { return (data = t), *this; }
	ThreadLocalStatic& operator=( const T* t ) { return (data = t), *this; }
	operator const T*() const { return data; }
	operator       T*()       { return data; }
	T* operator->()           { return data; }
	T& operator*()            { return *data; }
private:
	static eiSTATIC_TLS_TYPE(T) data;
};
template<class T, class Tag> eiSTATIC_TLS_TYPE(T) ThreadLocalStatic<T,Tag>::data;

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------