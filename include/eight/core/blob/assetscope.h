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
	AssetScope( Scope& a, const ThreadGroup& allocOwner, uint maxAssets, AssetScope* parent=0 ) : m_parent(parent), m_assets(a, maxAssets, maxAssets*7/10), a(a), allocOwner(allocOwner)
	{
	}

	void Seal()
	{
		eiAssertInTaskSection( allocOwner );
		a.Seal();
	}
	bool Sealed() const
	{
		eiAssertInTaskSection( allocOwner );
		return a.Sealed();
	}

	template</*class T, */class F>
	Asset* Load( AssetName n, BlobLoader& blobs, F& factory )
	{
		eiASSERT( !Sealed() );
	//	eiASSERT_CAST(Asset, T);
	//	eiASSERT(!m_assets.Find(n));

		int asset = -1;
		if( m_assets.Insert(n, asset) )
		{
			AssetStorage* ptr = 0;
			ptr = &m_assets.At(asset);
			memset(ptr, 0, sizeof(AssetStorage));
			BlobLoadContext ctx = { this, ptr, &factory, eiDEBUG(n.String()) };
			bool noFactoryAlloc = true;//TODO - allow factories to allocate the blobs
			BlobLoader::Request req =
			{
				/*onAllocate */ noFactoryAlloc ? allocOwner : ThreadGroup(),
				/*onComplete */                               ThreadGroup(),   // - TODO - ask factory for a task section
				/*userData   */                               ctx,
				/*pfnAllocate*/ noFactoryAlloc ? 
				                    &AssetScope::OnAllocate : 0,
				/*pfnComplete*/                               &F::OnBlobLoaded,
			};
			bool ok = blobs.Load( n, req );
			eiASSERT( ok );//TODO - mark the asset as needing to try to load again next time Load is called with 'n'
			return ptr;
		}
		else if( asset != -1 )
			return &m_assets.At(asset);
		eiASSERT( false );
		return 0;
	}

	template<class T>
	T* Find( AssetName n )
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
private:
	static void* OnAllocate(uint numBlobs, u32 blobSize[], u8  blobHint[], BlobLoadContext* ctx)
	{
		eiAssertInTaskSection( allocOwner );
		eiASSERT( numBlobs == 1 );//TODO
		return ctx->scope->a.Alloc( blobSize[0] );
	}

	AssetScope* m_parent;
	typedef MpmcHashTable<AssetName, AssetStorage> AssetMap;
	//typedef StaticHashMap<AssetName, AssetInfo> AssetMap;
	AssetMap m_assets;
	Scope& a;
	ThreadGroup allocOwner;
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------