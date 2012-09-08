#include <eight/core/thread/jobqueue.h>
#include <eight/core/thread/atomic.h>
#include <eight/core/alloc/stack.h>
#include <eight/core/os/win32.h>

using namespace eight;

#include <deque>
struct Job	{	virtual void Execute() = 0;	};

class WorkerThreads
{
public:
	struct ThreadData
	{
		Job* (*GetAJob)(WorkerThreads*);
		WorkerThreads* employer;
		volatile bool exit;
	};

	WorkerThreads()
	{
		m_workerData.GetAJob = &PopJob;
		m_workerData.employer = this;
		m_workerData.exit = false;

		InitializeCriticalSection( &m_jobLock );

		m_worker1 = CreateThread( 0, 0, &ThreadMain, &m_workerData, 0, 0 );
		m_worker2 = CreateThread( 0, 0, &ThreadMain, &m_workerData, 0, 0 );
	}
	~WorkerThreads()
	{
		m_workerData.exit = true;
		WaitForSingleObject( m_worker1, INFINITE );
		WaitForSingleObject( m_worker2, INFINITE );
		DeleteCriticalSection( &m_jobLock );
	}

	void PushJob( Job* job )
	{
		EnterCriticalSection( &m_jobLock );
		m_jobs.push_back( job );
		LeaveCriticalSection( &m_jobLock );
	}
private:
	WorkerThreads( const WorkerThreads& );
	WorkerThreads& operator=( const WorkerThreads& );

	ThreadData m_workerData;
	HANDLE m_worker1, m_worker2;
	CRITICAL_SECTION m_jobLock;
	std::deque<Job*> m_jobs;
	
	static DWORD WINAPI ThreadMain( void* data )
	{
		ThreadData* threadData = (ThreadData*)data;
		while( !threadData->exit )
		{
			Job* job = threadData->GetAJob( threadData->employer );
			if( job )
				job->Execute();
			else
				Sleep(0);
		}
	}

	static Job* PopJob( WorkerThreads* self )
	{
		Job* job = 0;
		EnterCriticalSection( &self->m_jobLock );
		if( !self->m_jobs.empty() )
		{
			job = self->m_jobs.front();
			self->m_jobs.pop_front();
		}
		LeaveCriticalSection( &self->m_jobLock );
		return job;
	}
};