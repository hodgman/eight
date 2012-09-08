//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------

u32 Fnv32a( const void* begin, const void* end );
u32 Fnv32a( const char* text );



inline u32 Fnv32a( const void* begin, const void* end )
{
    u32 offsetBasis = 2166136261;//magic salt
    u32 prime = 16777619;//magic prime 2^24+203
    u32 hash = offsetBasis;
    for( const u8* b=(u8*)begin; b != end; ++b )
    {
        hash ^= *b;
        hash *= prime;
    }
    return hash;
}
inline u32 Fnv32a( const char* text )
{
    u32 offsetBasis = 2166136261;//magic salt
    u32 prime = 16777619;//magic prime 2^24+203
    u32 hash = offsetBasis;
	if( text )
	{
		for( const u8* b=(u8*)text; *b; ++b )
		{
			hash ^= *b;
			hash *= prime;
		}
	}
    return hash;
}

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
