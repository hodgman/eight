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
	static void OnBlobLoaded( uint numBlobs, u8* data[], u32 size[], BlobLoadContext* context )
	{
		u8* defaultData[1]; u32 defaultSize[1];
		if(!numBlobs)
		{
			eiWarn( "Blob load failed for asset %s", context->dbgName );
			Self::GetDefaultBlob( (data=defaultData)[0], (size=defaultSize)[0] );
			numBlobs = 1;
		}

		Self* self = (Self*)context->factory;
		AssetStorage* asset = context->asset;
		if( asset->Loaded() )
			self->Release( asset->Data() );
		self->Acquire( numBlobs, data, size );
		asset->Assign( 0, data[0], size[0] );
		//only the address of blob 0 is stored in the asset, 
		// if other blob addresses are required for cleanup, 
		// they should be written into data[0]'s allocation during acquisition.
	}
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------