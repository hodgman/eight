//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/stack.h>
#include <eight/core/debug.h>
#include <eight/core/macro.h>
namespace eight {
//------------------------------------------------------------------------------

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
	                 u8* Alloc( uint size, uint align=0 );
	typedef void (FnDestructor)(void*);
	void OnUnwind( void*, FnDestructor* );

	StackAlloc& InternalAlloc() { return *alloc; }

	template<class T> static void DestructorCallback(void* p);

	const struct DestructorHelper
	{//used by eiNew to add the object to the destructor chain *after* it's constructor has executed
		template<class T>
		T* operator<<( T* newObject ) const { Owner().OnUnwind(newObject, &DestructorCallback<T>); return newObject; }
	private:
		Scope& Owner() const;
	} OnConstructed;
private:
	Scope( Scope& );
	void OnOutOfMemory( uint required );
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
	}* destructList;
	const char* name;

	struct OverflowNode
	{
		OverflowNode(OverflowNode* next, void* begin, uint size) : next(next), heap(begin), alloc(begin,size) {}
		OverflowNode* next;
		void* heap;
		StackAlloc alloc;
	};
	OverflowNode* overflowChain;
	u32 capacity;
	void FreeOverflowChain();
	
	void AddDestructors( Destructor*, u8*, uint count, uint size, FnDestructor* );
	void AddDestructor( Destructor*, u8*, FnDestructor* );
};

template<uint SIZE> struct LocalScope
{
	u8 buffer[SIZE];
	StackAlloc stack;
	Scope scope;
	LocalScope()
		: stack(buffer, SIZE)
		, scope(stack, "local")
	{}
	operator Scope&() { return scope; }
};

//------------------------------------------------------------------------------
#include "scope.hpp"
} // namespace eight
//------------------------------------------------------------------------------
