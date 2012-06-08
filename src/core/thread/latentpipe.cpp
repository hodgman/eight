#include <eight/core/thread/latentpipe.h>
#include <eight/core/thread/tasksection.h>
#include <eight/core/alloc/new.h>
#include <eight/core/alloc/scope.h>

using namespace eight;


uint PoolSize()
{
	eiASSERT( static_cast<internal::ThreadId*>(internal::_ei_thread_id) );
	return internal::_ei_thread_id->poolSize;
}

BaseLatentPipe::BaseLatentPipe( Scope& a, uint capacity, uint sizeOfT, uint padding )
	: m_capacity(capacity)
	, m_queue( eiAllocArray(a, byte*, PoolSize()) )
{
	uint sizeofQueue = sizeof(uint) + padding + m_capacity*sizeOfT;
	uint sizeof4FrameQueue = 4 * sizeofQueue;
	uint sizeofShared4FrameQueue = PoolSize() * sizeof4FrameQueue;
	byte* queueStorage = eiAllocArray( a, byte, sizeofShared4FrameQueue );
	for( int i=0; i!=internal::_ei_thread_id->poolSize; ++i )
	{
		m_queue[i] = queueStorage + i*sizeof4FrameQueue;
		for( int frame=0; frame!=4; ++frame )
		{
			*(uint*)(m_queue[i] + sizeofQueue * frame) = 0;
		}
	}
}

uint BaseLatentPipe::GetQueueCount() const
{
	return internal::_ei_thread_id->poolSize;
}
u32  BaseLatentPipe::GetQueueMask() const
{
	eiASSERT( internal::_ei_thread_id->poolMask == (1<<(1+GetQueueCount()))-1 );
	return internal::_ei_thread_id->poolMask;
}
byte* BaseLatentPipe::GetHeadQueue(uint sizeOfT, uint padding, uint frame)
{
	uint thread = internal::_ei_thread_id->index;
	return GetHeadQueue(thread, sizeOfT, padding, frame);
}
byte* BaseLatentPipe::GetHeadQueue(uint thread, uint sizeOfT, uint padding, uint frame)
{
	//other threads can be 1 frame behind or 1 frame ahead, so
	frame = (frame+2)%4;//write 2 frames ahead
	uint sizeofQueue = sizeof(uint) + padding + m_capacity*sizeOfT;
	return m_queue[thread] + sizeofQueue * frame;
}
byte* BaseLatentPipe::GetTailQueue(uint sizeOfT, uint padding, uint frame)
{
	uint thread = internal::_ei_thread_id->index;
	return GetTailQueue(thread, sizeOfT, padding, frame);
}
byte* BaseLatentPipe::GetTailQueue(uint thread, uint sizeOfT, uint padding, uint frame)
{
	frame = frame%4;
	eiASSERT( thread < internal::_ei_thread_id->poolSize );
	uint sizeofQueue = sizeof(uint) + padding + m_capacity*sizeOfT;
	return m_queue[thread] + sizeofQueue * frame;
}
