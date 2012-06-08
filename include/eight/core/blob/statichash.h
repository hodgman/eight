//------------------------------------------------------------------------------
#pragma once
#include "eight/core/alloc/scope.h"
namespace eight {
//------------------------------------------------------------------------------

//template<class T> struct ReturnType { typedef void type; };
//template<class T> struct ReturnType<T(*)()> { typedef T type; };

template<class K, class V> class StaticHashMap
{
public:
	StaticHashMap( Scope& a, uint maxAssets ) : m_data(a.New<KeyValue>(maxAssets)), m_capacity(maxAssets), m_size() {}

	struct KeyValue
	{
		K key;
		V value;
	};
	V* Find( const K& )
	{
		return 0;
	}
	void Add( const K&, const V& )
	{
	}
private:
	typedef typename NativeHash<K>::type H;
	KeyValue* m_data;
	uint m_capacity;
	uint m_size;
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------