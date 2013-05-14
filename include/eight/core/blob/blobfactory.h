//------------------------------------------------------------------------------
#pragma once
#include "loader.h"
#include "asset.h"
namespace eight {
//------------------------------------------------------------------------------

template<class Self>
class BlobFactory
{
public:
	static uint GetResolveOrder() { return 0; }
private:
	friend class AssetScope;
	SingleThread AssetHandlingThread()
	{
		return SingleThread();
	}
	static void* s_OnAllocate(uint numBlobs, uint idx, u32 blobSize, BlobLoadContext* ctx, uint workerIdx)
	{
		return ((Self*)(ctx->factory))->OnAllocate( numBlobs, idx, blobSize, ctx, workerIdx );
	}
	static void s_OnBlobLoaded( uint numBlobs, u8* data[], u32 size[], BlobLoadContext& ctx, BlobLoader& loader )
	{
		((Self*)(ctx.factory))->OnBlobLoaded( numBlobs, data, size, ctx, loader );
	}
	typedef void(FnResolve)(void*,Handle);
	static FnResolve* GetResolve() { if(&Self::Resolve==&BlobFactory<Self>::Resolve) return 0; else return &s_OnResolve; }
	static void s_OnResolve( void* factory, Handle h )
	{
		((Self*)factory)->Resolve( h );
	}
	static void s_OnRelease( void* factory, Handle h )
	{
		((Self*)factory)->Release( h );
	}
	void Resolve( Handle ) {}
protected:
	static void* ScopeAllocate( AssetScope& scope, uint size, uint workerIdx )
	{
		return scope.Allocate( size, workerIdx );
	}
	void* OnAllocate(uint numBlobs, uint idx, u32 blobSize, BlobLoadContext* ctx, uint workerIdx)
	{
		eiASSERT( numBlobs == 1 );//TODO
		eiASSERT( idx < numBlobs );
		eiASSERT( blobSize );
		eiASSERT( ctx && ctx->factory == this );
		return ctx->scope->Allocate( blobSize, workerIdx );
	}
	void OnBlobLoaded( uint numBlobs, u8* data[], u32 size[], BlobLoadContext& context, BlobLoader& loader )
	{
		eiASSERT( context.factory == this );
		Self* self = (Self*)this;
		AssetStorage* asset = context.asset;
		bool reload = false;
		if( asset->Data() )
		{
			reload = true;
			self->Release( *asset );
		}
		Handle h = self->Acquire( numBlobs, data, size, *context.scope, loader );
		asset->Assign( h );
		eiASSERT( asset->Data() );
		if( !reload )
			context.scope->OnBlobLoaded();
	}
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------