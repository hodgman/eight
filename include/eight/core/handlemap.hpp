
HandleMap::HandleMap( Scope& a, uint capacity )
	: m_h2p( eiAllocArray(a, u16, capacity) )
	, m_p2h( eiAllocArray(a, u16, capacity) )
	eiDEBUG( COMMA dbg_capacity(capacity) )
{
	Clear(capacity);
}

u16 HandleMap::ToPhysical( uint handle ) const
{
	eiASSERT( handle < dbg_capacity );
	eiDEBUG( m_h2p[handle] = 0xFFFFU );
	return m_h2p[handle];
}

u16 HandleMap::ToHandle( uint physical ) const
{
	eiASSERT( physical < dbg_capacity );
	eiDEBUG( m_p2h[physical] = 0xFFFFU );
	return m_p2h[physical];
}

u16 HandleMap::AllocHandleToPhysical( u16 handle )
{
	u16 physical = m_size++;
	eiASSERT( m_p2h[physical] == 0xFFFFU );
	eiASSERT( m_h2p[handle]   == 0xFFFFU );
	m_p2h[physical] = handle;
	m_h2p[handle] = physical;
	return physical;
}

HandleMap::MoveCmd HandleMap::EraseHandle( u16 handle )
{
	eiASSERT( m_size && handle != 0xFFFFU );
	u16 physical = m_h2p[handle];
	eiDEBUG( m_h2p[handle] = 0xFFFFU );
	u16 lastPhysical = m_size-1;
	--m_size;
	u16 handleLast = m_p2h[lastPhysical];
	eiDEBUG( m_p2h[lastPhysical] = 0xFFFFU );
	m_p2h[physical] = handleLast;
	m_h2p[handleLast] = physical;
	MoveCmd cmd = { lastPhysical, physical };
	return cmd;
}

void HandleMap::Clear(uint capacity)
{
	eiASSERT( capacity == dbg_capacity );
	m_size = 0;
	eiDEBUG( memset(m_h2p, 0xFFFFFFFF, capacity*sizeof(u16)));
	eiDEBUG( memset(m_p2h, 0xFFFFFFFF, capacity*sizeof(u16)));
}