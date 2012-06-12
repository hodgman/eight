//------------------------------------------------------------------------------
#include <eight/core/test.h>
#include <eight/core/application.h>
#include <eight/core/alloc/stack.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/resourcepool.h>
#include <eight/core/timer/timer.h>
#include <eight/core/timer/timer_impl.h>
#include <stdlib.h>
#include <stdio.h>
#include <deque>
#include <list>
using namespace eight;
//------------------------------------------------------------------------------

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

eiENTRY_POINT( test_main )
int test_main( int argc, char** argv )
{
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
	u8* scratch = (u8*)malloc(scratchSize);

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

	free(scratch);
	
	int errorCount = 0;
	eiRUN_TEST( FifoSpsc, errorCount );
	eiRUN_TEST( FifoMpmc, errorCount );
	eiRUN_TEST( TaskSection, errorCount );
	eiRUN_TEST( TaskSchedule, errorCount );

	return errorCount;
}

//------------------------------------------------------------------------------

namespace CUtil
{
typedef float F32;
typedef float Real;

typedef unsigned int DSSizeT;
typedef unsigned int UInt;

typedef unsigned short DateT;
typedef unsigned int TimeT;

typedef unsigned long TimeHiResT;

typedef unsigned short WCharT;

typedef size_t MemIndexT;

enum SortOrder {ASC, DESC};
enum ERBColor {RBC_RED, RBC_BLACK};
enum HeapType {MINHEAP, MAXHEAP};

enum EListLocation {LP_START, LP_END, LP_INDEX};

typedef void * (FncNewObject)(size_t);
typedef void (FncDeleteObject)(void *);

struct MemoryStats
{
	MemIndexT NumAllocFragments;
	MemIndexT NumAvailFragments;
	MemIndexT TotalBytes;
	MemIndexT UsedBytes;
};
#define DEBUG_LISTCHUNKALLOCATOR 0


class ListChunkNode
{
public:
	bool IsAlloc;
#if DEBUG_LISTCHUNKALLOCATOR == 1
	MemIndexT LinkID;
#endif
	ListChunkNode *Prev;
	ListChunkNode *Next;
	MemIndexT NumAllocFragments; //Number of Continuous Fragments from this fragment

	ListChunkNode(void *argpdata, ListChunkNode *argPrev, ListChunkNode *argnext, bool argIsAlloc = false, MemIndexT argNumAllocFragments = 0)
	{
		IsAlloc = argIsAlloc;
		Prev = argPrev;
		Next = argnext;
		NumAllocFragments = argNumAllocFragments;
	}

	ListChunkNode()
	{
	}
};

class LinkChunkAllocator
{
	ListChunkNode *m_freeStart;
	ListChunkNode *m_allocStart;

	ListChunkNode *m_dataLinks;
	char *m_data;

	MemIndexT m_fragmentSize;
	MemIndexT m_mumFragments;

	MemIndexT m_totalSize;

	MemIndexT m_numAllocFragments;

	inline void cutFragment(ListChunkNode *node, bool isfromFreePool = true);
	inline void defragmentLinks(ListChunkNode *startNode, ListChunkNode *midNode, ListChunkNode *endNode);

public:
	LinkChunkAllocator(MemIndexT fragmentSize, MemIndexT mumFragments);
	virtual ~LinkChunkAllocator();

	MemIndexT getNumLinks()
	{
		return m_mumFragments;
	}

	const ListChunkNode *getDataLinks()
	{
		return m_dataLinks;
	}

	const ListChunkNode *getFreeLinks()
	{
		return m_freeStart;
	}

	const ListChunkNode *getAllocatedLinks()
	{
		return m_allocStart;
	}

	void *newObject(MemIndexT size);
	void deleteObject(void *ptr);

	void getMemoryStats(MemoryStats &memStats)
	{
		memStats.NumAllocFragments = m_numAllocFragments;
		memStats.NumAvailFragments = m_mumFragments;
		memStats.UsedBytes = m_numAllocFragments * m_fragmentSize;
		memStats.TotalBytes = m_mumFragments * m_fragmentSize;
	}
};
}

void test2(void* scratch, uint scratchSize)
{
	//CUtil::LinkChunkAllocator lca(4, 2621440);

	StackAlloc alloc(scratch, ((u8*)scratch)+scratchSize);
	Scope a(alloc, "");
	Timer* timer = eiNew(a, Timer)();
	(void)/*printf*/("\n\n########Standard List########");
	for(int itrSample = 0; itrSample < 30; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			std::list<int> lst;
			for(int itr = 0; itr < 500; itr++)
			{
				lst.push_front(itr);
			}
		}
		double end = timer->Elapsed();
		(void)/*printf*/("\nSample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
	}

	struct ListItem
	{
		int data;
		ListItem* next;
	};

	(void)/*printf*/("\n\n########Malloc/Free########");
	for(int itrSample = 0; itrSample < 30; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			ListItem* lst = 0;
			for(int itr = 0; itr < 500; itr++)
			{
				ListItem* push = (ListItem*)malloc(sizeof(ListItem));
				push->next = lst;
				push->data = itr;
				lst = push;
			}

			while(lst)
			{
				ListItem* dead = lst;
				lst = dead->next;
				free(dead);
			}
		}
		double end = timer->Elapsed();
		(void)/*printf*/("\nSample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
	}

	(void)/*printf*/("\n\n########New/Delete########");
	for(int itrSample = 0; itrSample < 30; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			ListItem* lst = 0;
			for(int itr = 0; itr < 500; itr++)
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
		(void)/*printf*/("\nSample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
	}

	(void)/*printf*/("\n\n########Pool########");
	ResourcePool<ListItem> pool(a,10000);
	for(int itrSample = 0; itrSample < 30; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			ListItem* lst = 0;
			for(int itr = 0; itr < 500; itr++)
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
		(void)/*printf*/("\nSample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
	}
	
	(void)/*printf*/("\n\n########eiNew########");
	for(int itrSample = 0; itrSample < 30; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			Scope t(a,"");
			ListItem* lst = 0;
			for(int itr = 0; itr < 500; itr++)
			{
				ListItem* push = eiNew(t, ListItem);
				push->next = lst;
				push->data = itr;
				lst = push;
			}
		}
		double end = timer->Elapsed();
		(void)/*printf*/("\nSample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
	}

	(void)/*printf*/("\n\n########eiAlloc########");
	for(int itrSample = 0; itrSample < 30; itrSample++)
	{
		double begin = timer->Elapsed();
		for(int itrItr = 0; itrItr < 10; itrItr++)
		{
			Scope t(a,"");
			ListItem* lst = 0;
			for(int itr = 0; itr < 500; itr++)
			{
				ListItem* push = eiAlloc(t, ListItem);
				push->next = lst;
				push->data = itr;
				lst = push;
			}
		}
		double end = timer->Elapsed();
		(void)/*printf*/("\nSample %d Time: %f ms", itrSample, float(end - begin)*1000.0f);
		
		char* data = (char*)malloc(640*1024);//640KiB should be enough for anybody
		int& fooVec = *new(data) int;
		data += sizeof(fooVec);
	}
}

