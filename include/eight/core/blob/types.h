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
	static uint DataSize() { return sizeof(T); }
	bool operator!() const { return !offset; }
	Offset& operator=( void* ptr ) { offset = ptr ? ((u8*)ptr - (u8*)&offset) : 0; return *this; }
//private:
	Y offset;
};

template<class T> struct ArrayOffset
{
	const T* Ptr() const { return (T*)(((u8*)&offset) + offset); }
		  T* Ptr()       { return (T*)(((u8*)&offset) + offset); }
	const T& operator *() const { return *Ptr(); }
		  T& operator *()       { return *Ptr(); }
	const T& operator[](uint i) const { return Ptr()[i]; }
		  T& operator[](uint i)       { return Ptr()[i]; }
	uint DataSize(uint count) const { return sizeof(T) * count; }
	ArrayOffset& operator=(T* p) { offset = ((u8*)p)-((u8*)this); return *this; }
//private:
	s32 offset;
};

template<class T> struct Array
{
	const T* Begin() const { return (T*)(&data); }
		  T* Begin()       { return (T*)(&data); }
	const T* End  (uint count) const { return Begin()+count; }
	      T* End  (uint count)       { return Begin()+count; }
	const T& operator[](uint i) const { return ((T*)&data)[i]; }
		  T& operator[](uint i)       { return ((T*)&data)[i]; }
	static uint DataSize(uint count)  { return sizeof(T) * count; }
//private:
	u8 data;
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
	uint Bytes()    const { return sizeof(List) + DataSize(); }
//private:
	u32 count;
};

template<class T> struct ListOffset
{
	const T* Begin() const { return list->Begin(); }
		  T* Begin()       { return list->Begin(); }
	const T* End  () const { return list->End(); }
	      T* End  ()       { return list->End(); }
	const T& operator[](uint i) const { return (*list)[i]; }
		  T& operator[](uint i)       { return (*list)[i]; }
	uint DataSize() const { return list->DataSize(); }
	uint Bytes()    const { return list->Bytes(); }
	
	const List<T>* Ptr() const { return list.Ptr(); }
		  List<T>* Ptr()       { return list.Ptr(); }
	const List<T>* operator->() const { return list.Ptr(); }
		  List<T>* operator->()       { return list.Ptr(); }
	const List<T>& operator *() const { return *list; }
		  List<T>& operator *()       { return *list; }
	bool operator!() const { return !list; }
//private:
	Offset<List<T> > list;
};

struct String { u8 length; char chars; };

struct StringOffset
{
	bool operator==( const StringOffset& other )
	{
		const String& a = *this->Ptr();
		const String& b = *other.Ptr();
		return unlikely(&a==&b) || ( unlikely((hash)==(other.hash))
									 && likely(a.length==b.length)
									 && likely(0==memcmp(&a.chars, &b.chars, a.length)) );
	}
	const String* Ptr() const { return (String*)(((u8*)&hash) + (offset)); }
		  String* Ptr()       { return (String*)(((u8*)&hash) + (offset)); }
	const String* operator->() const { return Ptr(); }
		  String* operator->()       { return Ptr(); }
	const String& operator *() const { return *Ptr(); }
		  String& operator *()       { return *Ptr(); }
	bool HashEquals( u32 hash ) const { return hash == this->hash; }
//private:
	u32 hash;
	s32 offset;
};

struct CompareStringHash { inline bool operator()( const StringOffset& lhs, const StringOffset& rhs )
{
	return lhs.hash < rhs.hash;
}};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------