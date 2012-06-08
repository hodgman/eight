//------------------------------------------------------------------------------
#pragma once
namespace eight {
//------------------------------------------------------------------------------
#define eiALIGN( type, bytes )		 __declspec( align( bytes ) ) type

#define eiATOMIC_ALIGNMENT			 16

template<class T, int I>
struct eiALIGN( AtomicAligned, eiATOMIC_ALIGNMENT )
{
	eiALIGN( T data[I], eiATOMIC_ALIGNMENT );
};
//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
