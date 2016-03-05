#pragma once

#define eiRESTRICT //todo


#ifndef eiFORCE_INLINE
# if defined(_MSC_VER) && (_MSC_VER >= 1200)
#  define eiFORCE_INLINE __forceinline
# else
#  define eiFORCE_INLINE __inline
# endif
#endif