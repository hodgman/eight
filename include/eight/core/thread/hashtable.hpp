
inline bool HashTableBase::WaitForValid::operator()() const
{
	if( offsNextMinusOne.free == (s16)0xFFFF )//is in bucket chain, not free list
		return true;
	Next t; t.value = ((Atomic&)offsNextMinusOne.value);
	if( t.free == (s16)0xFFFF )
		return true;
	return false;
}

template<class K, class V, bool T>
THashTableBase<K,V,T>::THashTableBase( Scope& a, uint capacity, uint numBuckets )
	: freeHead(0)
	, numBuckets(numBuckets)
	, capacity(capacity)
	, bucketHeads(eiAllocArray(a, u32, numBuckets))
	, keyList(eiAllocArray(a, Node, capacity))
	, valueList(eiAllocArray(a, V, capacity))
{
	Init(capacity);
}

template<class K, class V, bool T>
THashTableBase<K,V,T>::THashTableBase( Scope& a, uint capacity )
	: freeHead(0)
	, numBuckets((capacity * 15 + 5) / 10)
	, capacity(capacity)
	, bucketHeads(eiAllocArray(a, u32, numBuckets))
	, keyList(eiAllocArray(a, Node, capacity))
	, valueList(eiAllocArray(a, V, capacity))
{
	Init(capacity);
}

template<class K, class V, bool T>
void THashTableBase<K,V,T>::Init(uint capacity)
{
	eiASSERT( numBuckets );
	memset(bucketHeads, 0, sizeof(u32)*numBuckets);
	memset(keyList,     0, sizeof(Node)*capacity);
	Next terminal = { {-1, 0} };//terminate the linked list
	keyList[numBuckets-1].offsNextMinusOne = terminal;
}

template<class K, class V, bool T>
int THashTableBase<K,V,T>::FindKeyInBucketChain( const K& k, int startNode )
{
	Node* node;
	eiASSERT( startNode >= 0 && startNode < (int)capacity );
	for( int index = startNode, next; index >= 0; index = next )
	{
		eiASSERT( index < (int)capacity );
		node = &keyList[index];
		eiASSERT( node->offsNextMinusOne.free == (s16)0xFFFF );
		if( T )
			YieldThreadUntil( WaitForValid(node->offsNextMinusOne) );
		if( node->key == k )
			return index;
		next = index+node->offsNextMinusOne.chain+1;
		if( next == index )
			break;
	}
	return -1;
}

template<class K, class V, bool T>
bool THashTableBase<K,V,T>::Insert( const K& k, int& index )
{
	H hash = NativeHash<K>::Hash(k);
	uint bucket = hash % numBuckets;
	const u32 mskLock = 0x80000000;
	u32 prevBucketHead;// = ~mskLock & bucketHeads[bucket];
	
	BusyWait spinner;
	do//lock a bucket
	{
		prevBucketHead = ~mskLock & bucketHeads[bucket];
		if( prevBucketHead )//if there's already a bucket chain, search it for duplicates
		{
			eiASSERT( ((int)prevBucketHead)-1 >= 0 );
			int idx = FindKeyInBucketChain( k, prevBucketHead-1 );
			if( idx >= 0 )//it's already in the bucket
			{
				index = idx;
				return false;
			}
		}
	} while( !spinner.Try(SetIfEqual(&bucketHeads[bucket], prevBucketHead|mskLock, prevBucketHead)) );
//	} while( !((Atomic&)bucketHeads[bucket]).SetIfEqual(prevBucketHead|mskLock, prevBucketHead) );//todo - use a BusyWait spinner
	
	eiASSERT( (!prevBucketHead || FindKeyInBucketChain( k, prevBucketHead-1 ) < 0) && "shouldn't have locked if it was in the bucket..." );
	
	s32 allocation;//allocate a node from the freelist
	{
		Next offsNextMinusOne;
		do
		{
			allocation = freeHead;
			if( allocation == -1 )
			{
				eiWarn( "Hash table filled up" );
				eiASSERT( false );
				index = -1;
				return false;
			}
			offsNextMinusOne = keyList[allocation].offsNextMinusOne;
	//	} while( !((Atomic&)freeHead).SetIfEqual(allocation+offsNextMinusOne.free+1, allocation) );
		} while( !spinner.Try(SetIfEqual(&freeHead, allocation+offsNextMinusOne.free+1, allocation)) );
		eiASSERT( !offsNextMinusOne.chain );
	}

	keyList[allocation].key = k;
	//valueList[allocation]     = v;
	//after writing the key/value, mark the node as valid and write the offset to the next node in the bucket chain
	u32 newNext = prevBucketHead ? prevBucketHead-1 //bucket heads are indexed from 1..n, get array index
	                             : allocation;//offset to self is used as terminator
	Next next = { (s16)(0xFFFFU),
	              (s16)(newNext-allocation-1), //offsNextMinusOne
	            };
	((Atomic&)keyList[allocation].offsNextMinusOne.value) = next.value;
	bucketHeads[bucket] = allocation+1;
	index = allocation;
	return true;
}
