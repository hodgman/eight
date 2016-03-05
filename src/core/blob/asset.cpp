#include <eight/core/blob/types.h>
#include <eight/core/blob/loader.h>
#include <eight/core/blob/assetscope.h>
#include "eight/core/thread/fifo_mpmc.h"
#include "eight/core/thread/atomic.h"
#include "eight/core/hash.h"
#include "eight/core/alloc/scope.h"
#include "eight/core/thread/tasksection.h"
#include "eight/core/thread/pool.h"
#include "eight/core/thread/futex.h"
#include "eight/core/sort/search.h"
#include "eight/core/sort/radix.h"
#include "eight/core/alloc/malloc.h"
#include "rdestl/set.h"
#include <set>
#include <vector>

using namespace eight;

//------------------------------------------------------------------------------

AssetName::AssetName(const char* n) : hash(Fnv32a(n))
{
	eiDEBUG( dbgName = n; )
}
AssetName::AssetName(u32 name, const char* strName) : hash(name)
{
	eiDEBUG( dbgName = strName );
	eiASSERT( Fnv32a(strName) == name );
}

//------------------------------------------------------------------------------

//todo - move
template<class T>
class PodVector : NonCopyable
{
public:
	PodVector(Scope& a, uint capacity)
		: items(eiAllocArray(a,T,capacity))
		, size()
		, capacity(capacity)
	{}
	      T& operator[]( uint i )       { eiASSERT(i<size); return items[i]; }
	const T& operator[]( uint i ) const { eiASSERT(i<size); return items[i]; }
	      T* Begin()       { return items; }
	const T* Begin() const { return items; }
	      T* End()         { return items + size; }
	const T* End()   const { return items + size; }
	uint Size()      const { return size; }
	uint Capacity()  const { return capacity; }
	void PushBack( const T& item )
	{
		eiASSERT( size < capacity );
		items[size] = item;
		++size;
	}
	void RemoveUnordered( uint index )
	{
		eiASSERT( index < size );
		uint idxLast = size-1;
		const T& last = items[idxLast];
		T& toRemove = items[index];
		toRemove = last;
		size = idxLast;
	}
	int Find( const T& item ) const
	{
		const T* i = LinearSearch( Begin(), End(), item );
		return i == End() ? -1 : i - Begin();
	}
	template<class Y, class Fn>
	int Find( const Y& item, Fn& pred ) const
	{
		const T* i = LinearSearch( Begin(), End(), item, pred );
		return i == End() ? -1 : i - Begin();
	}
private:
	T* items;
	uint size;
	uint capacity;
};

namespace eight{
#ifdef eiASSET_REFRESH
const SingleThread& OsThread(BlobLoader& loader);
void ReloadManifest(BlobLoader& loader);
class AssetRootImpl
{
public:
	AssetRootImpl(Scope& a, BlobLoader& b, uint maxScopes)
		: m_blobs(b)
		, m_lock()
		, m_items( a, maxScopes )
	{
	}
	void Created(AssetScope& a, uint depth)
	{
		ScopeLock<Futex> lock(m_lock);
		Item item = { &a, depth };
		m_items.PushBack(item);
	}
	void Destroyed(AssetScope& a)
	{
		ScopeLock<Futex> lock(m_lock);
		struct { bool operator()( const Item& lhs, AssetScope* rhs )
		{
			return lhs.p == rhs;
		}} predicate;
		int index = m_items.Find(&a, predicate);
		eiASSERT( index >= 0 );
		m_items.RemoveUnordered( (uint)index );
	}
	struct RefreshState
	{
		RefreshState(const SingleThread& blobThread)
			: waitForReady(TaskSection())
			, reloadManifest(blobThread)
			, sortAndInitLoads(TaskSection(1, 1))//allocations done on thread #0
			, update(TaskSection())
		{}
		TaskSection waitForReady;
		TaskSection reloadManifest;
		TaskSection sortAndInitLoads;
		TaskSection update;
		Atomic finished;

		AssetStorage** refreshedAssets;
		uint refreshedAssetsCount;
	};
	void HandleRefreshInterrupt(Scope& a, AssetName* files, uint count, uint worker, uint numWorkers, AtomicPtr<void>& shared)
	{
		Scope temp(a, "temp");
		RefreshState* state = 0;
		if(worker == 0)
		{
			BlobLoader::States s;
			do
			{
				s = m_blobs.Prepare();
			} while ( s == BlobLoader::Initializing );
			eiASSERT( s != BlobLoader::Error );

			eiASSERT( shared == 0 );
			state = eiNew(temp, RefreshState)(OsThread(m_blobs));
			shared = state;
		}
		else
		{
			YieldThreadUntil( WaitForTrue(shared) );
			state = (RefreshState*)(void*)shared;
		}

		eiBeginSectionTask(state->waitForReady)
		{
			bool finished;
			do
			{
				m_blobs.Update(worker, true);
				finished = true;
				for( Item* item=m_items.Begin(), *endItem=m_items.End(); item!=endItem; ++item )
				{
					if( item->p->Update(worker) != AssetScope::Loaded )
						finished = false;
				}
			}while(!finished);
		}
		eiEndSection(state->waitForReady);
		eiWaitForSection(state->waitForReady);

		eiBeginSectionTask(state->reloadManifest)
		{
			ReloadManifest(m_blobs);
			BlobLoader::States s;
			do
			{
				s = m_blobs.Prepare();
			} while ( s == BlobLoader::Initializing );
			eiASSERT( s != BlobLoader::Error );
		}
		eiEndSection(state->reloadManifest);
		eiWaitForSection(state->reloadManifest);

		// Use a single thread to kick off the loading process
		eiBeginSectionTask(state->sortAndInitLoads)
		{
			//Asset scopes should be resolved after their parent scopes are resolved. Sort topologically.
			{
				Scope bufferAlloc(temp, "temp");
				struct 
				{
					typedef uint Type;
					uint operator()( Item* in ) const { return in->depth; }
				} key;
				Item* buffer = eiAllocArray(bufferAlloc, Item, m_items.Size());
				RadixSort( m_items.Begin(), buffer, m_items.Size(), key );
			}

			for( Item* item=m_items.Begin(), *endItem=m_items.End(); item!=endItem; ++item )
			{
				item->p->BeginAssetRefresh();
			}
			std::vector<AssetStorage*> refreshed;
			{
				Scope perPass(temp, "temp");
				std::set<AssetName> processed;
				for( uint i=0; i!=count; ++i )
				{
					processed.insert(files[i]);
				}
				AssetName* toRefresh = files;
				uint toRefreshCount = count;
				while(toRefreshCount)
				{
					std::set<AssetName> dependent;
					for( Item* item=m_items.Begin(), *endItem=m_items.End(); item!=endItem; ++item )
					{
						for( uint i=0; i!=toRefreshCount; ++i )
						{
							Scope perItem(perPass, "temp");
							uint numDepends = 0;
							AssetName* depends = 0;
							AssetStorage* asset = item->p->Refresh( toRefresh[i], m_blobs, perItem, &depends, &numDepends );
							eiASSERT( asset || (!numDepends && !depends) );
							if( asset )
							{
								refreshed.push_back( asset );
								for( uint j=0; j!=numDepends; ++j )
								{
									if( processed.find(depends[j]) == processed.end() )
									{
										dependent.insert(depends[j]);
									}
								}
							}
						}
					}
					if( dependent.empty() )
					{
						toRefreshCount = 0;
						toRefresh = 0;
					}
					else
					{
						perPass.Unwind();
						toRefreshCount = dependent.size();
						toRefresh = eiAllocArray(perPass, AssetName, toRefreshCount);
						uint i=0;
						for( std::set<AssetName>::iterator it = dependent.begin(), end = dependent.end(); it != end; ++it, ++i )
						{
							toRefresh[i] = *it;
							eiASSERT( processed.find(*it) == processed.end() );
							processed.insert(*it);
						}
						eiASSERT( i == dependent.size() );
					}
				}
			}
			for( Item* item=m_items.Begin(), *endItem=m_items.End(); item!=endItem; ++item )
			{
				item->p->EndAssetRefresh();
			}

			uint count = refreshed.size();
			state->refreshedAssetsCount = count;
			state->refreshedAssets = count ? eiAllocArray(temp, AssetStorage*, count) : 0;
			for( uint i=0, end=count; i != end; ++i )
			{
				state->refreshedAssets[i] = refreshed[i];
				rde::vector<AssetRefreshCallback*>& vec = refreshed[i]->m_userDependsOnThis;
				for( uint j=0, end=vec.size(); j!=end; ++j )
				{
					ResetTaskSection(vec[j]->fnTask);
				}
			}
		}
		eiEndSection(state->sortAndInitLoads);
		eiWaitForSection(state->sortAndInitLoads);

		// Load, parse and resolve the assets
		eiBeginSectionTask(state->update)
		{//todo - update until loadeded all items with depth 0, then depth 1, etc...
			bool finished;
			do
			{
				m_blobs.Update(worker, true);
				finished = true;
				for( Item* item=m_items.Begin(), *endItem=m_items.End(); item!=endItem; ++item )
				{
					if( item->p->Update(worker) != AssetScope::Loaded )
						finished = false;
				}
			}while(!finished);
		}
		eiEndSection(state->update);
		eiWaitForSection(state->update);

		//fire off user notification events
		for( uint i=0, end=state->refreshedAssetsCount; i!=end; ++i )
		{
			rde::vector<AssetRefreshCallback*>& vec = state->refreshedAssets[i]->m_userDependsOnThis;
			for( uint j=0, end=vec.size(); j!=end; ++j )
			{
				eiBeginSectionTask(vec[j]->fnTask);
				{
					eiASSERT(vec[j]->fn);
					vec[j]->fn( vec[j]->arg );
				}
				eiEndSection(vec[j]->fnTask);
			}
		}

		++state->finished;

		if(worker == 0)
		{//don't let the temp scope unwind until all threads are finished reading/writing to the shared data in it
			YieldThreadUntil( WaitForValue(state->finished, numWorkers) );
			shared = 0;
		}
	}

private:
	struct Item
	{
		AssetScope* p;
		uint depth;
	};
	BlobLoader& m_blobs;
	Futex m_lock;
	PodVector<Item> m_items;
};
#else
class AssetRootImpl
{
public:
	AssetRootImpl(Scope&, BlobLoader&, uint)  {}
	void Created(AssetScope& a, uint depth) { eiASSERT(false); }
	void Destroyed(AssetScope&) { eiASSERT(false); }
	void HandleRefreshInterrupt(Scope&, AssetName*, uint, uint, uint, AtomicPtr<void>&) { eiASSERT(false); }
};
#endif
}

eiImplementInterface(AssetRoot, AssetRootImpl);
eiInterfaceConstructor(AssetRoot, (a,b,c), Scope& a, BlobLoader& b, uint c);
eiInterfaceFunction(void, AssetRoot, Created, (a,b), AssetScope& a, uint b);
eiInterfaceFunction(void, AssetRoot, Destroyed, (a), AssetScope& a);
eiInterfaceFunction(void, AssetRoot, HandleRefreshInterrupt, (a,b,c,d,e,f), Scope& a, AssetName* b, uint c, uint d, uint e, AtomicPtr<void>& f);


#ifdef eiASSET_REFRESH
AssetRefreshCallback::AssetRefreshCallback(callback fn, void* arg, TaskSection task)
	: fn(fn)
	, arg(arg)
	, fnTask(task)
{}
AssetRefreshCallback::~AssetRefreshCallback()
{
	ScopeLock<Futex> la(m_dependencyLock);
	for( uint i=0, end=thisDependsOn.size(); i != end; ++i )
	{
		AssetStorage& used = *thisDependsOn[i];
		ScopeLock<Futex> lb(used.m_dependencyLock);
		rde::vector<AssetRefreshCallback*>::iterator it = used.m_userDependsOnThis.find(this);
		eiASSERT( it != used.m_userDependsOnThis.end() );
		used.m_userDependsOnThis.erase_unordered(it);
	}
	thisDependsOn.clear();
}

void eight::AssetDependency(Asset& a_user, Asset& a_used)
{
	AssetStorage& user = (AssetStorage&)a_user;
	AssetStorage& used = (AssetStorage&)a_used;
	ScopeLock<Futex> la(user.m_dependencyLock);
	ScopeLock<Futex> lb(used.m_dependencyLock);

	user.m_thisDependsOnAsset.push_back(&used);
	used.m_assetDependsOnThis.push_back(&user);
}
void eight::AssetDependency(AssetRefreshCallback& cb, Asset& a_used)
{
	AssetStorage& used = (AssetStorage&)a_used;
	ScopeLock<Futex> la(cb.m_dependencyLock);
	ScopeLock<Futex> lb(used.m_dependencyLock);
	
	cb.thisDependsOn.push_back(&used);
	used.m_userDependsOnThis.push_back(&cb);
}

void* AssetStorage::RefreshAlloc(uint size)
{
	u8* mem = (u8*)AlignedMalloc(size+16, 16);
	AllocHeader* header = (AllocHeader*)mem;
	header->next = m_refreshAllocations;
	m_refreshAllocations = header;
	return mem + 16;
}
void AssetStorage::RefreshFree()
{
	for( AllocHeader* header = m_refreshAllocations; header; )
	{
		AllocHeader* toFree = header;
		header = header->next;
		AlignedFree(toFree);
	}
	UnlinkFromUsedAssets();
}
void AssetStorage::UnlinkFromUsedAssets()
{
	ScopeLock<Futex> la(m_dependencyLock);//user = this
	for( uint i=0, end=m_thisDependsOnAsset.size(); i != end; ++i )
	{
		AssetStorage& used = *m_thisDependsOnAsset[i];
		ScopeLock<Futex> lb(used.m_dependencyLock);
		rde::vector<AssetStorage*>::iterator it = used.m_assetDependsOnThis.find(this);
		eiASSERT( it != used.m_assetDependsOnThis.end() );
		used.m_assetDependsOnThis.erase_unordered(it);
	}
	m_thisDependsOnAsset.clear();
}
void AssetStorage::Destruct()
{//other assets/users that depend on this should have a shorter lifetime than this. They should all be dead by now.
	eiASSERT(m_userDependsOnThis.empty());
	eiASSERT(m_assetDependsOnThis.empty());
	this->~AssetStorage();//free RDE's allocations
}
#endif

void AssetStorage::Construct(const AssetName& n)
{
#if defined(eiASSET_REFRESH)
	new(this) AssetStorage;
	m_handle.id = 0;
	m_name = n;
	m_next = 0;
	m_resolved = false;
	m_refreshAllocations = 0;
#else
	eiUNUSED(n);
	memset(this, 0, sizeof(AssetStorage));
#endif
}


void AssetStorage::Assign(Handle h)
{
	eiSTATIC_ASSERT( sizeof(s32) == sizeof(m_handle) );
	((Atomic&)m_handle.id) = (s32)h.id;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

AssetScope::AssetScope( Scope& a, const SingleThread& allocOwner, const ThreadMask& users, uint maxAssets, uint maxFactories, AssetRoot& root )
	: m_root(&root)
	, m_parent()
	, m_assets(a, maxAssets, maxAssets*7/10)
	, m_factories(a, maxFactories, maxFactories*7/10)
	, m_maxFactories(maxFactories)
	, m_a(a)
	, m_allocOwner(allocOwner)
	, m_userThreads(users)
	, m_loadsInProgress()
{
#if defined(eiASSET_REFRESH)
	m_root->Created(*this, Depth());
#endif
	//TODO - assert parent is null or closed
}
AssetScope::AssetScope( Scope& a, const SingleThread& allocOwner, const ThreadMask& users, uint maxAssets, uint maxFactories, AssetScope& parent )
	: m_root(parent.m_root)
	, m_parent(&parent)
	, m_assets(a, maxAssets, maxAssets*7/10)
	, m_factories(a, maxFactories, maxFactories*7/10)
	, m_maxFactories(maxFactories)
	, m_a(a)
	, m_allocOwner(allocOwner)
	, m_userThreads(users)
	, m_loadsInProgress()
{
#if defined(eiASSET_REFRESH)
	m_root->Created(*this, Depth());
#endif
	//TODO - assert parent is null or closed
}

uint AssetScope::Depth()
{
	uint depth = 0;
	for(AssetScope* parent = m_parent; parent; )
	{
		++depth;
		parent = parent->m_parent;
	}
	return depth;
}

void AssetScope::Close()
{
#if eiDEBUG_LEVEL > 0
		bool ok;
		do
		{
			s32 i = m_loadsInProgress;
			eiASSERT( (i & s_loadingLock) == 0 );
			eiASSERT( ((i + s_loadingLock) & (~s_loadingLock)) == i );
			eiASSERT( (i + s_loadingLock) == (i|s_loadingLock) );
			ok = m_loadsInProgress.SetIfEqual(i|s_loadingLock, i);
			if( i == 0 && ok )
			{
			//	m_a.Seal();
				ok = m_loadsInProgress.SetIfEqual(0xFFFFFFFF, s_loadingLock);
				eiASSERT( ok );
				break;
			}
		}while( !ok );
#else
		m_loadsInProgress += s_loadingLock;
		s32 loads = m_loadsInProgress;
		eiASSERT( loads & s_loadingLock );
		if( loads == s_loadingLock )
		{
		//	m_a.Seal();
			m_loadsInProgress = 0xFFFFFFFF;
		}
#endif
}

AssetScope::State AssetScope::Update(uint worker)
{
	eiASSERT( GetThreadId()->ThreadIndex() == worker );
	if( IsLoading() )
		return Loading;
	s32 resolved = m_resolved;
	s32 userMask = (s32)m_userThreads.GetMask();
	s32 thisMask = 1<<worker;
	eiASSERT(  userMask & thisMask );
	eiASSERT( (userMask & 0xFFFF) == userMask );
	uint pass = ((uint)resolved) >> 16;
	if( (resolved & 0xFFFF) == userMask )//all threads done
	{
		if( (int)pass >= m_maxResolveOrder )//more resolve passes required?
		{
			OnLoaded();
			return Loaded;//nope, all done
		}
		else//yep
		{
			++pass;
			while(1)
			{
				if( m_resolved.SetIfEqual(pass << 16, resolved) )//start the next pass
					break;
				resolved = m_resolved;
				if( resolved >> 16 == pass )//another thread incremented the pass number
					break;
				eiASSERT( (resolved >> 16) < (int)pass );
			}
		}
	}
	if( (resolved & thisMask) == 0 )
	{
		if( Resolve(worker, pass) )
			m_resolved += thisMask;
	}
	return Resolving;
}


AssetsLoadedCallback::AssetsLoadedCallback(callback fn, void* arg, TaskSection task, const char* name)
	: fn(fn)
	, arg(arg)
	, fnTask(task)
	, next()
{
	eiDEBUG( dbgName = name );
	eiDEBUG( done = false );
}
AssetsLoadedCallback::~AssetsLoadedCallback()
{
	eiASSERT( done );
}
void AssetScope::OnLoaded( AssetsLoadedCallback& cb )
{
	if( IsLoaded() )
	{
		eiASSERT( false );
	}
	eiASSERT( !cb.next );
	struct Insert
	{
		Insert(AssetsLoadedCallback& cb, AtomicPtr<AssetsLoadedCallback>& m_onLoaded) : cb(cb), m_onLoaded(m_onLoaded) {} AssetsLoadedCallback& cb; AtomicPtr<AssetsLoadedCallback>& m_onLoaded;
		bool operator()()
		{
			cb.next = m_onLoaded;
			return m_onLoaded.SetIfEqual(&cb, cb.next);
		}
	};
	YieldThreadUntil( Insert(cb, m_onLoaded) );
}
void AssetScope::OnLoaded()
{
	for( AssetsLoadedCallback* c = m_onLoaded; c; c = c->next )
	{
		eiBeginSectionRedundant( c->fnTask );
		{
			c->fn( c->arg );
			eiDEBUG( c->done = true );
		}
		eiEndSection( c->fnTask );
	}
}

bool AssetScope::IsLoaded() const
{
	if( IsLoading() )
		return false;
	s32 resolved = m_resolved;
	s32 userMask = (s32)m_userThreads.GetMask();
	uint pass = ((uint)resolved) >> 16;
	if( (resolved & 0xFFFF) == userMask && //all threads done
		(int)pass >= m_maxResolveOrder )   //no more resolve passes required
	{
		return true;//nope, all done
	}
	return false;
}

bool AssetScope::IsLoading() const
{
	if( m_parent && m_parent->IsLoading() )
		return true;
	s32 loads = m_loadsInProgress;
	return loads != 0xFFFFFFFF;
}

AssetScope::~AssetScope()
{
	eiASSERT( (s32)m_dbgReleased == (s32)m_userThreads.GetMask() );
#if defined(eiASSET_REFRESH)
	m_root->Destroyed(*this);
	struct FreeRefreshAllocations
	{
		void operator()( const AssetName&, AssetStorage& asset ) const
		{
			asset.RefreshFree();
		}
	};
	m_assets.ForEach( FreeRefreshAllocations() );
	struct Destruct
	{
		void operator()( const AssetName&, AssetStorage& asset ) const
		{
			asset.Destruct();
		}
	};
	m_assets.ForEach( Destruct() );
#endif
}

void AssetScope::Release(uint worker)
{
	eiDEBUG( s32 thisMask = 1<<worker );
	eiDEBUG( s32 released = m_dbgReleased );
	eiDEBUG( s32 userMask = (s32)m_userThreads.GetMask() );
	eiASSERT(  userMask & thisMask );
	eiASSERT( (released & thisMask) == 0 );
	eiDEBUG( m_dbgReleased += thisMask );
	struct FnRelease
	{
		void operator()( void* factory, FactoryData& data ) const
		{
			eiASSERT( factory && (s32)data.ready );
			PfnRelease onRelease = data.onRelease;
			if( onRelease )
			{
				eiBeginSectionThread( data.factoryThread )//todo same thread mask (rename if so) or another?
				{
					for( AssetStorage* asset = data.assets; asset; asset = asset->m_next )
					{
						onRelease( factory, *asset );
					}
				}
				eiEndSection( data.factoryThread );
			}
		}
	};
	m_factories.ForEach( FnRelease() );
}

bool AssetScope::Resolve(uint worker, uint pass)
{
	struct FnResolve
	{
		FnResolve(uint pass):pass(pass), complete(true) {} uint pass; bool complete;
		void operator()( void* factory, FactoryData& data )
		{
			eiASSERT( factory && (s32)data.ready );
			PfnResolve onResolve = data.onResolve;
			if( onResolve && data.resolveOrder == pass )
			{
				eiBeginSectionThread( data.factoryThread )//todo same thread mask (rename if so) or another?
				{
					for( AssetStorage* asset = data.assets; asset; asset = asset->m_next )
					{
#ifdef eiASSET_REFRESH
						if( !asset->m_resolved )
						{
							asset->m_resolved = true;
							onResolve( factory, *asset );
						}
#else
						onResolve( factory, *asset );
#endif
						if( false )//TODO - max resolve per frame counter
						{
							complete = false;
							break;
						}
					}
				}
				eiEndSection( data.factoryThread );
			}
		}
	};
	FnResolve fn(pass);
	m_factories.ForEach( fn );
	return fn.complete;
}

static void AtomicMax(Atomic* inout, s32 input)//todo - move
{
	s32 value;
	do 
	{
		value = *inout;
		if( input <= value )
			return;
	} while ( inout->SetIfEqual(input, value) );
}

AssetScope::FactoryData* AssetScope::FindFactoryBucket(void* factory, const FactoryInfo& info)
{
	int index = -1;
	bool inserted = m_factories.Insert(factory, index);
	if( !inserted && index < 0 )
		return 0;
	FactoryData& data = m_factories.At(index);
	if( inserted )
	{
		AtomicMax(&m_maxResolveOrder, (int)info.resolveOrder);
		data.factoryThread = info.factoryThread;
		data.resolveOrder  = info.resolveOrder;
		data.onResolve     = info.onResolve;
		data.onRelease     = info.onRelease;
#ifdef eiASSET_REFRESH
		data.onRefresh     = info.onRefresh;
		data.onAllocate    = info.onAllocate;
		data.onComplete    = info.onComplete;
#endif
		data.assets = 0;
		data.ready = 1;//signal constructed
	}
	else
	{
		YieldThreadUntil( WaitFor1(data.ready) );//ensure constructed before continuing
		eiASSERT( ((SectionBlob*)&info.factoryThread)->bits == ((SectionBlob*)&data.factoryThread)->bits );
		eiASSERT( info.onResolve  == data.onResolve );
		eiASSERT( info.onRelease  == data.onRelease );
	}
	return &data;
}

Asset* AssetScope::Find( AssetName n )
{
	if( m_parent )
	{
		Asset* found = m_parent->Find(n);
		if( found )
			return found;
	}
	int asset = -1;
	if( m_assets.Find(n, asset) )
	{
		return &m_assets.At(asset);
	}
	return 0;
}
Asset* AssetScope::Load( AssetName n, BlobLoader& blobs, void* factory, const FactoryInfo& info )
{
#if defined(eiASSET_REFRESH)
	eiASSERT( m_assetRefreshMode!=0 || !m_a.Sealed() );
#else
	eiASSERT( !m_a.Sealed() );
#endif
	//TODO - look for asset in parent!
	int asset = -1;
	if( m_assets.Insert(n, asset) )
	{
		bool ok;
		eiASSERT( m_resolved == 0 );
		++m_loadsInProgress;
		AssetStorage* ptr = 0;
		ptr = &m_assets.At(asset);
		ptr->Construct(n);
		
		FactoryData* factoryData = FindFactoryBucket(factory, info);
		do
		{
			ptr->m_next = factoryData->assets;
		}while( !factoryData->assets.SetIfEqual(ptr, ptr->m_next) );

		BlobLoadContext ctx =
		{
			/*scope      */ this,
			/*asset      */ ptr,
			/*factory    */ factory
			/*factoryName*/ eiDEBUG(COMMA info.factoryName),
		};
		BlobLoader::Request req =
		{
			/*onComplete */ info.factoryThread,
			/*userData   */ ctx,
			/*pfnAllocate*/ info.onAllocate,
			/*pfnComplete*/ info.onComplete,
		};
		ok = blobs.Load( n, req );
		eiASSERT( ok );//TODO - mark the asset as needing to try to load again next time Load is called with 'n'
		return ptr;
	}
	else if( asset != -1 )
		return &m_assets.At(asset);
	eiASSERT( false );
	return 0;
}

void AssetScope::BeginAssetRefresh()
{
#if defined(eiASSET_REFRESH)
//	eiASSERT( !IsLoading() );
	eiDEBUG( uint worker = GetThreadId()->ThreadIndex() );
	eiDEBUG( s32 resolved = m_resolved );
	eiDEBUG( uint pass = ((uint)resolved) >> 16 );
	eiDEBUG( s32 userMask = (s32)m_userThreads.GetMask() );
	eiDEBUG( s32 thisMask = 1<<worker );
	eiASSERT(  userMask & thisMask );
	eiASSERT( (userMask & 0xFFFF) == userMask );
	eiASSERT( (resolved & 0xFFFF) == userMask );//all threads done
	eiASSERT( (int)pass >= m_maxResolveOrder );//no more resolve passes required
	eiASSERT( m_loadsInProgress == 0xFFFFFFFF );//nothing loading
	m_resolved = 0;
	m_loadsInProgress = s_loadingLock;
	m_assetRefreshMode = 1;
#else
	eiASSERT(false);
#endif
}
void AssetScope::EndAssetRefresh()
{
#if defined(eiASSET_REFRESH)
	m_loadsInProgress.SetIfEqual(0xFFFFFFFF, s_loadingLock);
#else
	eiASSERT(false);
#endif
}
AssetStorage* AssetScope::Refresh(AssetName n, BlobLoader& blobs, Scope& temp, AssetName** depends, uint* numDepends)
{
#if defined(eiASSET_REFRESH)
	int asset = -1;
	if( !m_assets.Find(n, asset) )
		return 0;
	struct FnRefresh
	{
		FnRefresh(AssetScope& self, AssetName n, BlobLoader& blobs, Scope& a) : self(self), n(n), blobs(blobs), a(a), result(), resultCount() {} AssetScope& self; AssetName n; BlobLoader& blobs; Scope& a;
		AssetName* result; uint resultCount;
		bool operator()( void* factory, FactoryData& data )
		{
			eiASSERT( factory && (s32)data.ready );
			for( AssetStorage* asset = data.assets; asset; asset = asset->m_next )
			{
				if( asset->m_name == n )
				{
					result = self.LoadRefresh(asset, factory, data, blobs, a, &resultCount);
					return true;
				}
			}
			return false;
		}
	};
	FnRefresh fn(*this, n, blobs, temp);
	m_factories.ForEachBreak( fn );
	*numDepends = fn.resultCount;
	*depends = fn.result;
	AssetStorage* refreshed = &m_assets.At(asset);
	eiASSERT( refreshed->m_name == n );
	return refreshed;
#else
	eiASSERT(false);
	return 0;
#endif
}
AssetName* AssetScope::LoadRefresh(AssetStorage* ptr, void* factory, const FactoryData& info, BlobLoader& blobs, Scope& a, uint* out_numDepends)
{
#if defined(eiASSET_REFRESH)
	eiASSERT( m_resolved == 0 );
	++m_loadsInProgress;

	ptr->UnlinkFromUsedAssets();
	// No locks are acquired here, because we're assuming that LoadRefresh is
	// only called during an interrupt, where all other threads are paused.
	uint numUsers = ptr->m_assetDependsOnThis.size();
	*out_numDepends = numUsers;
	AssetName* users = 0;
	if( numUsers )
	{
		users = eiAllocArray(a, AssetName, numUsers);
		for( uint i=0; i != numUsers; ++i )
		{
			users[i] = ptr->m_assetDependsOnThis[i]->m_name;
		}
	}

	BlobLoadContext ctx =
	{
		/*scope      */ this,
		/*asset      */ ptr,
		/*factory    */ factory
		/*factoryName*/ eiDEBUG(COMMA "Refresh"),
	};
	BlobLoader::Request req =
	{
		/*onComplete */ info.factoryThread,
		/*userData   */ ctx,
		/*pfnAllocate*/ info.onAllocate,
		/*pfnComplete*/ info.onComplete,
	};
	bool ok = blobs.Load( ptr->m_name, req );
	eiASSERT( ok );//TODO - mark the asset as needing to try to load again next time Load is called with 'n'
	return users;
#else
	eiASSERT(false);
	return 0;
#endif
}

void* AssetScope::Allocate( AssetStorage& asset, uint size, uint workerIdx )
{
//	eiDEBUG( s32 i = m_loadsInProgress );
	if( m_allocOwner.WorkerIndex() == workerIdx )
	{
		eiAssertInTaskSection( m_allocOwner );
#ifdef eiASSET_REFRESH
		if( m_assetRefreshMode != 0 )
			return asset.RefreshAlloc( size );
#endif
		void* allocation = m_a.Alloc( size );
		eiASSERT( allocation );
		return allocation;
	}
	return 0;
}

void AssetScope::OnBlobLoaded(AssetStorage& asset)
{
#ifdef eiASSET_REFRESH
	asset.m_resolved = false;
#endif
#if eiDEBUG_LEVEL > 0
	bool done;
	do
	{
		s32 loading = m_loadsInProgress;
		eiASSERT( loading != 0xFFFFFFFF );
		if( (((loading-1) & s_loadingLock) == s_loadingLock) && (((loading-1) & ~s_loadingLock) == 0) )
		{
			eiASSERT( loading == s_loadingLock+1 );
			bool ok = m_loadsInProgress.SetIfEqual(0, s_loadingLock+1);
			eiASSERT( ok );
		//	m_a.Seal();
			ok = m_loadsInProgress.SetIfEqual(0xFFFFFFFF, 0);
			eiASSERT( ok );
			done = true;
		}
		else
		{
			done = m_loadsInProgress.SetIfEqual(loading-1, loading);
			eiASSERT( loading != s_loadingLock+1 );
		}
	}while(!done);
#else
	bool done;
	do
	{
		s32 loading = m_loadsInProgress;
		if( loading == s_loadingLock+1 )
		{
			m_loadsInProgress = 0;
		//	m_a.Seal();
			m_loadsInProgress = 0xFFFFFFFF;
			done = true;
		}
		else
			done = m_loadsInProgress.SetIfEqual(loading-1, loading);
	}while(!done);
#endif
}
//------------------------------------------------------------------------------
