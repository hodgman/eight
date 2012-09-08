//------------------------------------------------------------------------------
#pragma once
#include "asset.h"
#include "eight/core/alloc/scope.h"
#include "eight/core/thread/tasksection.h"
#include "eight/core/thread/hashtable.h"
namespace eight {
//------------------------------------------------------------------------------

class AssetScope
{
public:
	AssetScope( Scope& a, const ThreadGroup& allocOwner, uint maxAssets, AssetScope* parent=0 ) : m_parent(parent), m_assets(a, maxAssets, maxAssets*7/10), a(a), allocOwner(allocOwner), m_loadsInProgress()
	{
	}

	void Close()//don't call more than once
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
					a.Seal();
					ok = m_loadsInProgress.SetIfEqual(0xFFFFFFFF, s_loadingLock);
					eiASSERT( ok );
					break;
				}
			}while( !ok );
#else
			m_loadsInProgress += s_loadingLock;
			s32 loads = m_loadsInProgress;
			if( loads == s_loadingLock )
			{
				a.Seal();
				m_loadsInProgress = 0xFFFFFFFF;
			}
#endif
	}
	bool Loaded()
	{
		s32 loads = m_loadsInProgress;
		return loads == 0xFFFFFFFF;
	}

	template</*class T, */class F>
	Asset* Load( AssetName n, BlobLoader& blobs, F& factory )//thread safe
	{
		eiASSERT( !a.Sealed() );
	//	eiASSERT_CAST(Asset, T);
	//	eiASSERT(!m_assets.Find(n));

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
			BlobLoadContext ctx = { this, ptr, &factory/*, eiDEBUG(n.String()) */};
			bool noFactoryAlloc = true;//TODO - allow factories to allocate the blobs
			BlobLoader::Request req =
			{
				/*onComplete */ factory.OnLoadedThread(),
				/*userData   */ ctx,
				/*pfnAllocate*/ &F::s_OnAllocate,
				/*pfnComplete*/ &F::s_OnBlobLoaded,
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

	template<class T>
	T* Find( AssetName n )//thread safe
	{
		eiASSERT( Sealed() );
		eiASSERT_CAST(Asset, T);
		AssetInfo* value = m_assets.Find(n);
		if(!value)
			return m_parent ? m_parent->Find<T>(n) : 0;
		if( value->type != eiTYPE_OF(T) )
		{
			eiASSERT(false);
			return 0;
		}
		return static_cast<T*>(value->ptr);
	}

	void* Allocate( uint size, uint workerIdx )//dont call directly. used by factories.
	{
		eiASSERT( s_loadingLock != 0xFFFFFFFF );
		if( allocOwner.WorkerIndex() == workerIdx )
		{
			eiAssertInTaskSection( allocOwner );
			return a.Alloc( size );
		}
		return 0;
	}

	void OnBlobLoaded()//dont call directly. used by factories.
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
				a.Seal();
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
				a.Seal();
				m_loadsInProgress = 0xFFFFFFFF;
				done = true;
			}
			else
				done = m_loadsInProgress.SetIfEqual(loading-1, loading);
		}while(!done);
#endif
	}
private:
/*	static void* OnAllocate(uint numBlobs, u32 blobSize[], u8  blobHint[], BlobLoadContext* ctx)
	{
		eiAssertInTaskSection( allocOwner );
		eiASSERT( numBlobs == 1 );//TODO
		return ctx->scope->a.Alloc( blobSize[0] );
	}*/

	AssetScope* m_parent;
	typedef MpmcHashTable<AssetName, AssetStorage> AssetMap;
	//typedef StaticHashMap<AssetName, AssetInfo> AssetMap;
	AssetMap m_assets;
	Scope& a;
	ThreadGroup allocOwner;
	Atomic m_loadsInProgress;
	const static s32 s_loadingLock = 0x80000000;
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------