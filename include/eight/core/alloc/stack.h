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
	StackAlloc( void* begin, uint size );
	//bool Valid() const;
	void Clear();
	//bool IsClear() const { return cursor == begin; }
	bool Align( uint );
	inline u8* Alloc( u32 required, u32 reserve );
	inline           u8* Alloc( u32 size  );
	template<class T> T* Alloc(           );
	template<class T> T* Alloc( u32 count );
	void Unwind(const void* p);
	const u8* Mark() const;
	const u8* Begin() const;
	u32 Capacity() const { return end-begin; }

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
	inline            u32 Alloc( u32 size  );
	template<class T> u32 Alloc(           );
	template<class T> u32 Alloc( u32 count );
	                  u32 Bytes() const { return cursor; }
private:
	u32 cursor;
};

//------------------------------------------------------------------------------
#include "stack.hpp"
} // namespace eight
//------------------------------------------------------------------------------
