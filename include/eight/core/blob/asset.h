//------------------------------------------------------------------------------
#pragma once
#include <eight/core/thread/futex.h>
#include <eight/core/thread/tasksection.h>
#include <rdestl/vector.h>
namespace eight {
	template<class T> struct NativeHash;
	struct AssetStorage;
//------------------------------------------------------------------------------

#define eiASSET_REFRESH //TODO always define as 0 or 1

struct Handle
{
	Handle(void*p) : ptr(p) {}
	Handle(uint i) : id(i) {}
	union
	{
		void* ptr;
		uint  id;
	};
	bool operator!() const { return !ptr; }
	bool operator!=(void* v) const { return ptr != v; }
	bool operator==(uint  i) const { return id  == i; }
};

struct AssetName
{
	u32 hash;
	eiDEBUG( const char* dbgName; )

	typedef u32 HashType;
	explicit AssetName(u32 name) : hash(name) { eiDEBUG( dbgName = "?"; ) }
	AssetName(const char* n=0);
	AssetName(u32 name, const char* strName);
	bool operator==(const AssetName& o) const { return hash == o.hash; }
	friend struct NativeHash<AssetName>;
};

inline bool operator<(const AssetName& lhs, const AssetName& rhs)
{
	return lhs.hash < rhs.hash;
}

struct Asset : NonCopyable
{
//public:
//	bool Loaded() const { return !!m_handle; }
protected:
	uint Id() const { return m_handle.id; }
	u8* Data() { return (u8*)m_handle.ptr; }
//	template<class T> T* Data()       { eiASSERT(m_handle); return (T*)m_handle.ptr; }
	Handle m_handle;
#if defined(eiASSET_REFRESH)
	Asset() : m_handle((void*)0) {}
#endif
};

#if defined(eiASSET_REFRESH)
class AssetRefreshCallback : NonCopyable
{
public:
	AssetRefreshCallback(callback, void*, TaskSection);
	~AssetRefreshCallback();
private:
	callback fn;
	void* arg;
	TaskSection fnTask;
	Futex m_dependencyLock;
	rde::vector<AssetStorage*> thisDependsOn;
	friend void AssetDependency(AssetRefreshCallback&, Asset& used);
	friend class AssetRootImpl;
};
       void AssetDependency(Asset& user, Asset& used);
       void AssetDependency(AssetRefreshCallback&, Asset& used);
#else
struct AssetRefreshCallback : NonCopyable
{
	AssetRefreshCallback(callback, void*, TaskSection){}
};
inline void AssetDependency(Asset& user, Asset& used){}
inline void AssetDependency(AssetRefreshCallback&, Asset& used){}
#endif


struct AssetStorage : public Asset
{
public:
	void Assign(Handle);//(uint id, u8* data, u32 size);
	operator const Handle&() const { return m_handle; }
	u8* Data() { return Asset::Data(); }
private:
	friend class AssetScope;
	void Construct(const AssetName& n);
	AssetStorage* m_next;
#if defined(eiASSET_REFRESH)
	AssetName m_name;
	struct AllocHeader { AllocHeader* next; };
	friend class AssetRefreshCallback;
	friend class AssetRootImpl;
	friend void AssetDependency(Asset&, Asset&);
	friend void AssetDependency(AssetRefreshCallback&, Asset&);
	AllocHeader* m_refreshAllocations;
	Futex m_dependencyLock;
	rde::vector<AssetStorage*> m_thisDependsOnAsset;
	rde::vector<AssetStorage*> m_assetDependsOnThis;
	rde::vector<AssetRefreshCallback*> m_userDependsOnThis;
	bool m_resolved;

	void* RefreshAlloc(uint size);
	void RefreshFree();
	void UnlinkFromUsedAssets();
	void Destruct();
#endif
};

//------------------------------------------------------------------------------
template<> struct NativeHash<AssetName> { static u32 Hash(const AssetName& n) { return n.hash; } typedef u32 type; };
}
//------------------------------------------------------------------------------