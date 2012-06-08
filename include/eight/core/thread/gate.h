//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/debug.h>
#include <eight/core/thread/tasksection.h>
namespace eight {
//------------------------------------------------------------------------------

class Gate
{
public:
	Gate( TaskSection& wait ) : wait(&wait) {}
	void PassGate()
	{
		BusyWait spinner;
		if( 1 != gate )//if gate not open
		{
			do
			{
				if( gate.SetIfEqual(-1,0) )//try move from closed to locked
				{
					u32 setTo;
					if( internal::IsTaskSectionDone(*wait) )
						setTo = 1;//move from locked to open
					else
						setTo = 0;//move from locked to closed
					eiDEBUG( bool ok = (gate.SetIfEqual(setTo,-1)) );
					eiASSERT( ok );
					eiDEBUG( if( !ok ) )
					gate = setTo;
				}
			} while( !spinner.Try(WaitFor1(gate)) );//while gate not open
		}
		passed++;
	}
private:
	Atomic gate;
	Atomic passed;
	TaskSection* wait;
};


//------------------------------------------------------------------------------
//#include ".hpp"
} // namespace eight
//------------------------------------------------------------------------------
