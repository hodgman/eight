#pragma once

#ifdef eiBUILD_EXCEPTIONS
#define THROW( a ) throw a
#else
#define THROW( a ) 
#endif