//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------

//u32 Fnv32a( const void* begin, const void* end, u32 offsetBasis = 2166136261U );
//u32 Fnv32a( const char* text, u32 offsetBasis = 2166136261U );



inline u32 Fnv32a( const void* begin, const void* end, u32 offsetBasis = 2166136261U )
{
    u32 prime = 16777619;//magic prime 2^24+203
    u32 hash = offsetBasis;
    for( const u8* b=(u8*)begin; b != end; ++b )
    {
        hash ^= *b;
        hash *= prime;
    }
    return hash;
}
inline u32 Fnv32a( const void* begin, const size_t size, u32 offsetBasis = 2166136261U )
{
	return Fnv32a(begin, ((u8*)begin) + size, offsetBasis);
}
inline u32 Fnv32a( const char* text, u32 offsetBasis = 2166136261U )
{
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
inline u16 Fnv16a( const char* text, u32 offsetBasis = 2166136261U )
{
    u32 hash = Fnv32a(text, offsetBasis);
    hash ^= (hash >> 16);
    return (u16)(hash & 0xFFFF);
}


//Compile-time version:
#define eiGX_HASH_(t) ^*(t))*16777619U)
#define eiGX_HASH_1( text) eiGX_HASH_(text)
#define eiGX_HASH_2( text) eiGX_HASH_(text) eiGX_HASH_1( text+1);
#define eiGX_HASH_3( text) eiGX_HASH_(text) eiGX_HASH_2( text+1);
#define eiGX_HASH_4( text) eiGX_HASH_(text) eiGX_HASH_3( text+1);
#define eiGX_HASH_5( text) eiGX_HASH_(text) eiGX_HASH_4( text+1);
#define eiGX_HASH_6( text) eiGX_HASH_(text) eiGX_HASH_5( text+1);
#define eiGX_HASH_7( text) eiGX_HASH_(text) eiGX_HASH_6( text+1);
#define eiGX_HASH_8( text) eiGX_HASH_(text) eiGX_HASH_7( text+1);
#define eiGX_HASH_9( text) eiGX_HASH_(text) eiGX_HASH_8( text+1);
#define eiGX_HASH_10(text) eiGX_HASH_(text) eiGX_HASH_9( text+1);
#define eiGX_HASH_11(text) eiGX_HASH_(text) eiGX_HASH_10(text+1);

#define eiGX_HASH_OPEN_1  ((
#define eiGX_HASH_OPEN_2  (( eiGX_HASH_OPEN_1
#define eiGX_HASH_OPEN_3  (( eiGX_HASH_OPEN_2
#define eiGX_HASH_OPEN_4  (( eiGX_HASH_OPEN_3
#define eiGX_HASH_OPEN_5  (( eiGX_HASH_OPEN_4
#define eiGX_HASH_OPEN_6  (( eiGX_HASH_OPEN_5
#define eiGX_HASH_OPEN_7  (( eiGX_HASH_OPEN_6
#define eiGX_HASH_OPEN_8  (( eiGX_HASH_OPEN_7
#define eiGX_HASH_OPEN_9  (( eiGX_HASH_OPEN_8
#define eiGX_HASH_OPEN_10 (( eiGX_HASH_OPEN_9
#define eiGX_HASH_OPEN_11 (( eiGX_HASH_OPEN_10
//If lengths of greater than 11 are required, just extend the above patterns with more lines...

#define eiFNV32A(text, size) (u32)eiJOIN(eiGX_HASH_OPEN_,size) 2166136261U eiJOIN(eiGX_HASH_,size)(text)
#define eiFNV16A(text, size) (u16)( (eiFNV32A(text,size) ^ (eiFNV32A(text,size)>>16)) & 0xFFFFU )

//Usage:
// u32 Test_fnv32a    = eiFNV32A("Test", 4);
// u16 Example_fnv16a = eiFNV16A("Example", 7);

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
