#include <eight/core/blob/types.h>
#include <eight/core/blob/loader.h>
#include <eight/core/blob/asset.h>
#include "eight/core/os/win32.h"
#include "eight/core/thread/fifo_mpmc.h"
#include "eight/core/thread/atomic.h"
#include "eight/core/hash.h"

using namespace eight;

//------------------------------------------------------------------------------

AssetName::AssetName(const char* n) : hash(Fnv32a(n)){}

//------------------------------------------------------------------------------
#ifdef eiASSET_REFRESH
void AssetStorage::FreeRefreshData()
{
	if( m_refresh.data )
		_aligned_free( m_refresh.data );
}
#endif


void AssetStorage::Assign(Handle h)
{
#ifdef eiASSET_REFRESH
//	eiASSERT( (!id && m_handle.ptr != data) || (id && m_handle.id != id) );
	if( !m_handle )
	{
		m_original.inUse = true;
		m_original.data = data;
		eiDEBUG( m_original.size = size );
	}
	else if( m_original.data == data )
	{
		m_original.inUse = true;
		m_refresh.inUse = false;
		eiASSERT( size <= m_original.size );
	}
	else 
	{
		m_original.inUse = false;
		m_refresh.inUse = true;
		if( m_refresh.data != data )
		{
			if( m_refresh.data )
				_aligned_free( m_refresh.data );
			m_refresh.data = data;
			eiDEBUG( m_refresh.size = size );
		}
		else
			eiASSERT( size <= m_refresh.size );
	}
#endif
	eiSTATIC_ASSERT( sizeof(s32) == sizeof(m_handle) );
	((Atomic&)m_handle.id) = (s32)h.id;
/*	if( id )
		((Atomic&)m_handle.id) = (s32)id;
	else
		((Atomic&)m_handle.ptr) = (s32)data;*/
//	eiDEBUG( m_size = size );
//	AtomicWrite(&m_loaded, 1);
}
/*
void* RefreshHeap::OnAllocate(uint numBlobs, uint blobIdx, u32 size, BlobLoadContext* context)
{
	eiASSERT( numBlobs==1 );
	eiASSERT( size );
	eiASSERT( size );
#ifdef eiASSET_REFRESH
	AssetStorage* asset = context->asset;
	eiASSERT( asset->Loaded() );
	if( asset->m_refresh.data && !asset->m_refresh.inUse && asset->m_refresh.size >= size[0] )//refresh storage area unused and large enough
	{
		return asset->m_refresh.data;
	}
	if( !asset->m_original.inUse && asset->m_original.size >= size[0] )//original storage area unused and large enough
	{
		eiASSERT( asset->m_original.data );
		return asset->m_original.data;
	}
#endif
	return _aligned_malloc(size[0], 16);
}
void RefreshHeap::OnFree(u8* alloc)
{
	_aligned_free(alloc);
}*/

//------------------------------------------------------------------------------
