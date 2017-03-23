//------------------------------------------------------------------------------
#pragma once
#include "eight/core/debug.h"
#include "eight/core/math/arithmetic.h"
#include "eight/core/macro.h"
#include "eight/core/noncopyable.h"
#include "eight/core/alloc/scope.h"
#include "eight/core/alloc/new.h"
#include "eight/core/blob/types.h"
#include "eight/core/thread/atomic.h"
#include "eight/core/thread/threadlocal.h"
#include "eight/core/thread/tasksection.h"
namespace eight {
//------------------------------------------------------------------------------

typedef void (FnTask)( void* o, void* b );

struct TaskProto
{
	FnTask* task;
	void*   blob;
	uint    blobSize;
	void*   user;
	const char* tag;
	Address<u8> outBlobAddress;//written to during compilation
};

struct TaskGroupProto;
struct TaskGroupDepends
{
	TaskGroupDepends(TaskGroupProto* g=0, bool p=false) : group(g), prevFrame(p) {}
	TaskGroupProto* group;
	bool prevFrame;
};
struct PreviousFrame : public TaskGroupDepends
{
	explicit PreviousFrame(TaskGroupProto* g=0) : TaskGroupDepends(g, true) {}
};

struct TaskGroupProto
{
	SectionBlob       section;
	TaskProto**       tasks;
	uint              numTasks;
	TaskGroupDepends* dependencies;
	uint              numDependencies;
};

struct Task
{
	FnTask*    task;
	void*      user;
	Offset<u8> blob;
	uint       blobSize;
	const char* tag;
};

struct TaskGroup : public VariableSize
{
	uint DependencyFlagsLength() const { return (dependencies.count+31)/32; }
	uint Bytes() const { return sizeof(*this) + dependencies.DataSize() + dependencyFlags.DataSize(DependencyFlagsLength()); }
	static const uint s_frames = 4;//Different threads can be one +/- 1 frame. Need to be able to write +2 frames ahead.
	SectionBlob             section[s_frames];
	Address<List<Task>>     tasks;
	ArrayOffset<u32>        dependencyFlags;
	List<Offset<TaskGroup>> dependencies;
};

struct TaskScheduleBlob : public BlobRoot
{
	List<Offset<TaskGroup>> groups;
};

struct TaskBlob : public BlobRoot
{
};

struct TaskSchedule
{
	TaskScheduleBlob* scheduleBlob;
	TaskBlob*         taskBlob;
};

TaskSchedule  MakeSchedule( Scope& a, TaskGroupProto** groups, uint count, bool sorted=false );//compiles prototypes into schedule and task blobs
void ExecuteSchedules( Scope& t, uint frame, const TaskSchedule*, uint count,     SingleThread resetThread );
void ExecuteSchedule ( Scope& a, uint frame, const TaskSchedule&,                 SingleThread resetThread );
bool StepSchedule    ( Scope& a, uint frame, const TaskSchedule&, uint* iterator, SingleThread resetThread );//repeat until returns false. *iterator should be 0 on the first iteration. *iterator will be modified.
//resetThread must be constant per frame.
//frame increments up to TaskGroup::s_frames.
//threads are +/- 1 frame apart from each other.

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
