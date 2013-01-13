//------------------------------------------------------------------------------
#pragma once
#include "asset.h"
#include "eight/core/noncopyable.h"
#include "eight/core/thread/hashtable.h"
#include "eight/core/thread/tasksection.h"
namespace eight {
	class Scope;
	class BlobLoader;
	template<class T> class BlobFactory;
//------------------------------------------------------------------------------

class AssetScope : NonCopyable
{
	typedef void* (*PfnAllocate)(uint numBlobs, uint idx, u32 blobSize, BlobLoadContext*, uint workerIdx);//called by all threads. Only 1 thread should return non-null per idx (deterministicly). Can return 0xFFFFFFFF to indicate deliberate NULL.
	typedef void  (*PfnComplete)(uint numBlobs, u8* blobData[], u32 blobSize[], BlobLoadContext*);//called by only 1 thread (onComplete)
	typedef void  (*PfnResolve) (void*, Handle);
	typedef void  (*PfnRelease) (void*, Handle);
public:
	AssetScope( Scope& a, const SingleThread& allocOwner, const ThreadMask& users, uint maxAssets, uint maxFactories, AssetScope* parent=0 );
	~AssetScope();

	void Close();//don't call more than once
//	bool Loaded() const;
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
	};
	template<class F>
	Asset* Load( AssetName n, BlobLoader& blobs, F& factory )//thread safe
	{
		FactoryInfo info = 
		{
			factory.AssetHandlingThread(), &F::s_OnAllocate, &F::s_OnBlobLoaded, F::GetResolve(), &F::s_OnRelease
		};
		return Load( n, blobs, &factory, info );
	}
	Asset* Load( AssetName n, BlobLoader& blobs, void* factory, const FactoryInfo& );//thread safe

private:
	void Resolve(uint worker);
	struct FactoryData
	{
		Atomic ready;
		SingleThread factoryThread;
		PfnResolve   onResolve;
		PfnRelease   onRelease;
		AtomicPtr<AssetStorage> assets;
	};
	template<class T> friend class BlobFactory;
	void* Allocate( uint size, uint workerIdx );//dont call directly. used by factories.
	void OnBlobLoaded();//dont call directly. used by factories.

	bool IsLoading() const;//if false, loaded but maybe not resolved

	FactoryData* FindFactoryBucket(void*, const FactoryInfo&);

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
	const static s32 s_loadingLock = 0x80000000;
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------