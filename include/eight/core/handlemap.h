//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/new.h>
#include <eight/core/alloc/scope.h>
namespace eight {
//------------------------------------------------------------------------------

class HandleMap : NonCopyable
{
public:
	HandleMap( Scope& a, uint capacity );
	u16 ToPhysical( uint handle ) const;
	u16 ToHandle( uint physical ) const;
	u16 AllocHandleToPhysical( u16 handle );
	struct MoveCmd { u16 from; u16 to; };
	MoveCmd EraseHandle( u16 handle );
	void Clear(uint capacity);
	uint Size() const { return m_size; }
private:
	u16* m_h2p;
	u16* m_p2h;
	uint m_size;
	eiDEBUG( uint dbg_capacity );
};

//------------------------------------------------------------------------------
#include "handlemap.hpp"
//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------