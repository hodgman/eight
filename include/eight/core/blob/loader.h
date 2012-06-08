//------------------------------------------------------------------------------
#pragma once
#include "eight/core/debug.h"
#include "eight/core/types.h"
#include "eight/core/alloc/new.h"
#include "eight/core/thread/tasksection.h"
namespace eight {
class Scope;
class TaskLoop;
eiInfoGroup(BlobLoader, false);
//------------------------------------------------------------------------------
struct Asset;
struct AssetStorage;
class AssetName;
class AssetScope;

struct BlobConfig { const char* path; uint maxRequests; };

struct BlobLoadContext
{
	AssetScope* scope;
	AssetStorage* asset;
	void* factory;
	eiDEBUG( const char* dbgName );
};

eiConstructAs( BlobLoader, BlobLoaderWin32 );

class BlobLoader 
{
protected:
	BlobLoader(){}
public:
	struct Request
	{
		typedef void* (*PfnAllocate)(uint numBlobs, u32 blobSize[], u8  blobHint[], BlobLoadContext*);
		typedef void  (*PfnComplete)(uint numBlobs, u8* blobData[], u32 blobSize[], BlobLoadContext*);

		ThreadGroup     onAllocate;
		ThreadGroup     onComplete;
		BlobLoadContext userData;
		PfnAllocate     pfnAllocate;
		PfnComplete     pfnComplete;
	};
	bool Load(const AssetName&, const Request&);//call at any time from any thread. Can fail if internal queues are full and if so should try again next frame.
	void Update();//should be called by all threads each frame
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------