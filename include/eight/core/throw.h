#pragma once
#include <eight/core/debug.h>

#ifdef _HAS_EXCEPTIONS
# if _HAS_EXCEPTIONS==0 && defined(eiBUILD_EXCEPTIONS)
#  pragma message("Warning: eiBUILD_EXCEPTIONS was defined with _HAS_EXCEPTIONS==0")
#  undef eiBUILD_EXCEPTIONS
# endif
#endif

#ifdef eiBUILD_EXCEPTIONS
# define eiTHROW( a ) throw a
# define eiBEGIN_CATCH_ALL() try{
# define eiEND_CATCH_ALL(out)   }catch(...){out = true;}
#else
# define eiTHROW( a ) eiASSERT( false && #a )
# define eiBEGIN_CATCH_ALL()
# define eiEND_CATCH_ALL(out)
#endif