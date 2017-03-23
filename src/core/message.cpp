#include <eight/core/message.h>
#include <eight/core/bind.h>
#include <eight/core/debug.h>
#include <eight/core/bit/twiddling.h>
#include <eight/core/test.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/thread/task.h>
#include <eight/core/hash.h>
#include <eight/core/profiler.h>
#include <eight/core/thread/pool.h>
#include <ctime>
#include <vector>
#include <rdestl/vector.h>
#include <rdestl/set.h>

using namespace eight;

//todo - cleanup
extern ProfileCallback g_JsonProfileOut;

void CallBuffer::Insert(FutureCall* data)
{
	++m_count;
	eiASSERT( (!m_first) == (!m_last) );
	eiASSERT( !data->next );
	if( !m_first )
	{
		m_first = m_last = data;
	}
	else
	{
		eiASSERT( !m_last->next );
		m_last->next = data;
		m_last = data;
	}
}

FutureCall* CallBuffer::Push( const char* tag, FnMethod* call, void* obj, uint argSize, uint returnSize, uint objectSize, bool methodIsConst )
{
	FutureCall* data = a.Alloc<FutureCall>();
	data->call = call;
	data->object = obj;
	data->objectSize = objectSize;
	data->methodIsConst = methodIsConst;
	data->args = argSize ? (ArgHeader*)a.Alloc(argSize, 16) : 0;
	data->argSize = argSize;
	data->tag = tag;
	data->next = 0;
	if( !returnSize )
	{
		data->returnOffset = 0xFFFFFFFF;
		data->returnSize = 0;
		data->returnIndex = 0xFFFF;
	}
	else
	{
		m_returnSize.Align( 16 );//todo
		uint offset = (uint)m_returnSize.Alloc( returnSize );
		eiASSERT( offset < 0xFFFFFFFF );
		data->returnOffset = offset;
		data->returnSize = returnSize;
		data->returnIndex = m_nextReturnIndex++;
	}
	Insert(data);
	return data;
};

void ExecuteCallBuffer( const CallBuffer& q, Scope& temp )
{	
	Scope a(temp,"ExecuteCallBuffer");
	const uint returnSize = q.ReturnValuesSize();
	const uint returnCount = q.ReturnValuesCount();
	uint* const returnOffsets = a.Alloc<uint>( returnCount );
	u8* const returnData = a.Alloc( returnSize );
	for( FutureCall* i = q.First(), *last = q.Last(); i; i != last ? i = i->next : i = 0 )
	{
		ArgHeader* callArg = i->args;
		if( callArg && i->args->futureMask )
		{
			ArgHeader& args = *callArg;
			u32 futureMask = args.futureMask;
			u8* argTuple = (u8*)(&args + 1);
			uint numArgs = 0;
			const uint* offsets;
			const uint* sizes;
			eiASSERT( args.info && (void*)args.info != eiMAGIC_POINTER(0xCD));
			args.info(numArgs, offsets, sizes);
			FutureIndex* futureArray = (FutureIndex*)(numArgs ? argTuple + offsets[numArgs-1] + sizes[numArgs-1] : 0);
			do// for each bit in futureMask
			{
				uint idx = LeastSignificantBit(futureMask);//find index of lsb set
				futureMask = ClearLeastSignificantBit(futureMask); // clear the least significant bit set for next iteration
				eiASSERT( idx < numArgs );//future bits should match up with arg indices
				uint offset = offsets[idx];//look up where this item is inside the tuple
				uint size   = sizes[idx];//and how many bytes to copy it
				FutureIndex& future = futureArray[offset];
				uint futureIndex = future.index;
				uint futureOffset = future.offset; eiUNUSED(futureOffset);//TODO - offset into the future value
				eiASSERT( futureIndex < returnCount );
				eiASSERT( returnOffsets[futureIndex] != 0xFFFFFFFF );
				const u8* returnValue = returnData + returnOffsets[futureIndex];
				eiASSERT( offset < numArgs );
				//todo - support non-POD copy
				memcpy( &argTuple[offset], returnValue, size );//copy the return value into the call's arg tuple
			}while( futureMask );
		}
		eiASSERT( i->call );
		eiASSERT( i->returnOffset < returnSize || i->returnOffset == 0xFFFFFFFF );
		eiASSERT( i->returnIndex < returnCount || i->returnOffset == 0xFFFFFFFF );
		void* callOut = 0;
		uint returnOffset = i->returnOffset;
		if( returnOffset != 0xFFFFFFFF )
		{
			returnOffsets[i->returnIndex] = returnOffset;
			callOut = returnData + returnOffset;
		}
		i->call( i->object, (void*)callArg, callOut );
	}
}


TaskSchedule BuildCallGraph( CallBuffer& q, Scope& temp, Scope& results )//send to task prototypes
{
	//Make a buffer to keep track of return values at execution time
	const uint returnSize = q.ReturnValuesSize();
	const uint returnCount = q.ReturnValuesCount();
	u8* const returnData = results.Alloc( returnSize );
	uint* const returnOffsets = results.Alloc<uint>( returnCount );

	struct MemoryDependency
	{
		const void* begin, *end;
		uint idxNode;
		bool write;
		bool Overlapping( const MemoryDependency& other ) const
		{
			bool a = begin >= other.begin && begin < other.end;
			bool b = end   >= other.begin && end   < other.end;
			bool c = other.begin >= begin && other.begin < end;
			bool d = other.end   >= begin && other.end   < end;
			return a|b|c|d;
		}
	};
	struct CallNode
	{
		FutureCall* call;
		void* returnValue;
		int beginMemoryDependency;
		int endMemoryDependency;
		uint nodeGroup;
	};
	Scope a(temp,"ExecuteCallBuffer");
	
	//Discover the memory dependencies of every call
	rde::vector<MemoryDependency> memory;
	const uint numNodes = q.Count();
	CallNode* nodes = eiAllocArray(a, CallNode, numNodes);
	void** nodeReturnAddresses = eiAllocArray(a, void*, numNodes);
	uint nextNodeIndex = 0;
	for( FutureCall* i = q.First(), *last = q.Last(); i; i != last ? i = i->next : i = 0 )
	{
		uint idxNode = nextNodeIndex++;
		CallNode& node = nodes[idxNode];
		node.call = i;
		node.beginMemoryDependency = memory.size();
		node.nodeGroup = 0xFFFFFFFF;//filled in later

		if( i->object )
		{
			MemoryDependency md = { i->object, ((u8*)i->object)+i->objectSize, idxNode, !i->methodIsConst };
			memory.push_back(md);
		}

		eiASSERT( i->returnOffset < returnSize || i->returnOffset == 0xFFFFFFFF );
		eiASSERT( i->returnIndex < returnCount || i->returnOffset == 0xFFFFFFFF );
		void* callOut = 0;
		uint returnOffset = i->returnOffset;
		if( returnOffset != 0xFFFFFFFF )
		{
			returnOffsets[i->returnIndex] = returnOffset;
			callOut = returnData + returnOffset;
			MemoryDependency md = { callOut, ((u8*)callOut)+i->returnSize, idxNode, true };
			memory.push_back(md);
		}
		node.returnValue = callOut;
		nodeReturnAddresses[idxNode] = callOut;

		ArgHeader* args = i->args;
		if( args && (args->futureMask || args->pointerMask) )
		{
			u8* argTuple = (u8*)(args + 1);
			uint numArgs = 0;
			const uint* offsets;
			const uint* sizes;
			eiASSERT( args->info && (void*)args->info != eiMAGIC_POINTER(0xCD));
			args->info(numArgs, offsets, sizes);
			FutureIndex* futureArray = (FutureIndex*)(numArgs ? argTuple + offsets[numArgs-1] + sizes[numArgs-1] : 0);
			
			for( u32 futureMask = args->futureMask; futureMask; futureMask = ClearLeastSignificantBit(futureMask) )
			{
				u32 bit = ClearAllButLeastSignificantBit(futureMask);
				uint idx = LeastSignificantBit(futureMask);

				eiASSERT( idx < numArgs );//future bits should match up with arg indices
				uint offset = offsets[idx];//look up where this item is inside the tuple
				uint size   = sizes[idx];//and how many bytes to copy it
				const FutureIndex& future = futureArray[offset];
				uint futureIndex = future.index;
				uint futureOffset = future.offset; eiUNUSED(futureOffset);//TODO - offset into the future value
				eiASSERT( futureIndex < returnCount );
				eiASSERT( returnOffsets[futureIndex] != 0xFFFFFFFF );
				const u8* returnValue = returnData + returnOffsets[futureIndex];
				eiASSERT( offset < numArgs );
				MemoryDependency md = { returnValue, ((u8*)returnValue)+size, idxNode, false };
				memory.push_back(md);
			}
			
			for( u32 pointerMask = args->pointerMask; pointerMask; pointerMask = ClearLeastSignificantBit(pointerMask) )
			{
				u32 bit = ClearAllButLeastSignificantBit(pointerMask);
				uint idx = LeastSignificantBit(pointerMask);
				if( bit & args->futureMask )
				{
					//todo - can make a job that, when the future value exists, checks the actual value against our memory range arrays and determines the actual dependencies... this would trigger a completely different graph compilation strategy
					eiWarn("Unable to correctly track dependencies when future points are used");
					continue;
				}
				eiASSERT( idx < numArgs );
				uint offset = offsets[idx];
				eiASSERT( sizes[idx] == sizeof(void*) );
				void* argValue = *(void**)&argTuple[offset];
				uint pointedToSize = 4;//TODO!
				MemoryDependency md = { argValue, ((u8*)argValue)+pointedToSize, idxNode, (bit & args->constMask)==0 };
				memory.push_back(md);
			}
		}
		node.endMemoryDependency = memory.size();
	}
	eiASSERT( numNodes == nextNodeIndex );
	
	struct NodeDependencies
	{
		rde::vector<uint> prerequisite;
		rde::vector<uint> postrequisite;
		u32 hash;
		void SetHash()
		{
			u32 hash = 0;
			for( auto it=prerequisite.begin(), end=prerequisite.end(); it!=end; ++it )
			{
				uint depends = *it;
				hash = Fnv32a(&depends, sizeof(uint), hash);
			}
			for( auto it=postrequisite.begin(), end=postrequisite.end(); it!=end; ++it )
			{
				uint depends = *it;
				hash = Fnv32a(&depends, sizeof(uint), hash);
			}
			this->hash = hash;
		}
		bool operator==( const NodeDependencies& o ) const
		{
			if( hash != o.hash ) return false;
			if( prerequisite.size() != o.prerequisite.size() ) return false;
			if( postrequisite.size() != o.postrequisite.size() ) return false;
			for( uint i=0, end=prerequisite.size(); i!=end; ++i ) { if(prerequisite[i] != o.prerequisite[i]) return false; }
			for( uint i=0, end=postrequisite.size(); i!=end; ++i ) { if(postrequisite[i] != o.postrequisite[i]) return false; }
			return true;
		}
	};
	NodeDependencies* nodeDependencies = eiNewArray(a, NodeDependencies, numNodes);
	for( uint idxNode = 0; idxNode != numNodes; ++idxNode )
	{
		CallNode& node = nodes[idxNode];
		//memory regions used by this node
		for( uint idxMd = node.beginMemoryDependency, endMd = node.endMemoryDependency; idxMd != endMd; ++idxMd )
		{
			const MemoryDependency& md = memory[idxMd];
			eiASSERT( md.idxNode == idxNode );
			//memory regions used by nodes that come before this one in the serial list
			int latestDependency = -1;
			for( uint idxOtherMd = 0; idxOtherMd != node.beginMemoryDependency; ++idxOtherMd )
			{
				const MemoryDependency& otherMd = memory[idxOtherMd];
				eiASSERT( otherMd.idxNode != idxNode );
				// read dependencies want the latest preceeding node in the sequence that writes to the memory
				// write dependencies want the latest preceeding node in the sequence that reads or writes to the memory
				if( otherMd.write == false && md.write == false )
					continue;
				if( md.Overlapping(otherMd) )
					latestDependency = otherMd.idxNode;
			}
			if( latestDependency >= 0 )
			{
				nodeDependencies[idxNode].prerequisite.push_back(latestDependency);
				nodeDependencies[latestDependency].postrequisite.push_back(idxNode);
			}
		}
	}
	
	for( uint idxNode = 0; idxNode != numNodes; ++idxNode )
	{
		nodeDependencies[idxNode].SetHash();
	}
	struct NodeGroup
	{
		NodeDependencies* depends;
		rde::set<uint> prerequisiteGroups;
		rde::vector<uint> nodes;
	};
	NodeGroup* nodeGroups = eiNewArray(a, NodeGroup, numNodes);//worst case alloc
	uint numNodeGroups = 0;
	for( uint idxNode = 0; idxNode != numNodes; ++idxNode )
	{
		int matchingGroup = -1;
		for( uint idxGroup = 0, numGroups = numNodeGroups; idxGroup != numGroups; ++idxGroup )
		{
			if( *nodeGroups[idxGroup].depends == nodeDependencies[idxNode] )
			{
				matchingGroup = idxGroup;
				break;
			}
		}
		if( matchingGroup < 0 )
		{
			matchingGroup = numNodeGroups;
			++numNodeGroups;
			NodeGroup& newGroup = nodeGroups[matchingGroup];
			newGroup.depends = &nodeDependencies[idxNode];
		}
		CallNode& node = nodes[idxNode];
		node.nodeGroup = matchingGroup;
		NodeGroup& group = nodeGroups[matchingGroup];
		group.nodes.push_back(idxNode);

		for( auto itDependency = nodeDependencies[idxNode].prerequisite.begin(), end = nodeDependencies[idxNode].prerequisite.end(); itDependency != end; ++itDependency )
		{
			CallNode& dependency = nodes[*itDependency];
			eiASSERT( dependency.nodeGroup != 0xFFFFFFFF );//should've been put in a group already!
			group.prerequisiteGroups.insert(dependency.nodeGroup);
		}
	}
	eiASSERT( numNodeGroups <= numNodes );
	
	struct FutureCallTask
	{
		FnMethod* call;
		ArgHeader* args;
		void* output;
		const u8* returnData;
		const uint* returnOffsets;
		static void Run( void* o, void* b )
		{
			FutureCallTask& self = *(FutureCallTask*)b;

			if( self.args && self.args->futureMask )
			{
				uint numArgs = 0;
				const uint* offsets;
				const uint* sizes;
				eiASSERT( self.args->info && (void*)self.args->info != eiMAGIC_POINTER(0xCD));
				self.args->info(numArgs, offsets, sizes);
				u8* argTuple = (u8*)(self.args + 1);
				FutureIndex* futureArray = (FutureIndex*)(numArgs ? argTuple + offsets[numArgs-1] + sizes[numArgs-1] : 0);
				
				for( u32 futureMask = self.args->futureMask; futureMask; futureMask = ClearLeastSignificantBit(futureMask) )
				{
					u32 bit = ClearAllButLeastSignificantBit(futureMask);
					uint idx = LeastSignificantBit(futureMask);

					eiASSERT( idx < numArgs );//future bits should match up with arg indices
					uint offset = offsets[idx];//look up where this item is inside the tuple
					uint size   = sizes[idx];//and how many bytes to copy it
					const FutureIndex& future = futureArray[offset];
					uint futureIndex = future.index;
					uint futureOffset = future.offset; eiUNUSED(futureOffset);//TODO - offset into the future value
					//eiASSERT( futureIndex < returnCount );
					eiASSERT( self.returnOffsets[futureIndex] != 0xFFFFFFFF );
					const u8* returnValue = self.returnData + self.returnOffsets[futureIndex];
					eiASSERT( offset < numArgs );
					//todo - support non-POD copy - this callback could be templated to know how
					memcpy( &argTuple[offset], returnValue, size );//copy the return value into the call's arg tuple
				}
			}
			self.call(o, self.args, self.output);
		}
	};
	TaskProto* tasks = eiAllocArray(a, TaskProto, numNodes);
	FutureCallTask* taskData = eiAllocArray(a, FutureCallTask, numNodes);
	for( uint idxNode = 0; idxNode != numNodes; ++idxNode )
	{
		FutureCall& in = *nodes[idxNode].call;
		TaskProto& out = tasks[idxNode];
		FutureCallTask& data = taskData[idxNode];
		data.call = in.call;
		data.args = in.args;
		data.output = nodeReturnAddresses[idxNode];
		data.returnData = returnData;
		data.returnOffsets = returnOffsets;
		out.task = &FutureCallTask::Run;
		out.blob = &data;
		out.blobSize = sizeof(FutureCallTask);
		out.user = in.object;
		out.tag = in.tag;
	}

	TaskGroupProto* taskGroups = eiAllocArray(a, TaskGroupProto, numNodeGroups);
	for( uint idxGroup = 0; idxGroup != numNodeGroups; ++idxGroup )
	{
		const NodeGroup& in = nodeGroups[idxGroup];
		TaskGroupProto& out = taskGroups[idxGroup];

		const uint numTasks = in.nodes.size();
		const uint numDependencies = in.depends->prerequisite.size();
		out.section	= TaskList(numTasks);
		out.tasks = eiAllocArray(a, TaskProto*, numTasks);
		out.numTasks = numTasks;
		out.dependencies = eiAllocArray(a, TaskGroupDepends, numDependencies+1);
		out.numDependencies = numDependencies+1;
		for( uint i=0; i!=numTasks; ++i )
			out.tasks[i] = &tasks[in.nodes[i]];
		for( uint i=0; i!=numDependencies; ++i )
		{
			out.dependencies[i].group = &taskGroups[in.depends->prerequisite[i]];
			out.dependencies[i].prevFrame = false;//todo
		}
		//make every group dependent on itself on the previous frame -- there's only one argument buffer, so two simultaneous invcations would have a race condition in retrieving future argument values.
		out.dependencies[numDependencies].group = &out;
		out.dependencies[numDependencies].prevFrame = true;
	}
	
	TaskGroupProto** taskGroupPointers = eiAllocArray(a, TaskGroupProto*, numNodeGroups);
	for( uint idxGroup = 0; idxGroup != numNodeGroups; ++idxGroup )
	{
		taskGroupPointers[idxGroup] = &taskGroups[idxGroup];
	}
	return MakeSchedule( results, taskGroupPointers, numNodeGroups, true );
}
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

namespace { 
	class Test
	{
	public:
		int DoStuff() { 
			//printf("DoStuff()\n");
			volatile float hax = 0; for( int j=0; j!=1000; ++j ) { hax += sqrtf(j * 42.0f); }
			return 42; 
		}
		void FooBar(int i, const float* p) const { 
			volatile float hax = 0; for( int j=0,end = 1000*(rand()%10)+5000*(GetThreadId()->ThreadIndex()/2+1); j!=end; ++j ) { hax += sqrtf(i * j * *p); }
			eiASSERT(i==42||i==41); //printf("FooBar(%d,%f)\n", i, *p);
		}
	};
}

eiTEST( Message )
{
	Test t;
	
	LocalScope<eiKiB(1)> s;
	CallBuffer q(s.scope);
	
//	LuaPush_Test_FooBar( q, &t, 0 );

	FutureIndex stuff = eiPush( q, t, Test::DoStuff );
	float f = 1337;
	for( uint i=0; i!=100; ++i )
	{
		eiPush( q, t, Test::FooBar, 41, &f );
		eiPush( q, t, Test::FooBar, stuff, &f );
	}

	printf("ExecuteCallBuffer()\n");
//	ExecuteCallBuffer(q, Scope(s, "temp"));
	
	LocalScope<eiKiB(10)> a;
	
	struct ThreadPoolData
	{
		Atomic done;
		Atomic sync;
		Scope* scope;
		CallBuffer* callBuffer;
		TaskSchedule schedule;
	} data = { Atomic(0), Atomic(0xFFFFFFFF), &a.scope, &q };
	auto threadMain = []( void* arg, ThreadId& tid, uint systemId ) -> int
	{
		eight::ProfileBegin();
		ThreadPoolData& data = *(ThreadPoolData*)arg;
		LocalScope<eiKiB(1)> temp;
		if( tid.ThreadIndex() == 0 )
		{
			//eiProfile("BuildCallGraph");
			data.schedule = BuildCallGraph( *data.callBuffer, temp, *data.scope );//send to task prototypes
			eiASSERT( data.sync == 0xFFFFFFFF );
			data.sync = 0;
		}
		//YieldThreadUntil(WaitForValue(data.sync, 0));
		YieldThreadUntil(WaitForGreaterEqual(data.sync, 0));
		
		int i;
		for( i=0; i!=10; ++i )
		{
			eiProfile("Frame");
			{
				eiProfile("Sync");
				if( i )
				YieldThreadUntil(WaitForGreaterEqual(data.sync, (i-1)*tid.NumThreadsInPool()));
			}
			eiProfile("ExecuteSchedule");
			ExecuteSchedule( temp, i, data.schedule, SingleThread(0) );
			++data.sync;
		}
		//	++data.sync;
		{
			YieldThreadUntil(WaitForGreaterEqual(data.sync, i*tid.NumThreadsInPool()));
			
			++data.done;
			YieldThreadUntil(WaitForGreaterEqual(data.done, 4));

			ProfileEnd( g_JsonProfileOut, *data.scope );
		//	++data.sync;
			++data.done;
		}
		return 0;
	};
	eight::InitProfiler(*data.scope, 10240);
		extern Atomic g_profilersRunning;
		g_profilersRunning = 1;
	printf("StartThreadPool()\n");
	StartThreadPool( a, threadMain, &data, 4 );//TODO
	YieldThreadUntil(WaitForValue(data.done, 4*2));
}
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
