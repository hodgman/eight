//------------------------------------------------------------------------------
#pragma once
#include <eight/core/macro.h>
#include <eight/core/throw.h>
namespace eight {
//------------------------------------------------------------------------------

#define eiENTRY_POINT( fn )						\
	int fn( int argc, char** argv );			\
	int main( int argc, char** argv )			\
	{											\
		int retVal = 0;							\
		bool error = false;						\
		eiBEGIN_CATCH_ALL();					\
		retVal = fn( argc, argv );				\
		eiEND_CATCH_ALL(error);					\
		eiASSERT( !error ); eiUNUSED( error );	\
		return retVal;							\
	}											//

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
