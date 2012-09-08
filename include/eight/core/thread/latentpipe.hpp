//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<class T> bool LatentMpsc<T>::Push(uint frame, const T& item)
{
	Queue* queue = GetHeadQueue(frame);
	if( queue->count < m_capacity )
	{
		(&queue->storage)[queue->count++] = item;
		return true;
	}
	return false;
}
//------------------------------------------------------------------------------
template<class T> bool LatentMpsc<T>::Pop(uint frame, T*& items, uint& count)
{
	for( uint i=0, end=GetQueueCount(); i!=end; ++i )
	{
		Queue* queue = GetTailQueue(i, frame);
		if( queue->count )
		{
			items = &queue->storage;
			count =  queue->count;
			queue->count = 0;
			return true;
		}
	}
	return false;
}
//------------------------------------------------------------------------------
template<class T> bool LatentMpsc<T>::Peek(uint frame, T*& item)
{
	for( uint i=0, end=GetQueueCount(); i!=end; ++i )
	{
		Queue* queue = GetTailQueue(i, frame);
		if( queue->count )
		{
			item = &(&queue->storage)[queue->count-1];
			return true;
		}
	}
	return false;
}
template<class T> void LatentMpsc<T>::Pop(uint frame)
{
	for( uint i=0, end=GetQueueCount(); i!=end; ++i )
	{
		Queue* queue = GetTailQueue(i, frame);
		if( queue->count )
		{
			queue->count--;
			return;
		}
	}
	eiASSERT( false );
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<class T> bool LatentSpmc<T>::Push(uint frame, const T& item, u32 threadMask)
{
	u32 queueMask = GetQueueMask();
	threadMask &= queueMask;
	for( u32 m=threadMask; m; m &= m - 1 )
	{
		uint thread = LeastSignificantBit(m);
		eiASSERT( thread < GetQueueCount() );
		Queue* queue = GetHeadQueue(thread, frame);
		if( queue->count >= m_capacity )
		{//fail to push, undo the push made to previous queues
			for( u32 u=threadMask; u && u!= m; u &= u - 1 )
			{
				thread = LeastSignificantBit(u);
				queue = GetHeadQueue(thread, frame);
				eiASSERT( queue->count > 0 );
				queue->count--;
			}
			return false;
		}
		else
			(&queue->storage)[queue->count++] = item;
	}
	return true;
}
//------------------------------------------------------------------------------
template<class T> void LatentSpmc<T>::Pop(uint frame, T*& items, uint& count)
{
	Queue* queue = GetTailQueue();
	items = &queue->storage;
	count =  queue->count;
	queue->count = 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------