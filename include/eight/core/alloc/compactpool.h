//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/pool.h>
#include <eight/core/handlemap.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T> class CompactPool : NonCopyable
{
	const static bool s_NonPod = true;//todo
public:
	CompactPool( Scope& a, uint size );
	~CompactPool();
	int Handle(const T& object);
	int Alloc( const T& data);
	int Alloc();
	void Release( int handle );
	T& operator[]( int handle );
	const T* Begin() const { return m_data; }
	      T* Begin()       { return m_data; }
	const T* End()   const { return m_data+Size(); }
	      T* End()         { return m_data+Size(); }
	uint Capactiy()  const { return m_pool.Capactiy(); }
	uint Size()      const { return m_map.Size(); }
	void Clear();
private:
	PoolBase<false> m_pool;
	HandleMap m_map;
	T* m_data;
};

//------------------------------------------------------------------------------
#include "compactpool.hpp"
//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------