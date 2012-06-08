//------------------------------------------------------------------------------
#pragma once
#include "loader.h"
#include "stdlib.h"
#include <eight/core/debug.h>
#include <eight/core/noncopyable.h>
namespace eight {
template<class T> struct NativeHash;
//------------------------------------------------------------------------------

class TypeInfo {};//TODO

#define eiASSET_REFRESH

#define eiASSERT_CAST(Base, Derived) (void)static_cast<Base*>((Derived*)0)

#define eiTYPE_OF(t) TypeInfo()

struct Asset;
struct AssetStorage;
struct RefreshHeap;
struct AssetRefreshContext
{
	AssetStorage* asset;
	RefreshHeap* heap;
};

struct RefreshHeap
{
	static void* OnAllocate(uint numBlobs, u32 size[], u8  blobHint[], BlobLoadContext* context);
	static void OnFree(u8* alloc);
};

struct AssetRefreshMemory
{
	bool inUse ;
	u8* data;
	u32 size;
//	void (*free)( u8* );
};

struct Handle
{
	union
	{
		void* ptr;
		uint  id;
	};
	bool operator!() const { return !ptr; }
	bool operator!=(void* v) const { return ptr != v; }
};

//TODO
class AssetName { const char* name; public: AssetName(const char* n=0):name(n){} const char* String() const { return name; } bool operator==(const AssetName&o)const{return 0==strcmp(name,o.name);} friend struct NativeHash<AssetName>; };

struct Asset : NonCopyable
{
public:
	bool Loaded() const { return !!m_handle; }
protected:
	uint Id() const { return m_handle.id; }
	u8* Data() { eiASSERT(m_handle); return (u8*)m_handle.ptr; }
//	template<class T> T* Data()       { eiASSERT(m_handle); return (T*)m_handle.ptr; }
	Handle m_handle;
};

struct AssetStorage : public Asset
{
public:
	void Assign(uint id, u8* data, u32 size);
	u8* Data() { return Asset::Data(); }
#if defined(eiASSET_REFRESH)
	void FreeRefreshData();
	template<class T> void Refresh( AssetScope& s, AssetName name, BlobLoader& blobs, T& factory, RefreshHeap& a )
	{//todo - Take out a lock to prevent two concurrent loads, return it when the load is done.
	 //     - Allow continuous calling, return whether the loaded asset matches 'name', and try to issue a load command if returning false.
		eiASSERT( !m_heap || m_heap == &a );
		//m_refresh.free = &RefreshHeap::OnFree;
		m_heap = &a;
		BlobLoader::Request req;
		BlobLoadContext ctxData  = { &s, this, &factory, eiDEBUG(name.String()) };
		m_ctxAlloc.asset = this;
		m_ctxAlloc.heap = &a;
		req.userData = ctxData;
		req.pfnComplete = &T::OnBlobLoaded;
		req.pfnAllocate = &RefreshHeap::OnAllocate;
		blobs.Load( name, req );
	}
private:
	friend struct RefreshHeap;
	AssetRefreshMemory m_original;
	AssetRefreshMemory m_refresh;
	AssetRefreshContext m_ctxAlloc;
	RefreshHeap* m_heap;
#endif
};

//------------------------------------------------------------------------------
template<> struct NativeHash<AssetName> { static u32 Hash(const AssetName& n) { return *(u32*)(&n.name); } typedef u32 type; };
}
//------------------------------------------------------------------------------