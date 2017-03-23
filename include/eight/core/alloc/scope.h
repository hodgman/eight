//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/stack.h>
#include <eight/core/debug.h>
#include <eight/core/macro.h>
#include <eight/core/traits.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T>
struct DerefHelper
{
	DerefHelper(T* p) : pointer(p) {}
	T* pointer;
	operator const T&() const { return *pointer; }
	operator const T*() const { return pointer; }
	operator       T&()       { return *pointer; }
	operator       T*()       { return pointer; }
	const T* operator ->() const { return pointer; }
	      T* operator ->()       { return pointer; }
	const T& operator *() const { return *pointer; }
	      T& operator *()       { return *pointer; }
};

class Scope : NonCopyable
{
public:
	Scope( StackAlloc&, const char* name );
	Scope( Scope&, const char* name );
	~Scope();
	void Unwind();
	void Seal();
	bool Sealed() const;
	//See new.h for keywords (macros) to 
	template<class T> void* New();           //no c, no d (c&d ARE performed by eiNew though!)
	template<class T> T* New( uint count );                 //   c,    d
	template<class T> T* New( uint count, const T& init );  //   c,    d
	template<class T> T* Alloc();                           //no c, no d
	template<class T> T* Alloc( uint count );               //no c, no d
	                 u8* Alloc( size_t size, uint align=0 );
	typedef void (FnDestructor)(void*);
	void OnUnwind( void*, FnDestructor* );

	StackAlloc& InternalAlloc() { return *alloc; }

	template<class T> static void DestructorCallback(void* p);

	const struct DestructorHelper
	{//used by eiNew to add the object to the destructor chain *after* it's constructor has executed
		template<class T>
		DerefHelper<T> operator<<( T* newObject ) const
		{
			if( !IsSameType<T,Scope>::value )
				Owner().OnUnwind(newObject, &DestructorCallback<T>, sizeof(T));
			return newObject;
		}
	private:
		Scope& Owner() const;
	} OnConstructed;
	
	void Delete(void* ptr)
	{
		if( !ptr )
			return;
		for( Destructor* list = destructList, *prev = 0; list; prev = list, list = list->next )
		{
			if( list->object == ptr )
			{
				list->fn(ptr);
				Free(ptr);
				//remove node from list
				if( prev )
					prev->next = list->next;
				else//removing first item in list
				{
					eiASSERT( list == destructList );
					destructList = list->next;

					//if it's on the top of our allocator, is safe to unwind it
					if( list->size && (((u8*)ptr) + list->size) == alloc->Mark() )
					{
						alloc->Unwind(ptr);
					}
				}
				return;
			}
		}
		eiASSERT( false );
	}
	inline void Free(void* ptr)
	{
#if eiDEBUG_LEVEL > 0
		if( !ptr )
			return;
		for( OverflowNode* node = overflowChain; node; node = node->next )
		{
			const u8* begin = node->alloc.Begin();
			const u8* end = begin + node->alloc.Capacity();
			if( ptr >= begin && ptr < end )
				return;
		}
		eiASSERT( ptr >= originalAlloc.Begin() && ptr < originalAlloc.Begin()+originalAlloc.Capacity() );
#endif
	}
private:
	Scope( Scope& );
	void OnOutOfMemory( size_t required );
	struct ArrayDestructor
	{
		uint count;
		void* p;
		template<class T> static void Callback(void* p);
	};
	template<class T> T* AllocObject();
	template<class T> T* AllocObject( uint count );
	u8* AllocObject(             uint size, FnDestructor* );
	u8* AllocObject( uint count, uint size, FnDestructor* );
	
	StackAlloc& originalAlloc;
	StackAlloc* alloc;
	const void* parent;
	const u8* base;
	struct Destructor
	{
		FnDestructor *fn;
		Destructor* next;
		void* object;
		u32 size;
	}* destructList;
	const char* name;

	struct OverflowNode
	{
		OverflowNode(OverflowNode* next, void* begin, size_t size) : next(next), heap(begin), alloc(begin,size) {}
		OverflowNode* next;
		void* heap;
		StackAlloc alloc;
	};
	OverflowNode* overflowChain;
	size_t capacity;
	void FreeOverflowChain();
	
	template<class T> 
	friend void* InterfaceAlloc(Scope&);
	void OnUnwind( void*, FnDestructor*, u32 sizeInBytes );
	void AddDestructor( Destructor*, u8*, FnDestructor*, u32 sizeInBytes );
};

template<uint SIZE> struct LocalScope
{
	u8 buffer[SIZE];
	StackAlloc stack;
	Scope scope;
	LocalScope(const char* name=0)
		: stack(buffer, SIZE)
		, scope(stack, name?name:"local")
	{}
	operator Scope&() { return scope; }
};

//------------------------------------------------------------------------------
#include "scope.hpp"
} // namespace eight
//------------------------------------------------------------------------------
