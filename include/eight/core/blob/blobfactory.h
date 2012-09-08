//------------------------------------------------------------------------------
#pragma once
#include "loader.h"
#include "asset.h"
namespace eight {
//------------------------------------------------------------------------------

template<class Self>
class BlobFactory
{
private:
	friend class AssetScope;
	ThreadGroup OnLoadedThread()
	{
		return ThreadGroup();
	}
	static void* s_OnAllocate(uint numBlobs, uint idx, u32 blobSize, BlobLoadContext* ctx, uint workerIdx)
	{
		return ((Self*)(ctx->factory))->OnAllocate( numBlobs, idx, blobSize, ctx, workerIdx );
	}
	static void s_OnBlobLoaded( uint numBlobs, u8* data[], u32 size[], BlobLoadContext* ctx )
	{
		((Self*)(ctx->factory))->OnBlobLoaded( numBlobs, data, size, ctx );
	}
	static void GetDefaultBlob( u8*& data, uint& size )
	{
		data = 0;
		size = 0;
	}
protected:
	void* OnAllocate(uint numBlobs, uint idx, u32 blobSize, BlobLoadContext* ctx, uint workerIdx)
	{
		eiASSERT( numBlobs == 1 );//TODO
		eiASSERT( idx < numBlobs );
		eiASSERT( blobSize );
		eiASSERT( ctx && ctx->factory == this );
		return ctx->scope->Allocate( blobSize, workerIdx );
	}
	void OnBlobLoaded( uint numBlobs, u8* data[], u32 size[], BlobLoadContext* context )
	{
		eiASSERT( context && context->factory == this );

		u8* defaultData[1]; u32 defaultSize[1];
		if(!numBlobs)
		{
			eiWarn( "Blob load failed for asset %s", "?"/*context->dbgName*/ );
			Self::GetDefaultBlob( (data=defaultData)[0], (size=defaultSize)[0] );
			numBlobs = 1;
		}

		Self* self = (Self*)this;
		AssetStorage* asset = context->asset;
		bool reload = false;
		if( asset->Data() )
		{
			reload = true;
			self->Release( *asset );
		}
		Handle h = self->Acquire( numBlobs, data, size );
		asset->Assign( h );
		eiASSERT( asset->Data() );
		if( !reload )
			context->scope->OnBlobLoaded();
	}
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------