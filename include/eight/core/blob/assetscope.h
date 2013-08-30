//------------------------------------------------------------------------------
#pragma once
#include "asset.h"
#include "eight/core/alloc/interface.h"
#include "eight/core/thread/hashtable.h"
#include "eight/core/thread/tasksection.h"
namespace eight {
	class Scope;
	class BlobLoader;
	class AssetScope;
	class AssetRootImpl;
	struct BlobLoadContext;
	template<class T> class BlobFactory;
//------------------------------------------------------------------------------

class AssetRoot : Interface<AssetRoot>
{
public:
	AssetRoot(Scope&, BlobLoader&, uint maxScopes=16);

	void HandleRefreshInterrupt(Scope& a, AssetName* files, uint count, uint worker, uint numWorkers, AtomicPtr<void>& state);
private:
	~AssetRoot();
	friend class AssetScope;
	//{
	void Created(AssetScope&, uint depth);
	void Destroyed(AssetScope&);
	//}
};

class AssetScope : NonCopyable
{
	typedef void* (*PfnAllocate)(uint numBlobs, uint idx, u32 blobSize, BlobLoadContext*, uint workerIdx);//called by all threads. Only 1 thread should return non-null per idx (deterministicly). Can return 0xFFFFFFFF to indicate deliberate NULL.
	typedef void  (*PfnComplete)(uint numBlobs, u8* blobData[], u32 blobSize[], BlobLoadContext&, BlobLoader&);//called by only 1 thread (onComplete)
	typedef void  (*PfnResolve) (void*, Handle);
	typedef void  (*PfnRelease) (void*, Handle);
	typedef void  (*PfnRefresh) (void*, Handle, uint numBlobs, u8* blobData[], u32 blobSize[], BlobLoadContext&, BlobLoader&);
public:
	AssetScope( Scope& a, const SingleThread& allocOwner, const ThreadMask& users, uint maxAssets, uint maxFactories, AssetRoot& parent );
	AssetScope( Scope& a, const SingleThread& allocOwner, const ThreadMask& users, uint maxAssets, uint maxFactories, AssetScope& parent );
	~AssetScope();

	void Release(uint worker);//must be called by all users prior to destruction

	void Close();//don't call more than once
	enum State
	{
		Loading,
		Resolving,
		Loaded,
	};
	State Update(uint worker);

	struct FactoryInfo
	{
		SingleThread factoryThread;
		PfnAllocate  onAllocate;
		PfnComplete  onComplete;
		PfnResolve   onResolve;
		PfnRelease   onRelease;
#if defined(eiASSET_REFRESH)
		PfnRefresh   onRefresh;
#endif
		uint         resolveOrder;
		const char*  factoryName;
	};
	template<class F>
	Asset* Load( AssetName n, BlobLoader& blobs, F& factory )//thread safe
	{
		FactoryInfo info = 
		{
			factory.AssetHandlingThread(),
			&F::s_OnAllocate,
			&F::s_OnBlobLoaded,
			F::GetResolve(),
			&F::s_OnRelease,
#if defined(eiASSET_REFRESH)
			F::GetRefresh(),
#endif
			F::GetResolveOrder(),
			F::DebugName()
		};
		return Load( n, blobs, &factory, info );
	}
	Asset* Load( AssetName n, BlobLoader& blobs, void* factory, const FactoryInfo& );//thread safe

private:
	friend class AssetRootImpl;
	//{
	void BeginAssetRefresh();
	void EndAssetRefresh();
	AssetStorage* Refresh(AssetName, BlobLoader&, Scope& temp, AssetName** depends, uint* numDepends);
	//}
	uint Depth();
	void Resolve(uint worker, uint pass);
	struct FactoryData
	{
		Atomic       ready;
		SingleThread factoryThread;
		uint         resolveOrder;
		PfnResolve   onResolve;
		PfnRelease   onRelease;
#if defined(eiASSET_REFRESH)
		PfnRefresh   onRefresh;
		PfnAllocate  onAllocate;
		PfnComplete  onComplete;
#endif
		AtomicPtr<AssetStorage> assets;
	};
	AssetName* LoadRefresh(AssetStorage*, void* factory, const FactoryData&, BlobLoader&, Scope& a, uint* numDepends);
	template<class T> friend class BlobFactory;
	//{
	void* Allocate( AssetStorage& asset, uint size, uint workerIdx );//dont call directly. used by factories.
	void OnBlobLoaded( AssetStorage& asset );//dont call directly. used by factories.
	//}

	bool IsLoading() const;//if false: loaded but maybe not resolved

	FactoryData* FindFactoryBucket(void*, const FactoryInfo&);

	AssetRoot* m_root;
	AssetScope* m_parent;
	typedef MpmcHashTable<AssetName, AssetStorage> AssetMap;
	typedef MpmcHashTable<void*,     FactoryData>  FactoryMap;
	AssetMap m_assets;
	FactoryMap m_factories;
	const uint m_maxFactories;
	Scope& m_a;
	SingleThread m_allocOwner;
	ThreadMask   m_userThreads;
	Atomic m_resolved;
	Atomic m_loadsInProgress;
	Atomic m_maxResolveOrder;
#if defined(eiASSET_REFRESH)
	Atomic m_assetRefreshMode;
#endif
	const static s32 s_loadingLock = 0x80000000;
	eiDEBUG( Atomic m_dbgReleased );
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------