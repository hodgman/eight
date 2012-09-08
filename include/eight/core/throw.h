#pragma once

#ifdef eiBUILD_EXCEPTIONS
#define eiTHROW( a ) throw a
#else
#define eiTHROW( a ) eiASSERT(false&&#a)
#endif