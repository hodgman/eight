//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/debug.h>
#include <eight/core/noncopyable.h>
#include "string.h"//for memset
namespace eight {
//------------------------------------------------------------------------------

class StackAlloc : public NonCopyable
{
public:
	//StackAlloc();
	StackAlloc( void* begin, void* end );
	StackAlloc( void* begin, size_t size );
	//bool Valid() const;
	void Clear();
	//bool IsClear() const { return cursor == begin; }
	bool Align( uint );
	inline u8* Alloc( size_t required, size_t reserve );
	inline           u8* Alloc( size_t size );
	template<class T> T* Alloc(           );
	template<class T> T* Alloc( u32 count );
	void Unwind(const void* p);
	const u8* Mark() const;
	const u8* Begin() const;
	size_t Capacity() const { return (end-begin); }

	void TransferOwnership(const void* from, const void* to) { eiASSERT(from==owner); owner=to; }
	const void* Owner() const { return owner; }
private:
	u8* begin;
	u8* end;
	u8* cursor;
	const void* owner;
};

class StackMeasure : public NonCopyable
{
public:
	StackMeasure() : cursor() {}
	                  void Clear();
	                  void Align( uint );//assumes the base address will also be aligned to this value
	inline            size_t Alloc( size_t size );
	template<class T> size_t Alloc(           );
	template<class T> size_t Alloc( u32 count );
	                  size_t Bytes() const { return cursor; }
private:
	size_t cursor;
};

template<uint SIZE> struct LocalStack
{
	u8 buffer[SIZE];
	StackAlloc stack;
	LocalStack(const char* name=0)
		: stack(buffer, SIZE)
	{}
	operator StackAlloc&() { return stack; }
};

//------------------------------------------------------------------------------
#include "stack.hpp"
} // namespace eight
//------------------------------------------------------------------------------
