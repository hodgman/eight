#include <eight/core/blob/types.h>
#include <eight/core/blob/asset.h>
#include <eight/core/blob/loader_win.h>
#include <eight/core/thread/taskloop.h>
#include <stdio.h>

using namespace eight;

//------------------------------------------------------------------------------

BlobLoaderWin32::BlobLoaderWin32(Scope& a, const BlobConfig& c, const TaskLoop& l)
	: m_loop(l)
	, m_queue( a, c.maxRequests )
	, m_loads( a, c.maxRequests )
	, m_pendingLoads()
	, m_basePath(c.path)
	, m_osUpdateTask( eiNew(a,ThreadGroup)() )
{
}

bool BlobLoaderWin32::EnqueueLoad(const AssetName& name, const Request& req)//call at any time from any thread
{
	uint frame = m_loop.Frame();
	QueueItem item = { name, req };
	return m_queue.Push( frame, item );
}

void BlobLoaderWin32::Update()
{
	eiBeginSectionThread(*m_osUpdateTask)//should be executed by one thread only, the same OS thread each time.
	{
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
		Lambda(BlobLoaderWin32& self) : self(self) {} BlobLoaderWin32& self;
		bool operator()( LoadItem& i )
		{
			return self.UpdateLoad( i );
		}
	} updateLoad( *this );
	m_loads.ForEach( updateLoad );
}

bool BlobLoaderWin32::StartLoad(QueueItem& q)
{
	LoadItem* item = m_loads.Allocate();
	if( !item )
		return false;//failed to allocate space in the internal buffer, leave this item in its queue

	const AssetName& name = q.name;
	Request& req = q.request;
	item->request = req;

	char buf[2048];
	const char* strName = name.String();
	int baseLen = strlen(m_basePath);
	int nameLen = strlen(strName);
	if( baseLen + nameLen + 1 >= 2048 ) { eiASSERT(false); return false; }
	memcpy(buf, m_basePath, baseLen);
	memcpy(buf+baseLen, strName, nameLen);
	buf[baseLen+nameLen] = '\0';
	const char* path = buf;
	eiInfo(BlobLoader, "loading %s\n", path);
	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if(file == INVALID_HANDLE_VALUE)
	{
		goto loadFailure;
	}

/*	for( int i=0, end=m_queue.Capacity(); ; ++i )
	{
		if( i == end )
			return false;//failed to allocate space in the internal buffer, leave this item in its queue
		eiASSERT( i < end );
		item = &m_loads[i];
		if( item->inUse == false )
		{
			item->inUse = true;
			break;
		}
	}*/

	DWORD fileSize = 0, fileSizeHi = 0;
	fileSize = GetFileSize(file, &fileSizeHi);
	eiASSERT( fileSizeHi == 0 );
	eiInfo(BlobLoader, "size %d\n", fileSize);
	if( fileSize == INVALID_FILE_SIZE )
	{
		DWORD error = GetLastError();
		void* errText = 0;
		DWORD errLength = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errText, 0, 0);
		LocalFree(errText); 
		goto loadFailure;
	}

	eiASSERT( item->pass == Measure );
	item->file = file;
	item->size = fileSize;
	item->pass = Alloc;
	return true;

loadFailure:
	item->buffer = 0;
	item->file = 0;
	item->size = 0;
	item->pass = Parse;
	return true;
}
bool BlobLoaderWin32::UpdateLoad(LoadItem& item)
{
	switch( item.pass )
	{
		case Alloc:
		eiBeginSectionThread( item.request.onAllocate );
		{
			u8 hint[1] = { 0 };//TODO - assets that create multiple blobs.
			u32 size[1] = { item.size };
			void* buffer = item.request.pfnAllocate(1, size, hint, &item.request.userData);
			eiDEBUG( memset(buffer, 0xf0f0f0f0, item.size) );
			eiInfo(BlobLoader, "buffer %x\n", buffer);
			item.buffer = buffer;
			if( buffer )
				item.pass = Load;
			else
				item.pass = Parse;
		}
		eiEndSection( item.request.onAllocate );
		break;

		case Load:
		eiBeginSectionThread( *m_osUpdateTask )
		{
			item.overlapped.Pointer = 0;
			item.overlapped.Internal = item.overlapped.InternalHigh = 0;
			item.overlapped.hEvent = this;
			BOOL ret = ReadFileEx(item.file, item.buffer, item.size, &item.overlapped, &BlobLoaderWin32::OnComplete);
			eiASSERT( ret != ERROR_HANDLE_EOF );
			if( !ret )
			{
				DWORD error = GetLastError();
				void* errText = 0;
				DWORD errLength = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errText, 0, 0);
				eiWarn("%s, %x, %x, %x, %x\n", (char*)errText, item.buffer, item.size, &item.overlapped, item.overlapped);
				LocalFree(errText); 
			}
			else
			{
				++m_pendingLoads;
			}
			item.pass = Parse;
		}
		eiEndSection( *m_osUpdateTask );
		break;
		
		case Parse:
		eiBeginSectionThread( item.request.onComplete );
		{
			bool done = false;
			if( !item.buffer )
			{
				item.request.pfnComplete(0, 0, 0, &item.request.userData);
				done = true;
			}
			else if( !item.overlapped.hEvent )
			{
				u8* data = (u8*)item.buffer;
				u32 size = item.size;
				item.request.pfnComplete(1, &data, &size, &item.request.userData);
				done = true;
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

VOID WINAPI BlobLoaderWin32::OnComplete(DWORD errorCode, DWORD numberOfBytesTransfered, OVERLAPPED* overlapped)
{
	BlobLoaderWin32* self = (BlobLoaderWin32*)overlapped->hEvent;
	eiASSERT( self );
	eiASSERT( &self->dbg_updating && self->dbg_updating != 0 );
	overlapped->hEvent = 0;
	--self->m_pendingLoads;
}

//------------------------------------------------------------------------------
bool BlobLoader::Load(const AssetName& n, const Request& req)
{
	return ((BlobLoaderWin32*)this)->EnqueueLoad(n, req);
}
void BlobLoader::Update()
{
	((BlobLoaderWin32*)this)->Update();
}

//------------------------------------------------------------------------------
