//------------------------------------------------------------------------------
#pragma once
#include <eight/core/debug.h>
#include <eight/core/types.h>
#include <eight/core/noncopyable.h>
#include <eight/core/bit/twiddling.h>
#include <eight/core/thread/latent.h>
namespace eight {
class Scope;
//------------------------------------------------------------------------------


class BaseLatentPipe : NonCopyable//wait-free pipe with 2 frame latency
{
protected:
	BaseLatentPipe( Scope& a, uint capacity, uint sizeOfT, uint padding );
	uint  GetQueueCount() const;
	u32   GetQueueMask() const;
	byte* GetHeadQueue(             uint sizeOfT, uint padding, uint frame);
	byte* GetHeadQueue(uint thread, uint sizeOfT, uint padding, uint frame);
	byte* GetTailQueue(             uint sizeOfT, uint padding, uint frame);
	byte* GetTailQueue(uint thread, uint sizeOfT, uint padding, uint frame);
	uint   m_capacity;
	byte** m_queue;
};
template<class T> class LatentPipe : public BaseLatentPipe
{
protected:
	struct Queue
	{
		uint count;
		static uint Padding() { Queue* t=0; return ((u8*)&t->storage) - ((u8*)(&t->count+1)); }
		T storage;
	};
	LatentPipe( Scope& a, uint capacity ) : BaseLatentPipe(a, capacity, sizeof(T), Queue::Padding()) {}
	Queue* GetHeadQueue(             uint frame)   { return (Queue*)BaseLatentPipe::GetHeadQueue(        sizeof(T), Queue::Padding(), frame); }
	Queue* GetHeadQueue(uint thread, uint frame)   { return (Queue*)BaseLatentPipe::GetHeadQueue(thread, sizeof(T), Queue::Padding(), frame); }
	Queue* GetTailQueue(             uint frame)   { return (Queue*)BaseLatentPipe::GetTailQueue(        sizeof(T), Queue::Padding(), frame); }
	Queue* GetTailQueue(uint thread, uint frame)   { return (Queue*)BaseLatentPipe::GetTailQueue(thread, sizeof(T), Queue::Padding(), frame); }
};

//------------------------------------------------------------------------------

template<class T> class LatentMpsc : LatentPipe<T>
{
public:
	LatentMpsc( Scope& a, uint capacity, ConcurrentFrames::Type frames ) : LatentPipe(a, capacity) { eiASSERT( frames <= ConcurrentFrames::TwoFrames ); } // TODO - create less or more buffers for 1/3 frames
	bool Push(uint frame, const T& item);//if fails, wait a frame before trying again
	bool Pop(uint frame, T*& items, uint& count);
	bool Peek(uint frame, T*& item);
	void Pop(uint frame);
	uint Capacity() const { return m_capacity; }
};

template<class T> class LatentSpmc : LatentPipe<T>
{
public:
	LatentSpmc( Scope& a, uint capacity, ConcurrentFrames::Type frames ) : LatentPipe(a, capacity) { eiASSERT( frames <= ConcurrentFrames::TwoFrames ); } // TODO - create less or more buffers for 1/3 frames
	bool Push(uint frame, const T& item, u32 threadMask=~0U);//if fails, wait a frame before trying again
	void Pop(uint frame, T*& items, uint& count);
	uint Capacity() const { return m_capacity; }
};

//------------------------------------------------------------------------------
#include "latentpipe.hpp"
//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
