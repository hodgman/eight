#include <eight/core/thread/task.h>
#include <eight/core/math/arithmetic.h>
#include <eight/core/bit/twiddling.h>
#include <eight/core/sort/radix.h>
#include <eight/core/thread/hashtable.h>
#include <eight/core/macro.h>
#include <stdio.h>
using namespace eight;

//todo
#define eiAssertInRange( in, inSize, blob, blobSize )				\
		eiASSERT( ((u8*)in)        >  ((u8*)blob)          );		\
		eiASSERT( ((u8*)in)+inSize <= ((u8*)blob)+blobSize );		//

namespace { struct SortByDepth
{
	SortByDepth( HashTable<const TaskGroupProto*, int>& depths ) : depths(depths) {} 
	HashTable<const TaskGroupProto*, int>& depths;

	typedef u16 Type;

	u16 operator()( TaskGroupProto** in ) const
	{
		int depth = depths[*in];
		eiASSERT( depth >= 0 && depth <= 0xffff );
		return (u16)depth;
	}
};}

static void BuildDependsDepths(const TaskGroupProto* group, HashTable<const TaskGroupProto*, int>& depths)
{
	int groupDepth = depths[group];
	if( groupDepth != -1 )
		return;
	groupDepth = 0;
	for( const TaskGroupDepends* d=group->dependencies, *end=d+group->numDependencies; d != end; ++d )
	{
		int dependDepth = depths[d->group];
		if( dependDepth == -1 && !d->prevFrame )
		{
			BuildDependsDepths( d->group, depths );
			dependDepth = depths[d->group];
			eiASSERT( dependDepth != -1 );
		}
		groupDepth = max(dependDepth+1, groupDepth);
	}
	depths[group] = groupDepth;
}

static void SortSchedule(Scope& a, TaskGroupProto** groups, uint count)
{
	Scope temp(a, "temp");
	HashTable<const TaskGroupProto*, int> depths(temp, count);
	for( TaskGroupProto** i=groups, **end=i+count; i != end; ++i )
	{
		const TaskGroupProto& group = **i;
		int index = -1;
		depths.Insert(*i, index);
		depths.At(index) = (group.numDependencies == 0) ? 0 : -1;
	//	depths[*i] = (group.numDependencies == 0) ? 0 : -1;
	}
	for( TaskGroupProto** i=groups, **end=i+count; i != end; ++i )
	{
		BuildDependsDepths( *i, depths );
	}
	TaskGroupProto** buffer = eiAllocArray( temp, TaskGroupProto*, count );
	RadixSort( groups, buffer, count, SortByDepth( depths ) );
}
/*void Hax( void* p )
{
	u32* u = (u32*)p;
	u[0] = u[1]&0x3fFFffFF;
}*/

TaskSchedule eight::MakeSchedule( Scope& a_, TaskGroupProto** groups, uint count, bool sorted )
{
	if( !sorted )
		SortSchedule( a_, groups, count );

	StackMeasure measureTasks;
	measureTasks.Alloc<TaskBlob>();

	StackAlloc& a = a_.InternalAlloc();

	TaskScheduleBlob* schedule = eiAlloc(a, TaskScheduleBlob);
	{
		schedule->groups.count = count;
		a.Alloc( schedule->groups.DataSize() );
		// Initialise the schedule and it's task groups. Measure out allocations in the tasks blob for each group.
		for( uint i=0, end=count; i != end; ++i )
		{
			const TaskGroupProto& input = *groups[i];
			TaskGroup data;
			for( uint j=0; j!=TaskGroup::s_frames; ++j )
				data.section[j] = input.section;
			
			//Hax( &data.section[TaskGroup::s_frames-1] );

			List<Task> taskList = { input.numTasks };
			data.tasks.address = (u32)measureTasks.Alloc( taskList.Bytes() );
			data.dependencies.count = input.numDependencies;

			for( uint t=0, end=input.numTasks; t!=end; ++t )
			{
				Address<u8> addr = { (u32)measureTasks.Alloc( input.tasks[t]->blobSize ) };
				input.tasks[t]->outBlobAddress = addr;
			}

			TaskGroup* output = (TaskGroup*)a.Alloc( data.Bytes() );
			*output = data;
			u32* flags = eiAllocArray(a, u32, data.DependencyFlagsLength());
			memset( flags, 0, sizeof(u32)*data.DependencyFlagsLength() );
			output->dependencyFlags = flags;
			schedule->groups[i] = output;
		}
		schedule->bytes = (u32)(a.Mark() - (u8*)schedule);
	}

	uint taskBlobSize = (uint)measureTasks.Bytes();
	TaskBlob* taskBlob = (TaskBlob*)a.Alloc( taskBlobSize );
	taskBlob->bytes = taskBlobSize;
	{
		for( uint i=0, end=count; i != end; ++i )
		{
			TaskGroup& output = *schedule->groups[i];
			const TaskGroupProto& input = *groups[i];

			//write task group dependency lists
			for( uint d=0, end=output.dependencies.count; d != end; ++d )
			{
				const TaskGroupDepends& inDepend = input.dependencies[d];
				eiASSERT( inDepend.group->section.Type() != TaskSectionType::ThreadMask );
				TaskGroup* blobDepend = 0;
				for( uint j=0, end=count; j != end && !blobDepend; ++j )
				{
					if( groups[j] == inDepend.group )
						blobDepend = schedule->groups[j].Ptr();
				}
				eiASSERT( blobDepend && "Dependency wasn't included in the 'groups' input array" );
				output.dependencies[d] = blobDepend;
				output.dependencyFlags[d/32] |= inDepend.prevFrame?(u32(1)<<(d&31)):0;
			}

			//Copy task structs into the task blob
			List<Task>& tasks = *output.tasks.Ptr(taskBlob);
			tasks.count = input.numTasks;
			eiAssertInRange( &tasks, tasks.Bytes(), taskBlob, taskBlob->bytes );
			for( uint t=0, end=input.numTasks; t!=end; ++t )
			{
				const TaskProto& inTask = *input.tasks[t];
				eiASSERT( inTask.task );
				tasks[t].task = inTask.task;
				tasks[t].user = inTask.user;
				tasks[t].blobSize = inTask.blobSize;
				tasks[t].tag = inTask.tag;
				if( !inTask.blobSize )
					tasks[t].blob = 0;
				else
				{
					void* blobOut = (void*)inTask.outBlobAddress.Ptr( taskBlob );
					eiAssertInRange( blobOut, inTask.blobSize, taskBlob, taskBlob->bytes );
					memcpy( blobOut, inTask.blob, inTask.blobSize );
					tasks[t].blob = blobOut;
				}
			}
		}
	}

	TaskSchedule results = { schedule, taskBlob };
	return results;
}

static void RunTask( List<Task>& tasks, uint i )
{
		Task& task = tasks[i];
		eiASSERT( task.task );
		eiProfile( task.tag );
		task.task( task.user, task.blob.Ptr() );
}
static void RunTasks( List<Task>& tasks )
{
	for( uint i=0, end=tasks.count; i != end; ++i )
	{
		RunTask( tasks, i );
	}
}

bool eight::StepSchedule( Scope& a, uint a_frame, const TaskSchedule& data, uint* iterator, SingleThread resetThread )
{
	uint frame = a_frame & 3;
	eiSTATIC_ASSERT( TaskGroup::s_frames == 4 );
	TaskScheduleBlob& schedule = *data.scheduleBlob;
	const TaskBlob* taskBlob = data.taskBlob;
	eiASSERT( iterator && *iterator < schedule.groups.count );
	uint& i = *iterator;

	if( i >= schedule.groups.count )
		return false;
	
	TaskGroup& group = *schedule.groups[i];

	eiBeginSectionThread( resetThread );
	{
		const uint twoFramesAhead = (frame + 2)%TaskGroup::s_frames; eiSTATIC_ASSERT( TaskGroup::s_frames >= 4 ); // assuming threads can be +/- 1 frame in execution.
		eiResetTaskSection( group.section[twoFramesAhead] );
	}
	eiEndSection( resetThread );

	auto WaitForDependencies = [&]()
	{
		for( uint j=0, end=group.dependencies.count; j != end; ++j )
		{
			TaskGroup& depend = *group.dependencies[j];
			uint f = frame;
			if( group.dependencyFlags[j/32] & (u32(1)<<(j&31)) )//todo - move magic numbers. prev frame flag.
			{
				if( eiUnlikely(a_frame==0) )
					continue;//on first frame, can't wait on previous frame
				f = f ? f-1 : TaskGroup::s_frames-1;
			}
			eiWaitForSection( depend.section[f] );
		}
	};

	List<Task>& tasks = *group.tasks.Ptr(taskBlob);
	switch( group.section[frame].Type() )//todo this should be a utility function in tasksection.cpp/h
	{
	case TaskSectionType::TaskSection:
		{
			TaskSection& section = reinterpret_cast<TaskSection&>(group.section[frame]);
			eiBeginSectionTask( section, "StepSchedule" );
				WaitForDependencies();
				::RunTasks( tasks );
			eiEndSection( section );
		}
		break;
	case TaskSectionType::ThreadMask:
		{
			ThreadMask& section = reinterpret_cast<ThreadMask&>(group.section[frame]);
			eiBeginSectionThread( section, "StepSchedule" );
				WaitForDependencies();
				::RunTasks( tasks );
			eiEndSection( section );
		}
		break;
	case TaskSectionType::TaskList:
		{
			TaskList& section = reinterpret_cast<TaskList&>(group.section[frame]);
			bool waited = false;
			eiBeginSectionTaskList( section, "StepSchedule" );
				if( !waited )
				{
					waited = true;
					WaitForDependencies();
				}
				eiASSERT( eiGetTaskDistribution().numWorkers == tasks.count );
				::RunTask( tasks, eiGetTaskDistribution().thisWorker );
			eiEndSection( section );
		}
		break;
	default:
		eiASSERT( false );
		break;
	}

	return ++i, i < schedule.groups.count;
}

void eight::ExecuteSchedule( Scope& a, uint frame, const TaskSchedule& data, SingleThread resetThread )
{
	uint i = 0;
	while( StepSchedule( a, frame, data, &i, resetThread ) ) {};
}

void eight::ExecuteSchedules( Scope& a, uint frame, const TaskSchedule* schedules, uint count, SingleThread resetThread )
{
	Scope temp( a, "temp" );
	uint* iterators = eiAllocArray( temp, uint, count );
	memset( iterators, 0, sizeof(uint)*count );
	bool didWork;
	do
	{
		didWork = false;
		for( int i=0; i!=count; ++i )
		{
			didWork |= StepSchedule( a, frame, schedules[i], &iterators[i], resetThread );
		}
	}while( didWork );
}
