//------------------------------------------------------------------------------
#pragma once
namespace eight {
//------------------------------------------------------------------------------
#ifdef _MSC_VER
# define eiALIGN(bytes)      __declspec( align(bytes) )
#else
# define eiALIGN(bytes)		 __attribute__ ((aligned (bytes)))
#endif

#define eiATOMIC_ALIGNMENT			 16 // todo, check build architecture

template<class T, int I>
struct eiALIGN(eiATOMIC_ALIGNMENT) AtomicAligned
{
	eiALIGN(eiATOMIC_ALIGNMENT) T data[I];
};
//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
