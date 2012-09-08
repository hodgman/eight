#include <eight/core/blob/types.h>
#include <eight/core/blob/asset.h>
#include <eight/core/blob/loader_win.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/thread/taskloop.h>
#include <eight/core/throw.h>
#include <stdio.h>

using namespace eight;

//------------------------------------------------------------------------------

BlobLoaderDevWin32::BlobLoaderDevWin32(Scope& a, const BlobConfig& c, const TaskLoop& l)
	: m_loop( l )
	, m_manifest()
	, m_queue( a, c.maxRequests, l.MaxConcurrentFrames() )
	, m_loads( a, c.maxRequests )
	, m_pendingLoads()
	, m_basePath( c.path )
	, m_baseLength( strlen(c.path) )
	, m_osUpdateTask( eiNew(a,ThreadGroup)() )
	, m_manifestFile()
	, m_manifestLoad()
	, m_manifestSize()
{
	char buf[MAX_PATH];
	const char* path = FullPath( buf, MAX_PATH, c.manifestFile, strlen(c.manifestFile) );
	if( !path )
		return;
	
	eiInfo(BlobLoader, "loading %s\n", path);
	m_manifestFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if(m_manifestFile == INVALID_HANDLE_VALUE)
		return;

	DWORD fileSize = 0, fileSizeHi = 0;
	fileSize = GetFileSize(m_manifestFile, &fileSizeHi);
	eiASSERT( fileSizeHi == 0 );
	eiInfo(BlobLoader, "size %d\n", fileSize);
	if( fileSize == INVALID_FILE_SIZE )
	{
		DWORD error = GetLastError();
		void* errText = 0;
		DWORD errLength = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errText, 0, 0);
		LocalFree(errText); 
		return;
	}

	eiASSERT( fileSize >= sizeof(AssetManifestDevWin32) );
	m_manifest = (AssetManifestDevWin32*)Malloc(fileSize);
	m_manifestSize = fileSize;
}

BlobLoaderDevWin32::~BlobLoaderDevWin32()
{
	if( m_manifestLoad )
		Free( m_manifestLoad );
	if( m_manifest )
		Free( m_manifest );
	if( m_manifestFile )
	CloseHandle( m_manifestFile );
}

BlobLoader::States BlobLoaderDevWin32::Prepare()
{
	if( !m_manifest )
		return BlobLoader::Error;
	if( !m_manifestFile )
	{
		eiASSERT( !m_manifestLoad );
		return BlobLoader::Ready;
	}
	if( !m_manifestLoad && m_manifestFile )
	{
		m_manifestLoad = (OVERLAPPED*)Malloc(sizeof(OVERLAPPED));
		{
			OVERLAPPED overlapped = {};
			overlapped.hEvent = this;
			*m_manifestLoad = overlapped;
		}
		eiASSERT( m_manifestSize >= sizeof(AssetManifestDevWin32) );
		BOOL ret = ReadFileEx(m_manifestFile, m_manifest, m_manifestSize, m_manifestLoad, &BlobLoaderDevWin32::OnManifestComplete);
		eiASSERT( ret != ERROR_HANDLE_EOF );
		if( !ret )
		{
			DWORD error = GetLastError();
			void* errText = 0;
			DWORD errLength = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errText, 0, 0);
			eiWarn("%s, %x, %x, %x, %x\n", (char*)errText, m_manifest, m_manifestSize, m_manifestLoad, m_manifestLoad);
			LocalFree(errText); 
			Free(m_manifestLoad);
			Free(m_manifest);
			CloseHandle( m_manifestFile );
			m_manifestLoad = 0;
			m_manifest = 0;
			m_manifestFile = 0;
			return BlobLoader::Error;
		}
	}
	SleepEx(0, TRUE);
	if( m_manifestLoad && !m_manifestLoad->hEvent )
	{
		Free(m_manifestLoad);
		CloseHandle( m_manifestFile );
		m_manifestLoad = 0;
		m_manifestFile = 0;
		return BlobLoader::Ready;
	}
	return BlobLoader::Initializing;
}
VOID WINAPI BlobLoaderDevWin32::OnManifestComplete(DWORD errorCode, DWORD numberOfBytesTransfered, OVERLAPPED* overlapped)
{
	eiASSERT( overlapped->hEvent );
	overlapped->hEvent = 0;
}

bool BlobLoaderDevWin32::EnqueueLoad(const AssetName& name, const Request& req)//call at any time from any thread
{
	uint frame = m_loop.Frame();
	QueueItem item = { name, req };
	return m_queue.Push( frame, item );
}

void BlobLoaderDevWin32::Update(uint worker)
{
	eiBeginSectionThread(*m_osUpdateTask)//should be executed by one thread only, the same OS thread each time.
	{
		eiASSERT( m_manifest && !m_manifestFile && !m_manifestLoad );
		eiASSERT( dbg_updating.SetIfEqual(1, 0) );

		uint frame = m_loop.Frame();
		QueueItem* item = 0;
		while( m_queue.Peek(frame, item) )
		{
			if( StartLoad(*item) )
				m_queue.Pop(frame);
			else
				break;//internal buffer m_loads is full, leave the rest of the queue alone for now (it will be cycled around again in 4 frames time)
		}

		if( m_pendingLoads )
		{
			bool loadedSomething = ( 0 != SleepEx(0, TRUE) );//trigger OnComplete calls

			if(loadedSomething && true)
			{
			}
		}
		
		eiASSERT( dbg_updating.SetIfEqual(0, 1) );
	}
	eiEndSection(*m_osUpdateTask);

	//TODO - only loop on required subset
//	for( int i=0, end=m_queue.Capacity(); i != end; ++i )
//		UpdateLoad( m_loads[i] );

	struct Lambda
	{
		Lambda(BlobLoaderDevWin32& self, uint worker) : self(self), worker(worker) {} BlobLoaderDevWin32& self; uint worker;
		bool operator()( LoadItem& i )
		{
			return self.UpdateLoad( i, worker );
		}
	} updateLoad( *this, worker );
	m_loads.ForEach( updateLoad );
}

const char* BlobLoaderDevWin32::FullPath( char* buf, int bufSize, const char* name, int nameLen )
{
	if( m_baseLength + nameLen + 1 > bufSize ) { eiASSERT(false); return 0; }
	buf[m_baseLength+nameLen] = '\0';
	memcpy(buf, m_basePath, m_baseLength);
	memcpy(buf+m_baseLength, name, nameLen);
	return buf;
}

bool BlobLoaderDevWin32::StartLoad(QueueItem& q)
{
	LoadItem* item = m_loads.Allocate();
	if( !item )
		return false;//failed to allocate space in the internal buffer, leave this item in its queue

	item->request = q.request;

	AssetManifestDevWin32::AssetInfo info = m_manifest->GetInfo(q.name);
	if( !info.numBlobs )
		goto loadFailure;
	char buf[MAX_PATH];
	const char* path = FullPath( buf, MAX_PATH, &info.fileName->chars, info.fileName->length );
	if( !path )
		goto loadFailure;
	
	eiInfo(BlobLoader, "loading %s\n", path);
	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if(file == INVALID_HANDLE_VALUE)
		goto loadFailure;

	eiASSERT( item->pass == Measure );
	eiASSERT( item->numAllocated == 0 );
	eiASSERT( info.numBlobs <= LoadItem::MAX_BLOBS );
	for( uint i=0, end=info.numBlobs; i!=end; ++i )
		item->sizes[i] = info.blobSizes[i];
	item->numBuffers = info.numBlobs;
	item->file = file;
	item->pass = Alloc;
	return true;

loadFailure:
	item->numBuffers = 0;
	item->file = 0;
	item->pass = Parse;
	return true;
}
bool BlobLoaderDevWin32::UpdateLoad(LoadItem& item, uint worker)
{
	switch( item.pass )
	{
	case Alloc:
		{
			uint numBuffers = item.numBuffers;
			for( uint i=0; i!=numBuffers; ++i )
			{
				if( !item.buffers[i] )
				{
					void* buffer = item.request.pfnAllocate(numBuffers, i, item.sizes[i], &item.request.userData, worker);
					if( buffer )
					{
						if( buffer == (void*)0xFFFFFFFF )
							buffer = 0;
						eiDEBUG( if(buffer) memset(buffer, 0xf0f0f0f0, item.sizes[i]) );
						eiInfo(BlobLoader, "buffer %x\n", buffer);
						eiASSERT( !item.buffers[i] );
						item.buffers[i] = buffer;
						uint numAllocated = (uint)++item.numAllocated;
						eiASSERT( numAllocated <= numBuffers );
						bool finalAlloc = (numBuffers == numAllocated);
						if( finalAlloc )
						{
							bool anyBuffers = !!buffer;
							for( uint j=0; j!=numBuffers && !anyBuffers; ++j )
								anyBuffers = !!item.buffers[j];
							if( anyBuffers )
								item.pass = Load;
							else
								item.pass = Parse;
							break;
						}
					}
				}
			}
		}
		break;

	case Load:
		eiBeginSectionThread( *m_osUpdateTask )
		{
			eiASSERT( item.numBuffers );

			uint offset = 0;
			for( uint i=0, end=item.numBuffers; i!=end; ++i )
			{
				OVERLAPPED o = {};
				item.overlapped[i] = o;
				item.overlapped[i].Offset = offset;
				item.overlapped[i].hEvent = this;
				offset += item.sizes[i];
				BOOL ret = ReadFileEx(item.file, item.buffers[i], item.sizes[i], &item.overlapped[i], &BlobLoaderDevWin32::OnComplete);
				eiASSERT( ret != ERROR_HANDLE_EOF );
				if( !ret )
				{
					DWORD error = GetLastError();
					void* errText = 0;
					DWORD errLength = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errText, 0, 0);
					eiWarn("%s, %x, %x, %x, %x", (char*)errText, item.buffers[i], item.sizes[i], &item.overlapped[i], item.overlapped[i]);
					LocalFree(errText);
					item.overlapped[i].hEvent = 0;
				}
				else
				{
					++m_pendingLoads;
				}
			}
			item.pass = Parse;
		}
		eiEndSection( *m_osUpdateTask );
		break;
		
	case Parse:
		eiBeginSectionThread( item.request.onComplete );
		{
			bool done = false;
			if( !item.numBuffers )
			{
				eiASSERT( !item.file );
				item.request.pfnComplete(0, 0, 0, &item.request.userData);
				done = true;
			}
			else 
			{
				uint buffersDone = 0;
				for( uint i=0, end=item.numBuffers; i!=end; ++i )
				{
					if( !item.overlapped[i].hEvent )
						++buffersDone;
				}
				if( buffersDone == item.numBuffers )
				{
					CloseHandle( item.file );
					item.request.pfnComplete(item.numBuffers, (u8**)item.buffers, (u32*)item.sizes, &item.request.userData);
					done = true;
				}
			}
			if( done )
			{
				LoadItem blank = {};
				item = blank;
				return true;//release the item
			}
		}
		eiEndSection( item.request.onComplete );
		break;
	}//end switch
	return false;//don't deallocate
}

VOID WINAPI BlobLoaderDevWin32::OnComplete(DWORD errorCode, DWORD numberOfBytesTransfered, OVERLAPPED* overlapped)
{
	BlobLoaderDevWin32* self = (BlobLoaderDevWin32*)overlapped->hEvent;
	eiASSERT( self );
	eiASSERT( &self->dbg_updating && self->dbg_updating != 0 );
	eiAssertInTaskSection( self->m_osUpdateTask );
	overlapped->hEvent = 0;
	--self->m_pendingLoads;
}

//------------------------------------------------------------------------------
#include <algorithm>
template <class I, class T>
I BinarySearch( I first, I last, const T& value )
{
	first = std::lower_bound( first, last, value );
	return (first!=last && !(value<*first)) ? first : last;
}

AssetManifestDevWin32::AssetInfo AssetManifestDevWin32::GetInfo(const AssetName& name)
{
	AssetInfo result = {};
	u32* const begin = names.Begin();
	u32* const end = names.End();
	u32* find = BinarySearch( begin, end, name.hash );
	eiASSERT( find >= begin && find <= end );
	if( find != end )
	{
		int index = find - begin;
		AssetInfo_& info = *Info()[index];
		uint numBlobs = NumBlobs()[index];
		eiASSERT( numBlobs );
		result.numBlobs = numBlobs;
		result.blobSizes = info.blobSizes.Begin();
		result.fileName = &info.FileName(result.numBlobs);
	}
	return result;
}

//------------------------------------------------------------------------------
bool BlobLoader::Load(const AssetName& n, const Request& req)
{
	return ((BlobLoaderDevWin32*)this)->EnqueueLoad(n, req);
}
void BlobLoader::Update(uint worker)
{
	((BlobLoaderDevWin32*)this)->Update(worker);
}
BlobLoader::States BlobLoader::Prepare()
{
	return ((BlobLoaderDevWin32*)this)->Prepare();
}

//------------------------------------------------------------------------------
