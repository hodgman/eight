
inline HandleMap::HandleMap( Scope& a, uint capacity )
	: m_h2p( eiAllocArray(a, u16, capacity) )
	, m_p2h( eiAllocArray(a, u16, capacity) )
	eiDEBUG( COMMA dbg_capacity(capacity) )
{
	Clear(capacity);
}

inline u16 HandleMap::ToPhysical( uint handle ) const
{
	eiASSERT( handle < dbg_capacity );
	eiASSERT( handle == m_p2h[m_h2p[handle]] );
	return m_h2p[handle];
}

inline u16 HandleMap::ToHandle( uint physical ) const
{
	eiASSERT( physical < dbg_capacity );
	eiASSERT( physical == m_h2p[m_p2h[physical]] );
	return m_p2h[physical];
}

inline u16 HandleMap::AllocHandleToPhysical( u16 handle )
{
	u16 physical = m_size++;
	eiASSERT( m_p2h[physical] == 0xFFFFU );
	eiASSERT( m_h2p[handle]   == 0xFFFFU );
	m_p2h[physical] = handle;
	m_h2p[handle] = physical;
	return physical;
}

inline HandleMap::MoveCmd HandleMap::EraseHandle( u16 handle )
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

inline void HandleMap::Clear(uint capacity)
{
	eiASSERT( capacity == dbg_capacity );
	m_size = 0;
	eiDEBUG( memset(m_h2p, 0xFFFFFFFF, capacity*sizeof(u16)));
	eiDEBUG( memset(m_p2h, 0xFFFFFFFF, capacity*sizeof(u16)));
}