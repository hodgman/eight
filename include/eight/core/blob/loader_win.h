//------------------------------------------------------------------------------
#pragma once
#include <eight/core/thread/latentpipe.h>
#include <eight/core/os/win32.h>
#include <eight/core/blob/loader.h>
#include <eight/core/thread/atomic.h>
#include <eight/core/thread/fifo_mpmc.h>
#include <eight/core/alloc/new.h>

//------------------------------------------------------------------------------
namespace eight {
	class TaskSection;
	class TaskLoop;
//------------------------------------------------------------------------------

struct InactiveArrayBase
{
	static uint NumMasks(uint size)
	{
		uint totalMasks = 0;
		do
		{
			totalMasks += size;
			size = (size+63)/64;
		} while( size > 1 );//if there were more than 64 items, need another layer
		return totalMasks;
	}
	static void Enable(uint index, uint size, u64* masks)
	{
		eiDEBUG( void* begin = masks COMMA *end = masks+NumMasks(size) );
		eiASSERT( size );
		do
		{
			eiASSERT( index < size );
			int off = index / 64;
			int bit = index % 64;
			bool preEnabled = (masks[off] != 0);
			eiASSERT( (masks[off] & (u64(1)<<bit)) == 0 );
			eiASSERT( &masks[off] >= begin && &masks[off] < end );
			masks[off] |= u64(1)<<bit;
			if( preEnabled )
				return;
			index = off;
			masks += size;
			size = (size+63)/64;
		} while( size > 1 );
	}
	static void Disable(uint index, uint size, u64* masks)
	{
		eiDEBUG( void* begin = masks COMMA *end = masks+NumMasks(size) );
		eiASSERT( size );
		do
		{
			eiASSERT( index < size );
			int off = index / 64;
			int bit = index % 64;
			eiASSERT( (masks[off] & (u64(1)<<bit)) != 0 );
			eiASSERT( &masks[off] >= begin && &masks[off] < end );
			masks[off] &= ~(u64(1)<<bit);
			if( masks[off] != 0 )
				return;
			index = off;
			masks += size;
			size = (size+63)/64;
		} while( size > 1 );
	}
	static bool Enabled(uint index, uint size, u64* masks)
	{
		eiDEBUG( void* begin = masks COMMA *end = masks+NumMasks(size) );
		eiASSERT( index < size );
		int off = index / 64;
		int bit = index % 64;
		eiASSERT( &masks[off] >= begin && &masks[off] < end );
		return (masks[off] & bit) != 0;
	}
	template<class T, class Fn>
	static void ForEach( Fn& fn, uint size, u64* masks, T* data )
	{
		eiDEBUG( void* begin = masks COMMA *end = masks+NumMasks(size) );
		if( !size )
			return;
		uint numLayers = (size==1)?1:0;
		for( uint s=size; s > 1; s = (s+63)/64 )
			++numLayers;
		const uint maxLayers = 5;
		eiASSERT( numLayers <= maxLayers );
		u64* layers[maxLayers];
		for( uint i=0, o=0, s=size; i == 0 || s > 1; ++i, o+=s, s = (s+63)/64 )
			layers[i] = &masks[o];
		eiASSERT( numLayers == 1 || ((u8*)layers[1])-((u8*)layers[0])==size*8 );

		eiASSERT( numLayers );
		struct Iterator
		{
			uint offs;
			uint layer;
			Iterator* next;
		};
		uint stackUsage=1;
		const uint maxStackUsage = 1+(maxLayers-1)*64;
		Iterator stack[maxStackUsage] = { { 0, numLayers-1, NULL }, };
		for( Iterator* i=stack; i; i=i->next )
		{
			eiASSERT( &layers[i->layer][i->offs] >= begin && &layers[i->layer][i->offs] < end );
			for( u64 mask = layers[i->layer][i->offs]; mask; mask &= mask-1 )
			{
				uint offset = LeastSignificantBit(mask)
				             + i->offs*64;
				if( i->layer )
				{
					Iterator more = { offset, i->layer-1, i->next };
					eiASSERT( stackUsage < maxStackUsage );
					stack[stackUsage] = more;
					i->next = &stack[stackUsage];
					++stackUsage;
				}
				else
				{
					eiASSERT( offset < size );
					if( fn( data[offset] ) )
					{
						Disable( offset, size, masks );
					}
				}
			}
		}
	}
	static int Allocate(uint size, u64* masks)
	{
		if( !size )
			return -1;
		for( uint i=0, end=(size+63)/64; i != end; ++i )
		{
			u64 mask = masks[i];
			if( ~mask )//any bits off
			{
				uint index = LeastSignificantBit(~mask)
				           + 64*i;
				Enable(index, size, masks);
				return (int)index;
			}
		}
		return -1;
	}
};

class InactiveArrayMask// : protected InactiveArrayBase
{
public:
	InactiveArrayMask( Scope& a, uint size )
		: masks( eiAllocArray(a, u64, InactiveArrayBase::NumMasks(size)) )
	{
		memset( masks, 0, sizeof(u64)*InactiveArrayBase::NumMasks(size) );
	}
	void Enable ( uint i, uint size )       { InactiveArrayBase::Enable (i, size, masks); }
	void Disable( uint i, uint size )       { InactiveArrayBase::Disable(i, size, masks); }
	bool Enabled( uint i, uint size ) const { return InactiveArrayBase::Enabled(i, size, masks); }
	template<class T, class Fn>
	void ForEach( Fn& fn, uint size, T* data ) { InactiveArrayBase::ForEach( fn, size, masks, data ); }
	int Allocate(uint size) 
	{
		int free = InactiveArrayBase::Allocate(size, masks);
		eiASSERT( free < (int)size );
		return free;
	}
private:
	u64* masks;
};
template<class T> class InactiveArray : protected InactiveArrayMask
{
public:
	InactiveArray( Scope& a, uint size )
		: InactiveArrayMask( a, size )
		, size(size)
		, data( eiAllocArray(a, T, size) )
	{
		memset( data,  0, sizeof(T)*size );
	}
	void Enable ( uint i )       { InactiveArrayMask::Enable (i, size); }
	void Disable( uint i )       { InactiveArrayMask::Disable(i, size); }
	bool Enabled( uint i ) const { return InactiveArrayMask::Enabled(i, size); }
	template<class Fn>
	void ForEach( Fn& fn ) { InactiveArrayMask::ForEach( fn, size, data ); }
	T* Allocate()
	{
		int free = InactiveArrayMask::Allocate(size);
		return free < 0 ? 0 : &data[free];
	}
private:
	uint size;
	  T* data;
};

class AssetManifestDevWin32
{
public:
	struct AssetInfo
	{
		uint numBlobs;
		u32* blobSizes;
		String* fileName;
	};
	AssetInfo GetInfo(const AssetName&);
private:
	struct AssetInfo_
	{
		Array<u32> blobSizes;
		String& FileName(u8 numBlobs) { return *(String*)(blobSizes.End(numBlobs)); }
	};
	List<u32>                 names;
	Array<u8>&                NumBlobs() { return *(Array<u8>*)names.End(); };
	Array<Offset<AssetInfo_>>& Info() { return *(Array<Offset<AssetInfo_>>*)NumBlobs().End(names.count); };
};

class BlobLoaderDevWin32 : public NonCopyable
{
public:
	BlobLoaderDevWin32(Scope&, const BlobConfig&, const TaskLoop&);
	~BlobLoaderDevWin32();

	BlobLoader::States Prepare();
	bool Load(const AssetName& name, const BlobLoader::Request& req);//call at any time from any thread
	void Update(uint worker);//should be called by all threads
	void ImmediateDevLoad(const char* path, const BlobLoader::ImmediateDevRequest&);

private:
	struct Pass { enum Type
	{
		Measure = 0,
		Alloc,
		Load,
		Parse,
	};};
	struct LoadItem
	{
		const static int MAX_BLOBS = 3;
		OVERLAPPED overlapped[MAX_BLOBS];
		BlobLoader::Request request;
		HANDLE file;
		Atomic pass;
		Atomic numAllocated;
		int numBuffers;
		DWORD sizes[MAX_BLOBS];
		void* buffers[MAX_BLOBS];
	};
	struct QueueItem
	{
		AssetName name;
		BlobLoader::Request request;
	};

	const char* FullPath( char* buf, int bufSize, const char* name, int nameLen );
	const char* FullDevPath( char* buf, int bufSize, const char* name, int nameLen );
	bool StartLoad(QueueItem& name);
	bool UpdateLoad(LoadItem& item, uint worker);
	static VOID WINAPI OnComplete(DWORD errorCode, DWORD numberOfBytesTransfered, OVERLAPPED* overlapped);
	static VOID WINAPI OnManifestComplete(DWORD errorCode, DWORD numberOfBytesTransfered, OVERLAPPED* overlapped);

	const TaskLoop& m_loop;
	AssetManifestDevWin32* m_manifest;
	LatentMpsc<QueueItem> m_queue;
	InactiveArray<LoadItem> m_loads;
	int m_pendingLoads;
	const char* const m_baseDevPath;
	const char* const m_basePath;
	const int m_baseLength;
	SingleThread& m_osUpdateTask;
	HANDLE m_manifestFile;
	OVERLAPPED* m_manifestLoad;
	uint m_manifestSize;
	eiDEBUG( Atomic dbg_updating; )
};

//------------------------------------------------------------------------------
eiNoDestructor(BlobLoaderDevWin32::QueueItem);
}
//------------------------------------------------------------------------------