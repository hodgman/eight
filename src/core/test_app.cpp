//------------------------------------------------------------------------------
#include <eight/core/test.h>
#include <eight/core/application.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/pool.h>
#include <eight/core/timer/timer.h>
#include <eight/core/thread/atomic.h>
#include <eight/core/thread/hashtable.h>
#include <eight/core/hash.h>
#include <stdio.h>
#include <deque>
#include <list>
using namespace eight;
//------------------------------------------------------------------------------

//TODO - revisit these ideas

void* operator new(size_t s, StackAlloc& a) {return a.Alloc(s);}
void operator delete(void*, StackAlloc&){}

struct ModelAabb{ int model, aabb; };
struct Model{};
struct Camera{};
struct Aabb{};
struct Rt{};
struct RtPool
{
	Rt* Acquire(int option){return 0;}
	void Release(Rt*){}
};
template<class T> struct Array
{
	Array(StackAlloc& a, int size) : length(size) { a.Alloc<T>(size); }
	int length;
	T* Begin() { return (T*)(&length + 1); }
	T& At(int i) { return *(Begin() + i); }
};
struct FlowContext;
struct Event
{
	template<class T> Event(void* object, T callback) : fn((Callback)callback), object(object) {}
	typedef void (*Callback)( void*, FlowContext& );
	Callback fn;
	void* object;
};
struct FlowContext
{
	FlowContext(StackAlloc& a):alloc(a){}
	std::deque<Event> events;
	void Execute()
	{
		const u8* mark = alloc.Mark();
		while(!events.empty())
		{
			Event& e = events.front();
			e.fn(e.object, *this);
			events.pop_front();
		}
		alloc.Unwind(mark);
	}
	StackAlloc& TempAlloc() { return alloc; }
private:
	StackAlloc& alloc;
};

struct TriggerArray : public Array<Event>
{
	TriggerArray(StackAlloc& a, int size) : Array<Event>(a, size) {}
	void Trigger( FlowContext& context )
	{
		for( int i=0, end=length; i!=end; ++i )
		{
			context.events.push_back( At(i) );
		}
	}
};

struct FrustumCull
{
	FrustumCull(StackAlloc& a, int listeners) : aabbs(), camera(), indices(), output(), onComplete(new(a)TriggerArray(a, listeners)) {}
	//-- in
	Array<Aabb>** aabbs;
	Camera** camera;
	Array<ModelAabb>** indices;
	//-- out
	Array<int>* output;
	//-- events
	static void Cull( FrustumCull* self, FlowContext& c )
	{
		Array<Aabb>& aabbs = **self->aabbs;
		Array<ModelAabb>& indices = **self->indices;
		Camera& camera = **self->camera;
		StackAlloc& a = c.TempAlloc();
		self->output = new(a)Array<int>(a,indices.length);
		self->onComplete->Trigger(c);
	}
	//-- callbacks
	TriggerArray* onComplete;
};

struct AcquireTarget
{
	AcquireTarget(StackAlloc& a, int listeners) : targetType(), pool(), output(), onComplete(new(a)TriggerArray(a, listeners)) {}
	//-- config
	int targetType;
	//-- in
	RtPool** pool;
	//-- out
	Rt* output;
	//-- events
	static void Acquire( AcquireTarget* self, FlowContext& c )
	{
		RtPool& pool = **self->pool;
		self->output = pool.Acquire(self->targetType);
		self->onComplete->Trigger(c);
	}
	//-- callbacks
	TriggerArray* onComplete;
};

struct DrawModels
{
	DrawModels(StackAlloc& a, int listeners) : models(), indices(), target(), output(), onComplete(new(a)TriggerArray(a, listeners)) {}
	//-- in
	Array<Model>** models;
	Array<int>** indices;
	Rt** target;
	//-- out
	Rt* output;
	//-- events
	static void Draw( DrawModels* self, FlowContext& c )
	{
		Array<Model>& models = **self->models;
		Array<int>& indices = **self->indices;
		self->output = *self->target;
		self->onComplete->Trigger(c);
	}
	//-- callbacks
	TriggerArray* onComplete;
};

struct EntryPoint
{
	EntryPoint(StackAlloc& a, int calls) : onBegin(new(a)TriggerArray(a, calls)) {}
	TriggerArray* onBegin;
};

struct Scene
{
	Scene(StackAlloc& a, int maxModels)
		: aabbs(new(a)Array<Aabb>(a, maxModels))
		, models(new(a)Array<Model>(a, maxModels))
		, opaqueList(new(a)Array<ModelAabb>(a, maxModels))
		, translucentList(new(a)Array<ModelAabb>(a, maxModels))
		, pool(new(a)RtPool)
		, camera(new(a)Camera){}
	Array<Aabb>* aabbs;
	Array<Model>* models;
	Array<ModelAabb>* opaqueList;
	Array<ModelAabb>* translucentList;
	RtPool* pool;
	Camera* camera;
};


#include <map>
#include <unordered_map>
#include <string>
#include <rdestl/hash_map.h>
#include <rdestl/rde_string.h>


static const char* words[] = 
{
	"fug",
	"fugacities",
	"fugacity",
	"fugal",
	"fugally",
	"fugato",
	"fugatos",
	"fugged",
	"fuggier",
	"fuggiest",
	"fugging",
	"fuggy",
	"fugio",
	"fugios",
	"fugitive",
	"fugitively",
	"fugitiveness",
	"fugitives",
	"fugle",
	"fugled",
	"fugleman",
	"fuglemen",
	"fugles",
	"fugling",
	"fugs",
	"fugu",
	"fugue",
	"fugued",
	"fugues",
	"fuguing",
	"fuguist",
	"fuguists",
	"fugus",
	"fuhrer",
	"fuhrers",
	"fuji",
	"fug",
	"fugacities",
	"fugacity",
	"fugal",
	"fugally",
	"fugato",
	"fugatos",
	"fugged",
	"fuggier",
	"fuggiest",
	"fugging",
	"fuggy",
	"fugio",
	"fugios",
	"fugitive",
	"fugitively",
	"fugitiveness",
	"fugitives",
	"fugle",
	"fugled",
	"fugleman",
	"fuglemen",
	"fugles",
	"fugling",
	"fugs",
	"fugu",
	"fugue",
	"fugued",
	"fugues",
	"fuguing",
	"fuguist",
	"fuguists",
	"fugus",
	"fuhrer",
	"fuhrers",
	"fuji",
};

template<class String, class Map, class Clear> void TestMap( Map& map, Timer* timer, int test_size, const char* desc, Clear& clear )
{
	int sum = 0;
	double a=0, b=0, c=0;
	for( int i=0; i!=100; ++i )
	{
		ReadWriteBarrier();
		double begin = timer->Elapsed();
		ReadWriteBarrier();
		for( int i=0; i!=test_size; ++i )
		{
			int word_index = i % eiArraySize(words);
			map[String(words[word_index])] = i;
		}
		ReadWriteBarrier();
		double middle = timer->Elapsed();
		ReadWriteBarrier();
		for( int i=0; i!=test_size; ++i )
		{
			int word_index = i % eiArraySize(words);
			sum += map[String(words[word_index])];
		}
		ReadWriteBarrier();
		double end = timer->Elapsed();
		ReadWriteBarrier();

		a += middle - begin;
		b += end - middle;
		c += end - begin;

		clear();
	}
	printf("%s size %d, Write: %f ms, Read: %f ms. Total: %f. Result = %d\n", desc, test_size, a*100, b*100, c*100, sum);
}


struct String
{
	String(const char* string) : string(string), hash(Fnv32a(string)) {}
	u32 hash;
	const char* string;
	bool operator==( const String& s )
	{
		return hash == s.hash && (0==strcmp(string,s.string));
	}
};
template<> struct NativeHash<String>
{
	static u32 Hash(const String& s) { return s.hash; }
	typedef u32 type;
};
template<> struct NativeHash<std::string>
{
	static u32 Hash(const std::string& s) { return Fnv32a(s.c_str()); }
	typedef u32 type;
};
template<> struct NativeHash<const char*>
{
	static u32 Hash(const char* string) { return Fnv32a(string); }
	typedef u32 type;
};

namespace rde
{
	hash_value_t extract_int_key_value(const std::string& t)
	{
		return (hash_value_t)Fnv32a(t.c_str());
	}
}

template<class Key, class Value>
struct VectorMap
{
	VectorMap(uint size) { nodes.reserve(size); }
	struct Node { Node(const Key& k) :k(k){} Key k; Value v; };
	std::vector<Node> nodes;

	void clear() { nodes.clear(); }

	Value& operator[]( const Key& k )
	{
		for( size_t i=0, end=nodes.size(); i!=end; ++i )
		{
			if( nodes[i].k == k )
				return nodes[i].v;
		}
		nodes.emplace_back(k);
		return nodes.back().v;
	}
};

eiENTRY_POINT( test_main )
int test_main( int argc, char** argv )
{
	/*
	const u32 W = 1280;
	const u32 H = 720;
	const u32 R = 128;
	u32* source = new u32[W*H];
	u32* buffer = new u32[(W+R*2)*(H+R*2)];
	for( u32 y=0; y!=H; ++y )
	{
		for( u32 x=0; x!=W; ++x )
		{
		}
	}
	delete [] source;
	delete [] buffer;




	const u32 scratchSize = 1024*1024*100;
	u8* scratch = (u8*)Malloc(scratchSize);

	void test2(void* scratch, uint scratchSize);
	test2(scratch, scratchSize);

	StackAlloc alloc(scratch, scratch+scratchSize);

	Scene* scene = new(alloc) Scene(alloc, 128);

	EntryPoint* entry = new(alloc) EntryPoint(alloc, 2);

	FrustumCull* cullOpaque = new(alloc) FrustumCull(alloc, 1);
	FrustumCull* cullTranslucent = new(alloc) FrustumCull(alloc, 0);

	AcquireTarget* acquireMain = new(alloc) AcquireTarget(alloc, 1);

	DrawModels* drawOpaque = new(alloc) DrawModels(alloc, 1);
	DrawModels* drawTranslucent = new(alloc) DrawModels(alloc, 1*0x0);

	entry->onBegin->At(0) = Event(cullOpaque, FrustumCull::Cull);
	entry->onBegin->At(1) = Event(cullTranslucent, FrustumCull::Cull);

	cullOpaque->aabbs = &scene->aabbs;
	cullOpaque->camera = &scene->camera;
	cullOpaque->indices = &scene->opaqueList;
	cullOpaque->onComplete->At(0) = Event(acquireMain, AcquireTarget::Acquire);
	
	cullTranslucent->aabbs = &scene->aabbs;
	cullTranslucent->camera = &scene->camera;
	cullTranslucent->indices = &scene->translucentList;

	acquireMain->targetType = 42;
	acquireMain->pool = &scene->pool;
	acquireMain->onComplete->At(0) = Event(drawOpaque, DrawModels::Draw);

	drawOpaque->models = &scene->models;
	drawOpaque->indices = &cullOpaque->output;
	drawOpaque->target = &acquireMain->output;
	drawOpaque->onComplete->At(0) = Event(drawTranslucent, DrawModels::Draw);

	drawTranslucent->models = &scene->models;
	drawTranslucent->indices = &cullTranslucent->output;
	drawTranslucent->target = &drawOpaque->output;

	FlowContext c(alloc);
	entry->onBegin->Trigger(c);
	c.Execute();

	Free(scratch);*/

	InitCrashHandler();

	
//	{
//		LocalScope<eiKiB(1)> a;
//		Timer* timer = eiNewInterface(a, Timer)();
//		for(int itrSample = 0; itrSample < 5; itrSample++)
//		{
//			int test_size = (int)pow(10,itrSample+1);
//			{ std::map<          std::string, int> map;                 TestMap<std::string>( map, timer, test_size, "std::map            ", [&](){map.clear();} ); }
//			{ std::unordered_map<std::string, int> map(test_size);      TestMap<std::string>( map, timer, test_size, "std::unordered_map  ", [&](){map.clear();} ); }
//			{ VectorMap<         std::string, int> map(test_size);      TestMap<std::string>( map, timer, test_size, "VectorMap           ", [&](){map.clear();} ); }
//			{ rde::hash_map<     std::string, int> map(test_size);      TestMap<std::string>( map, timer, test_size, "rde::hash_map       ", [&](){map.clear();} ); }
//			{ HashTable<               void*, int> map( a, test_size ); TestMap<      void*>( map, timer, test_size, "eight::CheatTable   ", [&](){map.~    HashTable(); new(&map) HashTable<      void*, int>( a, test_size );} ); }
//			{ HashTable<         const char*, int> map( a, test_size ); TestMap<const char*>( map, timer, test_size, "eight::HashTable1   ", [&](){map.~    HashTable(); new(&map) HashTable<const char*, int>( a, test_size );} ); }
//			{ HashTable<              String, int> map( a, test_size ); TestMap<     String>( map, timer, test_size, "eight::HashTable2   ", [&](){map.~    HashTable(); new(&map) HashTable<     String, int>( a, test_size );} ); }
//			{ HashTable<         std::string, int> map( a, test_size ); TestMap<std::string>( map, timer, test_size, "eight::HashTable3   ", [&](){map.~    HashTable(); new(&map) HashTable<     String, int>( a, test_size );} ); }
//			{ MpmcHashTable<          String, int> map( a, test_size ); TestMap<     String>( map, timer, test_size, "eight::MpmcHashTable", [&](){map.~MpmcHashTable(); new(&map) MpmcHashTable< String, int>( a, test_size );} ); }
//		/*	{
//				ReadWriteBarrier();
//				double begin = timer->Elapsed();
//				ReadWriteBarrier();
//				HashTable<const char*, int> map( a, test_size );
//				for( int i=0; i!=test_size; ++i )
//				{
//					int word_index = i % eiArraySize(words);
//					map[(words[word_index])] = i;
//				}
//				ReadWriteBarrier();
//				double middle = timer->Elapsed();
//				ReadWriteBarrier();
//				int sum = 0;
//				for( int i=0; i!=test_size; ++i )
//				{
//					int word_index = i % eiArraySize(words);
//					sum += map[(words[word_index])];
//				}
//				ReadWriteBarrier();
//				double end = timer->Elapsed();
//				ReadWriteBarrier();
//				printf("%s size %d, Write: %f ms, Read: %f ms. Result = %d\n", "eight::HashTable  ", test_size, float(middle - begin)*1000.0f, float(end - middle)*1000.0f, sum);
//			}*/
//			printf("\n");
//		}
//	}
	
	int errorCount = 0;
//	eiRUN_TEST( Lua, errorCount );
//	eiRUN_TEST( FluidSim, errorCount );
//	eiRUN_TEST( SphereFrustum, errorCount );
//	eiRUN_TEST( Bind, errorCount );
	eiRUN_TEST( Message, errorCount );
//	eiRUN_TEST( FifoSpsc, errorCount );
//	eiRUN_TEST( FifoMpmc, errorCount );
//	eiRUN_TEST( TaskSection, errorCount );
//	eiRUN_TEST( TaskSchedule, errorCount );

	return errorCount;
}


//------------------------------------------------------------------------------

eiInfoGroup( AllocProfile, true );

void test2(void* scratch, uint scratchSize)
{
	StackAlloc alloc(scratch, ((u8*)scratch)+scratchSize);
	Scope a(alloc, "");
	Timer* timer = eiNewInterface(a, Timer)();
	eiInfo(AllocProfile, "########Standard List########");
	for(int itrSample = 0; itrSample < 3; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			std::list<int> lst;
			for(int itr = 0; itr < 50; itr++)
			{
				lst.push_front(itr);
			}
		}
		double end = timer->Elapsed();
		eiInfo(AllocProfile, "Sample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
	}

	struct ListItem
	{
		int data;
		ListItem* next;
	};

	eiInfo(AllocProfile, "########Malloc/Free########");
	for(int itrSample = 0; itrSample < 3; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			ListItem* lst = 0;
			for(int itr = 0; itr < 50; itr++)
			{
				ListItem* push = (ListItem*)Malloc(sizeof(ListItem));
				push->next = lst;
				push->data = itr;
				lst = push;
			}

			while(lst)
			{
				ListItem* dead = lst;
				lst = dead->next;
				Free(dead);
			}
		}
		double end = timer->Elapsed();
		eiInfo(AllocProfile, "Sample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
	}

	eiInfo(AllocProfile, "########New/Delete########");
	for(int itrSample = 0; itrSample < 3; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			ListItem* lst = 0;
			for(int itr = 0; itr < 50; itr++)
			{
				ListItem* push = new ListItem;
				push->next = lst;
				push->data = itr;
				lst = push;
			}

			while(lst)
			{
				ListItem* dead = lst;
				lst = dead->next;
				delete dead;
			}
		}
		double end = timer->Elapsed();
		eiInfo(AllocProfile, "Sample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
	}

	eiInfo(AllocProfile, "########Pool########");
	Pool<ListItem, false> pool(a,10000);
	for(int itrSample = 0; itrSample < 3; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			ListItem* lst = 0;
			for(int itr = 0; itr < 50; itr++)
			{
				ListItem* push = pool.Alloc();//(ListItem*)lca.newObject(sizeof(ListItem));
				push->next = lst;
				push->data = itr;
				lst = push;
			}

			while(lst)
			{
				ListItem* dead = lst;
				lst = dead->next;
				pool.Release(dead);//lca.deleteObject(dead);
			}
		}
		double end = timer->Elapsed();
		eiInfo(AllocProfile, "Sample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
	}
	
	eiInfo(AllocProfile, "########eiNew########");
	for(int itrSample = 0; itrSample < 3; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			Scope t(a,"");
			ListItem* lst = 0;
			for(int itr = 0; itr < 50; itr++)
			{
				ListItem* push = eiNew(t, ListItem);
				push->next = lst;
				push->data = itr;
				lst = push;
			}
		}
		double end = timer->Elapsed();
		eiInfo(AllocProfile, "Sample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
	}

	eiInfo(AllocProfile, "########eiAlloc########");
	for(int itrSample = 0; itrSample < 3; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			Scope t(a,"");
			ListItem* lst = 0;
			for(int itr = 0; itr < 50; itr++)
			{
				ListItem* push = eiAlloc(t, ListItem);
				push->next = lst;
				push->data = itr;
				lst = push;
			}
		}
		double end = timer->Elapsed();
		eiInfo(AllocProfile, "Sample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
		
		char* data = (char*)Malloc(640*1024);//640KiB should be enough for anybody
		int& fooVec = *new(data) int;
		data += sizeof(fooVec);
	}
}

