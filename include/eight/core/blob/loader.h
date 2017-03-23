//------------------------------------------------------------------------------
#pragma once
#include "eight/core/debug.h"
#include "eight/core/types.h"
#include "eight/core/noncopyable.h"
#include "eight/core/alloc/interface.h"
#include "eight/core/thread/tasksection.h"
namespace eight {
class Scope;
class TaskLoop;
eiInfoGroup(BlobLoader, true);
//------------------------------------------------------------------------------
struct Asset;
struct AssetStorage;
struct AssetName;
class AssetScope;

struct BlobConfig
{
	const char* path;
	const char* devPath;
	uint maxRequests;
	const char* manifestFile;
	uint numLayers;
	const char*const* layers;
	SingleThread osWorkerThread;
	JobPool* backgroundPool;
};

struct BlobLoadContext
{
	AssetScope* scope;
	AssetStorage* asset;
	void* factory;
	const char* assetName;
	eiDEBUG( const char* dbgFactoryName );
};

class BlobLoader : Interface<BlobLoader>
{
public:
	BlobLoader(Scope&, const BlobConfig&, const TaskLoop&);

	enum States
	{
		Initializing,
		Error,
		Ready,
	};
	States Prepare();//call repeatedly by the creation thread, until Error/Ready is returned.
	struct Request
	{
		typedef void* (*PfnAllocate)(uint numBlobs, uint idx, u32 blobSize, BlobLoadContext*, uint workerIdx);//called by all threads. Only 1 thread should return non-null per idx (deterministicly). Can return 0xFFFFFFFF to indicate deliberate NULL.
		typedef void  (*PfnComplete)(uint numBlobs, u8* blobData[], u32 blobSize[], BlobLoadContext&, BlobLoader&);//called by only 1 thread (onComplete)
		SingleThread     onComplete;
		BlobLoadContext userData;
		PfnAllocate     pfnAllocate;
		PfnComplete     pfnComplete;
	};
	bool Load(const AssetName&, const Request&);//call at any time from any thread. Can fail if internal queues are full and if so should try again next frame.
	void Update(const ThreadId&, bool inRefreshInterrupt=false);//should be called by all threads each frame
	void UpdateBackground();//should be called by all threads in the background pool


	struct ImmediateDevRequest
	{
		typedef void* (*PfnAllocate)(u32 blobSize);
		typedef void  (*PfnComplete)(u8* blobData, u32 blobSize);
		PfnAllocate     pfnAllocate;
		PfnComplete     pfnComplete;
	};
#if !defined(eiBUILD_RETAIL)
	void ImmediateDevLoad(const char* path, const ImmediateDevRequest&);
#endif
private:
	~BlobLoader();
};

struct BlobLoaderInitialized
{
	BlobLoaderInitialized( BlobLoader& b ) : b(b) {} BlobLoader& b;
	bool operator()() const { return b.Prepare() != BlobLoader::Initializing; }
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------