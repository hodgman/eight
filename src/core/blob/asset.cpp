#include <eight/core/blob/types.h>
#include <eight/core/blob/loader.h>
#include <eight/core/blob/assetscope.h>
#include "eight/core/thread/fifo_mpmc.h"
#include "eight/core/thread/atomic.h"
#include "eight/core/hash.h"
#include "eight/core/alloc/scope.h"
#include "eight/core/thread/tasksection.h"
#include "eight/core/thread/pool.h"

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
//------------------------------------------------------------------------------

AssetScope::AssetScope( Scope& a, const SingleThread& allocOwner, const ThreadMask& users, uint maxAssets, uint maxFactories, AssetScope* parent )
	: m_parent(parent)
	, m_assets(a, maxAssets, maxAssets*7/10)
	, m_factories(a, maxFactories, maxFactories*7/10)
	, m_maxFactories(maxFactories)
	, m_a(a)
	, m_allocOwner(allocOwner)
	, m_userThreads(users)
	, m_loadsInProgress()
{
	//TODO - assert parent is null or closed
}

void AssetScope::Close()
{
#if eiDEBUG_LEVEL > 0
		bool ok;
		do
		{
			s32 i = m_loadsInProgress;
			eiASSERT( (i & s_loadingLock) == 0 );
			eiASSERT( ((i + s_loadingLock) & (~s_loadingLock)) == i );
			eiASSERT( (i + s_loadingLock) == (i|s_loadingLock) );
			ok = m_loadsInProgress.SetIfEqual(i|s_loadingLock, i);
			if( i == 0 && ok )
			{
				m_a.Seal();
				ok = m_loadsInProgress.SetIfEqual(0xFFFFFFFF, s_loadingLock);
				eiASSERT( ok );
				break;
			}
		}while( !ok );
#else
		m_loadsInProgress += s_loadingLock;
		s32 loads = m_loadsInProgress;
		eiASSERT( loads & s_loadingLock );
		if( loads == s_loadingLock )
		{
			m_a.Seal();
			m_loadsInProgress = 0xFFFFFFFF;
		}
#endif
}

AssetScope::State AssetScope::Update(uint worker)
{
	eiASSERT( CurrentPoolThread().index == worker );
	if( IsLoading() )
		return Loading;
	s32 resolved = m_resolved;
	s32 userMask = (s32)m_userThreads.GetMask();
	if( resolved != userMask )
	{
		s32 thisMask = 1<<worker;
		if( (resolved & thisMask) == 0 )
		{
			Resolve(worker);
			m_resolved += thisMask;
		}
		if( (resolved|thisMask) != userMask )
			return Resolving;
	}
	return Loaded;
}
	
bool AssetScope::IsLoading() const
{
	s32 loads = m_loadsInProgress;
	return loads != 0xFFFFFFFF;
}

AssetScope::~AssetScope()
{
	eiASSERT( "todo" );
	//todo
}
void AssetScope::Resolve(uint worker)
{
	struct FnResolve
	{
		void operator()( void* factory, FactoryData& data )
		{
			eiASSERT( factory && (s32)data.ready );
			PfnResolve onResolve = data.onResolve;
			if( onResolve )
			{
				eiBeginSectionThread( data.factoryThread )//todo same thread mask (rename if so) or another?
				{
					for( AssetStorage* asset = data.assets; asset; asset = asset->m_next )
					{
						onResolve( factory, *asset );
					}
				}
				eiEndSection( data.factoryThread );
			}
		}
	};
	m_factories.ForEach( FnResolve() );
}

AssetScope::FactoryData* AssetScope::FindFactoryBucket(void* factory, const FactoryInfo& info)
{
	int index = -1;
	bool inserted = m_factories.Insert(factory, index);
	if( !inserted && index < 0 )
		return 0;
	FactoryData& data = m_factories.At(index);
	if( inserted )
	{
		data.factoryThread = info.factoryThread;
		data.onResolve     = info.onResolve;
		data.onRelease     = info.onRelease;
		data.assets = 0;
		data.ready = 1;
	}
	else
	{
		YieldThreadUntil( WaitFor1(data.ready) );
		eiASSERT( ((SectionBlob*)&info.factoryThread)->bits == ((SectionBlob*)&data.factoryThread)->bits );
		eiASSERT( info.onResolve  == data.onResolve );
		eiASSERT( info.onRelease  == data.onRelease );
	}
	return &data;
}

Asset* AssetScope::Load( AssetName n, BlobLoader& blobs, void* factory, const FactoryInfo& info )
{
	//TODO - look for asset in parent!
	eiASSERT( !m_a.Sealed() );
	int asset = -1;
	if( m_assets.Insert(n, asset) )
	{
		bool ok;
#if eiDEBUG_LEVEL > 0
		s32 i = m_loadsInProgress;
		eiASSERT( (i & s_loadingLock) == 0 );
		ok = m_loadsInProgress.SetIfEqual(i+1, i);
		eiASSERT( ok );
#else
		++m_loadsInProgress;
#endif
		AssetStorage* ptr = 0;
		ptr = &m_assets.At(asset);
		memset(ptr, 0, sizeof(AssetStorage));
		
		FactoryData* factoryData = FindFactoryBucket(factory, info);
		do
		{
			ptr->m_next = factoryData->assets;
		}while( !factoryData->assets.SetIfEqual(ptr, ptr->m_next) );

		BlobLoadContext ctx = { this, ptr, factory/*, eiDEBUG(n.String()) */};
		BlobLoader::Request req =
		{
			/*onComplete */ info.factoryThread,
			/*userData   */ ctx,
			/*pfnAllocate*/ info.onAllocate,
			/*pfnComplete*/ info.onComplete,
		};
		ok = blobs.Load( n, req );
		eiASSERT( ok );//TODO - mark the asset as needing to try to load again next time Load is called with 'n'
		return ptr;
	}
	else if( asset != -1 )
		return &m_assets.At(asset);
	eiASSERT( false );
	return 0;
}

void* AssetScope::Allocate( uint size, uint workerIdx )
{
	eiDEBUG( s32 i = m_loadsInProgress );
	if( m_allocOwner.WorkerIndex() == workerIdx )
	{
		eiAssertInTaskSection( m_allocOwner );
		return m_a.Alloc( size );
	}
	return 0;
}

void AssetScope::OnBlobLoaded()
{
#if eiDEBUG_LEVEL > 0
	bool done;
	do
	{
		s32 loading = m_loadsInProgress;
		eiASSERT( loading != 0xFFFFFFFF );
		if( (((loading-1) & s_loadingLock) == s_loadingLock) && (((loading-1) & ~s_loadingLock) == 0) )
		{
			eiASSERT( loading == s_loadingLock+1 );
			bool ok = m_loadsInProgress.SetIfEqual(0, s_loadingLock+1);
			eiASSERT( ok );
			m_a.Seal();
			ok = m_loadsInProgress.SetIfEqual(0xFFFFFFFF, 0);
			eiASSERT( ok );
			done = true;
		}
		else
		{
			done = m_loadsInProgress.SetIfEqual(loading-1, loading);
			eiASSERT( loading != s_loadingLock+1 );
		}
	}while(!done);
#else
	bool done;
	do
	{
		s32 loading = m_loadsInProgress;
		if( loading == s_loadingLock+1 )
		{
			m_loadsInProgress = 0;
			m_a.Seal();
			m_loadsInProgress = 0xFFFFFFFF;
			done = true;
		}
		else
			done = m_loadsInProgress.SetIfEqual(loading-1, loading);
	}while(!done);
#endif
}
//------------------------------------------------------------------------------
