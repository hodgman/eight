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
			};
			s32 value;
		};
	};
	struct WaitForValid
	{
		WaitForValid( const Next& a ) : offsNextMinusOne(a) {} const Next& offsNextMinusOne; 
		bool operator()() const;
	};
};

// If ThreadSafe, safely supports concurrent insertions OR concurrent lookups. Issue a fence between switching modes.
//                When performing 'unsafe' concurrent insertions AND lookups, lookups may fetch default-initialised values.
// Key and value should be POD - no constructors or destructors are called.
template<class K, class V, bool ThreadSafe>
class THashTableBase : HashTableBase
{
public:
	THashTableBase( Scope& a, uint capacity );
	THashTableBase( Scope& a, uint capacity, uint numBuckets );
	bool Find( const K& k, int& outIndex );
	bool Insert( const K& k, int& outIndex );
	V& At( int i ) { eiASSERT(i>=0 && i<(int)capacity); return valueList[i]; }
	V& operator[]( const K& k )
	{
		int i=-1;
		Insert( k, i );
		return At(i);
	}
	//todo - document (lack of) thread safety? dont insert during enumeration.
	template<class Fn> void ForEach( Fn& fn )
	{
		for( u32 bucket=0, end=numBuckets; bucket!=end; ++bucket )
		{
			int bucketHeadPlus1 = bucketHeads[bucket];
			if( bucketHeadPlus1 > 0 )
			{
				for( int index = bucketHeadPlus1-1, prev = -1, next; index != prev; prev = index, index = next )
				{
					eiASSERT( index < (int)capacity );
					Node& node = keyList[index];
					eiASSERT( node.offsNextMinusOne.free == (s16)0xFFFF );
					fn( node.key, valueList[index] );
					next = index+node.offsNextMinusOne.chain+1;
				}
			}
		}
	}
	template<class Fn> void ForEachBreak( Fn& fn )
	{
		for( u32 bucket=0, end=numBuckets; bucket!=end; ++bucket )
		{
			int bucketHeadPlus1 = bucketHeads[bucket];
			if( bucketHeadPlus1 > 0 )
			{
				for( int index = bucketHeadPlus1-1, prev = -1, next; index != prev; prev = index, index = next )
				{
					eiASSERT( index < (int)capacity );
					Node& node = keyList[index];
					eiASSERT( node.offsNextMinusOne.free == (s16)0xFFFF );
					if( fn( node.key, valueList[index] ) )
						return;
					next = index+node.offsNextMinusOne.chain+1;
				}
			}
		}
	}
private:
	void Init(uint capacity);
	int FindKeyInBucketChain( const K& k, int startNode );
	typedef typename NativeHash<K>::type H;
	struct Node
	{
		K key;
		Next offsNextMinusOne;
		eiSTATIC_ASSERT( sizeof(Next) == 4 );
	};
	s32 freeHead;//index, base 0
	u32 numBuckets;
	u32 capacity;
	u32* bucketHeads;//array of indices, base 1
	Node* keyList;
	V* valueList;

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

template<class K, class V>
class HashTable : public THashTableBase<K,V,false>
{
public:
	HashTable( Scope& a, uint capacity )                  : THashTableBase( a, capacity ) {}
	HashTable( Scope& a, uint capacity, uint numBuckets ) : THashTableBase( a, capacity, numBuckets ) {}
};

template<class K, class V>
class MpmcHashTable : public THashTableBase<K,V, true>
{
public:
	MpmcHashTable( Scope& a, uint capacity )                  : THashTableBase( a, capacity ) {}
	MpmcHashTable( Scope& a, uint capacity, uint numBuckets ) : THashTableBase( a, capacity, numBuckets ) {}
};

//------------------------------------------------------------------------------
#include "hashtable.hpp"
//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
