//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/debug.h>
#include "string.h"//memcmp
namespace eight {
//------------------------------------------------------------------------------

#define unlikely(x) x//TODO - move
#define likely(x) x

struct VariableSize
{
	static uint Bytes();
};
	
struct BlobRoot : public VariableSize
{
	uint Bytes() const { return bytes; }
	u32 bytes;
};

template<class T, class Y=s32> struct Offset
{
	const T* Ptr() const { return (T*)(((u8*)&offset) + offset); }
		  T* Ptr()       { return (T*)(((u8*)&offset) + offset); }
	const T* operator->() const { return Ptr(); }
		  T* operator->()       { return Ptr(); }
	const T& operator *() const { return *Ptr(); }
		  T& operator *()       { return *Ptr(); }
	uint DataSize() const { return sizeof(T); }
	bool operator!() const { return !offset; }
	Offset& operator=( void* ptr ) { offset = ptr ? ((u8*)ptr - (u8*)&offset) : 0; return *this; }
//private:
	Y offset;
};

template<class T> struct Array
{
	const T* Ptr() const { return (T*)(((u8*)&offset) + offset); }
		  T* Ptr()       { return (T*)(((u8*)&offset) + offset); }
	const T& operator *() const { return *Ptr(); }
		  T& operator *()       { return *Ptr(); }
	const T& operator[](uint i) const { return Ptr()[i]; }
		  T& operator[](uint i)       { return Ptr()[i]; }
	uint DataSize(uint count) const { return sizeof(T) * count; }
	Array& operator=(T* p) { offset = ((u8*)p)-((u8*)this); return *this; }
//private:
	s32 offset;
};

template<class T> struct Address
{
	const T* Ptr(const void* base) const { return (T*)(((u8*)base) + address); }
		  T* Ptr(const void* base)       { return (T*)(((u8*)base) + address); }
	uint DataSize() const { return sizeof(T); }
//private:
	u32 address;
};

template<class T> struct List
{
	const T* Begin() const { return (T*)(&count + 1); }
		  T* Begin()       { return (T*)(&count + 1); }
	const T* End  () const { return Begin()+count; }
	      T* End  ()       { return Begin()+count; }
	const T& operator[](uint i) const { eiASSERT(i < count); return Begin()[i]; }
		  T& operator[](uint i)       { eiASSERT(i < count); return Begin()[i]; }
	uint DataSize() const { return sizeof(T) * count; }
	uint Bytes() const { return sizeof(List) + DataSize(); }
//private:
	u32 count;
};

struct String { u8 length; u8 chars; };

struct StringOffset
{
	bool operator==( const StringOffset& other )
	{
		const String& a = *this->Ptr();
		const String& b = *other.Ptr();
		return unlikely(&a==&b) || ( unlikely((offset&~0xFFFFU)==(other.offset&~0xFFFFU))
									 && likely(a.length==b.length)
									 && likely(0==memcmp(&a.chars, &b.chars, a.length)) );
	}
	const String* Ptr() const { return (String*)(((u8*)&offset) + (offset&0xFFFFU)); }
		  String* Ptr()       { return (String*)(((u8*)&offset) + (offset&0xFFFFU)); }
	const String* operator->() const { return Ptr(); }
		  String* operator->()       { return Ptr(); }
	const String& operator *() const { return *Ptr(); }
		  String& operator *()       { return *Ptr(); }
//private:
	s32 offset;
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------