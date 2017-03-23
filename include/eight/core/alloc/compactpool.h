//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/pool.h>
#include <eight/core/id.h>
#include <eight/core/handlemap.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T> class CompactPool : NonCopyable
{
	const static bool s_NonPod = true;//todo
public:
	eiTYPEDEF_ID(Id);
	CompactPool( Scope& a, uint size );
	~CompactPool();
	Id Handle(const T& object);
	Id Alloc( const T& data);
	Id Alloc();
	void Release( Id handle );
	T& operator[]( Id handle );
	const T* Begin() const { return m_data; }
	      T* Begin()       { return m_data; }
	const T* End()   const { return m_data+Size(); }
	      T* End()         { return m_data+Size(); }
	uint Capacity()  const { return m_pool.Capacity(); }
	uint Size()      const { return m_map.Size(); }
	void Clear();
	template<class Fn> void ForEach(Fn& fn);
	template<class Fn> void ForEach(Fn& fn) const;
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