//------------------------------------------------------------------------------
#include <eight/core/align.h>
#include <eight/core/debug.h>
using namespace eight;
//------------------------------------------------------------------------------

void TodoUnitTest()
{
	eiSTATIC_ASSERT( sizeof(               int    ) == 4  )
	eiSTATIC_ASSERT( sizeof( AtomicAligned<int,1> ) == 16 )
	eiSTATIC_ASSERT( sizeof( AtomicAligned<int,4> ) == 16 )
	eiSTATIC_ASSERT( sizeof( AtomicAligned<int,5> ) == 32 )
	eiSTATIC_ASSERT( sizeof( AtomicAligned<int,8> ) == 32 )
/*	struct 
	{
		char c;
		AtomicAligned<int,5> a;
	}; stuff;*/
}