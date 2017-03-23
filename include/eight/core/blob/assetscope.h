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

	void HandleRefreshInterrupt(Scope& a, AssetName* files, uint count, const ThreadId&, AtomicPtr<void>& state);
private:
	~AssetRoot();
	friend class AssetScope;
	//{
	void Created(AssetScope&, uint depth);
	void Destroyed(AssetScope&);
	//}
};

class AssetsLoadedCallback
{
public:
	AssetsLoadedCallback(callback, void*, TaskSection, const char* dbgName);
	~AssetsLoadedCallback();
private:
	friend class AssetScope;
	callback fn;
	void* arg;
	TaskSection fnTask;
	AssetsLoadedCallback* next;
	eiDEBUG( const char* dbgName );
	eiDEBUG( bool done );
};

class AssetScope : NonCopyable
{
	typedef void* (*PfnAllocate)(uint numBlobs, uint idx, u32 blobSize, BlobLoadContext*, uint workerIdx);//called by all threads. Only 1 thread should return non-null per idx (deterministicly). Can return 0xFFFFFFFF to indicate deliberate NULL.
	typedef void  (*PfnComplete)(uint numBlobs, u8* blobData[], u32 blobSize[], BlobLoadContext&, BlobLoader&);//called by only 1 thread (onComplete)
	typedef void  (*PfnResolve) (void*, Handle);
	typedef void  (*PfnRelease) (void*, Handle);
	typedef void  (*PfnRefresh) (void*, Handle, uint numBlobs, u8* blobData[], u32 blobSize[], BlobLoadContext&, BlobLoader&);
public:
	AssetScope( Scope& a, const SingleThread& allocOwner, const ThreadMask& users, uint maxAssets, uint maxFactories, AssetRoot& parent, const char* dbgName );
	AssetScope( Scope& a, const SingleThread& allocOwner, const ThreadMask& users, uint maxAssets, uint maxFactories, AssetScope& parent, const char* dbgName );
	~AssetScope();

	void Release(const ThreadId&);//must be called by all users prior to destruction

	void Close();//don't call more than once
	enum State
	{
		Loading,
		Resolving,
		Loaded,
	};
	State Update(const ThreadId&);

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
	{//can't call if sealed
		if( m_parent )
		{
			Asset* found = m_parent->Find(n);
			if( found )
				return found;
		}
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

	void OnLoaded( AssetsLoadedCallback& );
private:
	void OnLoaded();

	Asset* Find( AssetName );

	friend class AssetRootImpl;
	//{
	void BeginAssetRefresh();
	void EndAssetRefresh();
	AssetStorage* Refresh(AssetName, BlobLoader&, Scope& temp, AssetName** depends, uint* numDepends);
	//}
	uint Depth();
	bool Resolve(uint worker, uint pass);
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
	bool IsLoaded() const;

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
	AtomicPtr<AssetsLoadedCallback> m_onLoaded;
	const static s32 s_loadingLock = 0x80000000;
	eiDEBUG( Atomic m_dbgReleased );
	eiDEBUG( const char* m_dbgName );
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------