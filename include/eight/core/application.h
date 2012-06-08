//------------------------------------------------------------------------------
#pragma once
#include <eight/core/debug.h>
namespace eight {
//------------------------------------------------------------------------------

//todo - move to exception.h
#ifdef _HAS_EXCEPTIONS
#if _HAS_EXCEPTIONS==0 && defined(eiUSE_EXCEPTIONS)
#pragma message("Warning: eiUSE_EXCEPTIONS was defined with _HAS_EXCEPTIONS==0")
#undef eiUSE_EXCEPTIONS
#endif
#endif

#ifdef eiUSE_EXCEPTIONS
	#define eiBEGIN_CATCH_ALL() try{
	#define eiEND_CATCH_ALL(out)   }catch(...){out = true;}
#else
	#define eiBEGIN_CATCH_ALL()
	#define eiEND_CATCH_ALL(out)
#endif

#define eiENTRY_POINT( fn )						\
	int fn( int argc, char** argv );			\
	int main( int argc, char** argv )			\
	{											\
		int retVal = 0;							\
		bool error = false;						\
		eiBEGIN_CATCH_ALL();					\
		retVal = fn( argc, argv );				\
		eiEND_CATCH_ALL(error);					\
		eiASSERT( !error );						\
		return retVal;							\
	}											//

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
