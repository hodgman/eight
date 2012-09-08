//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/debug.h>
namespace eight {
//------------------------------------------------------------------------------

uint MostSignificantBit( u32 );
uint MostSignificantBit( u64 );
uint LeastSignificantBit( u32 );
uint LeastSignificantBit( u64 );

inline uint BitIndex( u32 value )
{
	eiASSERT( LeastSignificantBit( value ) == MostSignificantBit( value ) );
	return LeastSignificantBit( value );
}

uint CountBitsSet( u32 i );
uint CountBitsSet( u64 i );

inline u8 Pow2_8( int i )
{
	return ((u8)1)<<i;
}
inline u16 Pow2_16( int i )
{
	return ((u16)1)<<i;
}
inline u32 Pow2_32( int i )
{
	return ((u32)1)<<i;
}
inline u64 Pow2_64( int i )
{
	return ((u64)1)<<i;
}
inline u32 Pow2_Flat8( int i )
{
	u8 l = (u8)i;
	u8 t0 = ((u8)-((s8)((l>>0)&(u8)1)))^(u8)0x55;
	u8 t1 = ((u8)-((s8)((l>>1)&(u8)1)))^(u8)0x33;
	u8 t2 = ((u8)-((s8)((l>>2)&(u8)1)))^(u8)0x0F;
	u8 t3 = ((u8)-((s8)((l>>3)&(u8)1)))^(u8)0xFF;
	return t0 & t1 & t2 & t3;
}
inline u32 Pow2_Flat16( int i )
{
	u16 l = (u16)i;
	u16 t0 = ((u16)-((s16)((l>>0)&(u16)1)))^(u16)0x5555;
	u16 t1 = ((u16)-((s16)((l>>1)&(u16)1)))^(u16)0x3333;
	u16 t2 = ((u16)-((s16)((l>>2)&(u16)1)))^(u16)0x0F0F;
	u16 t3 = ((u16)-((s16)((l>>3)&(u16)1)))^(u16)0x00FF;
	u16 t4 = ((u16)-((s16)((l>>4)&(u16)1)))^(u16)0xFFFF;
	return t0 & t1 & t2 & t3 & t4;
}
inline u32 Pow2_Flat32( int i )
{
	u32 l = (u32)i;
	u32 t0 = ((u32)-((s32)((l>>0)&(u32)1)))^(u32)0x55555555;
	u32 t1 = ((u32)-((s32)((l>>1)&(u32)1)))^(u32)0x33333333;
	u32 t2 = ((u32)-((s32)((l>>2)&(u32)1)))^(u32)0x0F0F0F0F;
	u32 t3 = ((u32)-((s32)((l>>3)&(u32)1)))^(u32)0x00FF00FF;
	u32 t4 = ((u32)-((s32)((l>>4)&(u32)1)))^(u32)0x0000FFFF;
	u32 t5 = ((u32)-((s32)((l>>5)&(u32)1)))^(u32)0xFFFFFFFF;
	return t0 & t1 & t2 & t3 & t4 & t5;
}
inline u64 Pow2_Flat64( int i )
{
	u64 l = (u64)i;
	u64 t0 = ((u64)-((s64)((l>>0)&(u64)1)))^(u64)0x5555555555555555;
	u64 t1 = ((u64)-((s64)((l>>1)&(u64)1)))^(u64)0x3333333333333333;
	u64 t2 = ((u64)-((s64)((l>>2)&(u64)1)))^(u64)0x0F0F0F0F0F0F0F0F;
	u64 t3 = ((u64)-((s64)((l>>3)&(u64)1)))^(u64)0x00FF00FF00FF00FF;
	u64 t4 = ((u64)-((s64)((l>>4)&(u64)1)))^(u64)0x0000FFFF0000FFFF;
	u64 t5 = ((u64)-((s64)((l>>5)&(u64)1)))^(u64)0x00000000FFFFFFFF;
	u64 t6 = ((u64)-((s64)((l>>6)&(u64)1)))^(u64)0xFFFFFFFFFFFFFFFF;
	return t0 & t1 & t2 & t3 & t4 & t5 & t6;
}


//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
