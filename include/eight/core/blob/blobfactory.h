//------------------------------------------------------------------------------
#pragma once
#include "loader.h"
#include "asset.h"
#include <eight/core/typeinfo.h>
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
	static const char* DebugName() { return TypeName<Self>::Get(); }
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
	void Resolve( Handle ) {}
#if defined(eiASSET_REFRESH)
	typedef void(FnRefresh)(void*, Handle, uint numBlobs, u8* data[], u32 size[], BlobLoadContext&, BlobLoader&);
	static FnRefresh* GetRefresh() { if(&Self::Refresh==&BlobFactory<Self>::Refresh) return 0; else return &s_Refresh; }
	static void s_Refresh( void* factory, Handle h, uint numBlobs, u8* data[], u32 size[], BlobLoadContext& ctx, BlobLoader& loader )
	{
		((Self*)factory)->Refresh( h );
	}
	void Refresh( Handle ) {}
#endif
	static void s_OnRelease( void* factory, Handle h )
	{
		((Self*)factory)->Release( h );
	}
	Handle Reacquire( Handle, uint numBlobs, u8* blobData[], u32 blobSize[], AssetScope&, BlobLoader&, Asset& )
	{
		eiASSERT(false);
		return Handle((void*)0);
	}
protected:
	static void* ScopeAllocate( BlobLoadContext& ctx, uint size, uint workerIdx )
	{
		return ctx.scope->Allocate( *ctx.asset, size, workerIdx );
	}
	/*void* OnAllocate(uint numBlobs, uint idx, u32 blobSize, BlobLoadContext* ctx, uint workerIdx)
	{
		eiASSERT( numBlobs == 1 );//TODO
		eiASSERT( idx < numBlobs );
		eiASSERT( blobSize );
		eiASSERT( ctx && ctx->factory == this );
		if( !ctx->isRefresh )
			return ctx->scope->Allocate( blobSize, workerIdx );
		else
		{
			eiASSERT(!"ToDo");
			return 0;
		}
	}*/
	void OnBlobLoaded( uint numBlobs, u8* data[], u32 size[], BlobLoadContext& context, BlobLoader& loader )
	{
		eiASSERT( context.factory == this );
		Self* self = (Self*)this;
		AssetStorage* asset = context.asset;
		bool isRefresh = !!asset->Data();
		Handle h((void*)0);
#if !defined(eiASSET_REFRESH)
		eiASSERT( !isRefresh );
#else
		if( isRefresh )
		{
			const bool hasReacquireFunc = !(&Self::Reacquire == &BlobFactory<Self>::Reacquire);
			if(hasReacquireFunc)
			{
				h = self->Reacquire( *asset, numBlobs, data, size, *context.scope, loader, *asset );
			}
			else
			{
				self->Release( *asset );
				h = self->Acquire( numBlobs, data, size, *context.scope, loader, *asset );
			}
		}
		else
#endif
		{
			h = self->Acquire( numBlobs, data, size, *context.scope, loader, *asset );
		}
		asset->Assign( h );
		eiASSERT( asset->Data() );
		context.scope->OnBlobLoaded(*asset);
	}
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------