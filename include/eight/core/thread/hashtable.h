//------------------------------------------------------------------------------
#pragma once
#include <eight/core/debug.h>
#include <eight/core/types.h>
#include <eight/core/alloc/new.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/thread/atomic.h>
#include <eight/core/math/tmp.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T> struct NativeHash { static u32 Hash(const T&) { return NoHashDefinedForThisType; } typedef u32 type; };

template<class T> struct NativeHash<T*>
{
	static u32 Hash(const T* p) { return (*(u32*)&p)/*>>Pow2Down<sizeof(T)>::value*/; }
	typedef u32 type;
};

struct HashTableBase
{
	struct Next
	{
		union
		{
			struct
			{
			s16 free;//when in free list, holds offsNextMinusOne
			s16 chain;//when in a bucket chain, holds offsNextMinusOne and the above is 0xFFFF
			} b;
			u32 value;
		};
	};
	struct WaitForValid : NonCopyable
	{
		WaitForValid( const Next& a ) : offsNextMinusOne(a) {} const Next& offsNextMinusOne; 
		bool operator()() const;
	};
};

// If ThreadSafe, safely supports concurrent insertions OR concurrent lookups. Issue a fence between switching modes.
//                When performing 'unsafe' concurrent insertions AND lookups, lookups may fetch default-initialised values.
// Key and value should be POD - no constructors or destructors are called.
template<class K, bool ThreadSafe>
class THashSetBase : HashTableBase
{
public:
	THashSetBase( Scope& a, uint capacity );
	THashSetBase( Scope& a, uint capacity, uint numBuckets );
	bool Find( const K& k, int& outIndex );
	bool Insert( const K& k, int& outIndex );
	      K& KeyAt( int i )       { eiASSERT(i>=0 && i<(int)capacity); return keyList[i].key; }
	const K& KeyAt( int i ) const { eiASSERT(i>=0 && i<(int)capacity); return keyList[i].key; }
private:
	void Init(uint capacity);
	int FindKeyInBucketChain( const K& k, int startNode );
	typedef typename NativeHash<K>::type H;
	s32   freeHead;//index, base 0
protected:
	struct Node
	{
		K key;
		Next offsNextMinusOne;
		eiSTATIC_ASSERT(sizeof(Next) == 4);
	};
	u32   const numBuckets;
	u32   const capacity;
	u32*  const bucketHeads;//array of indices, base 1
	Node* const keyList;
private:
	struct SetIfEqual
	{
		SetIfEqual(void* inout, u32 newVal, u32 oldVal) : inout(inout), newVal(newVal), oldVal(oldVal) {}; void* inout; u32 newVal; u32 oldVal;
		bool operator()()
		{
			if( ThreadSafe )
				return ((Atomic*)inout)->SetIfEqual(newVal, oldVal);
			else
			{
				u32* data = (u32*)inout;
				eiASSERT( *data == oldVal );
				*data = newVal;
				return true;
			}
		}
	};
};

template<class K, class V, bool ThreadSafe>
class THashTableBase : public THashSetBase<K,ThreadSafe>
{
public:
	THashTableBase(Scope& a, uint capacity);
	THashTableBase(Scope& a, uint capacity, uint numBuckets);

	u32 Index(const V* value)
	{
		ptrdiff_t index = value - valueList;
		eiASSERT(index >= 0 && index<(ptrdiff_t)capacity);
		return (u32)index;
	}
	V& At(int i) { eiASSERT(i >= 0 && i<(int)capacity); return valueList[i]; }
	const V& At(int i) const { eiASSERT(i >= 0 && i<(int)capacity); return valueList[i]; }
	V& operator[](const K& k)
	{
		int i = -1;
		Insert(k, i);
		return At(i);
	}
	//todo - document (lack of) thread safety -- i.e. don't insert during enumeration.
	template<class Fn> void ForEach(Fn& fn)
	{
		for (u32 bucket = 0, end = numBuckets; bucket != end; ++bucket)
		{
			int bucketHeadPlus1 = bucketHeads[bucket];
			if (bucketHeadPlus1 > 0)
			{
				for (int index = bucketHeadPlus1 - 1, prev = -1, next; index != prev; prev = index, index = next)
				{
					eiASSERT(index < (int)capacity);
					Node& node = keyList[index];
					eiASSERT(node.offsNextMinusOne.b.free == (s16)(u16)0xFFFF);
					fn(node.key, valueList[index]);
					next = index + node.offsNextMinusOne.b.chain + 1;
				}
			}
		}
	}
	template<class Fn> void ForEachBreak(Fn& fn)
	{
		for (u32 bucket = 0, end = numBuckets; bucket != end; ++bucket)
		{
			int bucketHeadPlus1 = bucketHeads[bucket];
			if (bucketHeadPlus1 > 0)
			{
				for (int index = bucketHeadPlus1 - 1, prev = -1, next; index != prev; prev = index, index = next)
				{
					eiASSERT(index < (int)capacity);
					Node& node = keyList[index];
					eiASSERT(node.offsNextMinusOne.b.free == (s16)(u16)0xFFFF);
					if (fn(node.key, valueList[index]))
						return;
					next = index + node.offsNextMinusOne.b.chain + 1;
				}
			}
		}
	}
private:
	V* valueList;
};


template<class K>
class HashSet : public THashSetBase<K, false>
{
public:
	HashSet(Scope& a, uint capacity) : THashSetBase(a, capacity) {}
	HashSet(Scope& a, uint capacity, uint numBuckets) : THashSetBase<K, V, false>(a, capacity, numBuckets) {}
};

template<class K>
class MpmcHashSet : public THashSetBase<K, true>
{
public:
	MpmcHashSet(Scope& a, uint capacity) : THashSetBase(a, capacity) {}
	MpmcHashSet(Scope& a, uint capacity, uint numBuckets) : THashSetBase<K, V, true>(a, capacity, numBuckets) {}
};

template<class K, class V>
class HashTable : public THashTableBase<K,V,false>
{
public:
	HashTable( Scope& a, uint capacity )                  : THashTableBase<K,V,false>( a, capacity ) {}
	HashTable( Scope& a, uint capacity, uint numBuckets ) : THashTableBase<K,V,false>( a, capacity, numBuckets ) {}
};

template<class K, class V>
class MpmcHashTable : public THashTableBase<K,V, true>
{
public:
	MpmcHashTable( Scope& a, uint capacity )                  : THashTableBase<K,V,true>( a, capacity ) {}
	MpmcHashTable( Scope& a, uint capacity, uint numBuckets ) : THashTableBase<K,V,true>( a, capacity, numBuckets ) {}
};

//------------------------------------------------------------------------------
#include "hashtable.hpp"
//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
