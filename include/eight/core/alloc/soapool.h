//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/pool.h>
#include <eight/core/handlemap.h>
#include <eight/core/soa.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T>
class CompactSoaPool : NonCopyable
{
public:
	typedef typename Soa<T>::Proxy Proxy;
	CompactSoaPool(Scope& a, uint size);
	Proxy operator[](uint handle);
	void Set( uint handle, const T& tuple );
	void Get( uint handle, T& tuple ) const;
	int Alloc(const T& tuple);
	int Alloc();
	void Release( int handle );
	void Clear();
	uint Capacity() const { eiASSERT(m_pool.Capacity()==m_data.Length()); return m_pool.Capacity(); }
	uint Size()     const { return m_map.Size(); }
private:
	PoolBase<false> m_pool;
	HandleMap m_map;
	Soa<T> m_data;
};

//------------------------------------------------------------------------------
#include "soapool.hpp"
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------