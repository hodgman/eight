//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/debug.h>
#include <eight/core/intrinsics.h>
#include <eight/core/performance.h>
#include <eight/core/macro.h>

#if defined(eiBUILD_WINDOWS) || defined(eiBUILD_XB360)
# pragma warning( push )
# pragma warning( disable: 4146 ) // warning C4146: unary minus operator applied to unsigned type, result still unsigned
#endif


#if defined(eiBUILD_PS4) || defined(eiBUILD_XBONE) || ( defined(eiBUILD_PS3) && defined(__SPURS_JOB__) )
#define PLATFORM_HAS_POPCNT
#endif

//------------------------------------------------------------------------------
namespace eight {
//------------------------------------------------------------------------------

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
//------------------------------------------------------------------------------

eiFORCE_INLINE u32 CountLeadingZeros(u32 value)
{
#if defined (eiBUILD_WINDOWS)
	unsigned long mostSignificantBit;
	char anyBitsSet = _BitScanReverse( &mostSignificantBit, value );
	return anyBitsSet ? 31-mostSignificantBit : 32;
#elif defined (eiBUILD_PS4)
	return __lzcnt32(value);
#elif defined(eiBUILD_XBONE)
	return _lzcnt_u32(value);
#elif defined (eiBUILD_XB360)
	return _CountLeadingZeros(value);
#elif defined (eiBUILD_PS3) && defined(__SPURS_JOB__)
	__vector unsigned int a;
	a[0] = value;
	__vector unsigned int result = spu_cntlz(a); 
	return result[0];
#elif defined(eiBUILD_PS3) && !defined(__SPURS_JOB__)
	return __cntlzw(value);
#else
#	pragma message("port me, please!")
	//very inefficient - can use as a fallback for ports to new platforms
	if (value == 0)
		return 32;
	unsigned int count;
	for( count = 0; (value & 0x80000000)==0; ++count, value <<= 1 ) {}
	return count;
#endif
}

eiFORCE_INLINE u64 CountLeadingZeros(u64 value)
{
#if defined(eiBUILD_WINDOWS) && defined(eiBUILD_64BIT)
	return __lzcnt64(value);
#elif defined(eiBUILD_PS4)
	return __lzcnt64(value);
#elif defined(eiBUILD_XBONE)
	return _lzcnt_u64(value);
#elif defined(eiBUILD_XB360)
	return _CountLeadingZeros64(value);
#elif defined (eiBUILD_PS3) && !defined(__SPURS_JOB__)
	return __cntlzd(value);
#elif defined (eiBUILD_PS3) && defined(__SPURS_JOB__)
	__vector unsigned int a;
	a[0] = value;
	a[1] = value >> 32;
	__vector unsigned int result = spu_cntlz(a); 
	return ((value>>32) & 0xFFffFFff) ? result[1] : result[0];
#else
	//if no 64bit instruction, implement in terms of the 32 bit version
	u32 low = u32(value & 0xFFffFFff);
	u32 high = u32((value>>32) & 0xFFffFFff);
	return high ? CountLeadingZeros(high) : 32 + CountLeadingZeros(low);
#endif
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

inline u32 CountBitsSet( u32 i )
{
#if defined(eiBUILD_PS4) || defined(eiBUILD_XBONE)
	return _mm_popcnt_u32(i);
#elif defined(eiBUILD_PS3) && defined(__SPURS_JOB__)
	__vector unsigned int a;
	a[0] = x;
	a[1] = 0;
	a[2] = 0;
	a[3] = 0;
	__vector unsigned int result = spu_cntb(a); 
	return result[0];
#else
	//version for platforms without a popcnt instruction
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return ((i + (i >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
	//TODO - on PC, To determine hardware support for the popcnt instruction, call the __cpuid intrinsic with InfoType=0x00000001 and check bit 23 of CPUInfo[2] (ECX). This bit is 1 if the instruction is supported, and 0 otherwise. If you run code that uses this intrinsic on hardware that does not support the popcnt instruction, the results are unpredictable.
#endif
}

eiFORCE_INLINE u32 CountBitsSet_14bit( u32 i )
{
	eiASSERT( (i & ~0x7FFFU)==0 );//only the lower 14 bits supported
#if defined(eiBUILD_64BIT_MATH) && !defined(eiBUILD_HAS_POPCNT)
	//version for platforms without a popcnt instruction but have fast 64-bit math
	return (u32)((i * 0x200040008001ULL & 0x111111111111111ULL) % 0xF);
#else
	return CountBitsSet(i);
#endif
}

inline u32 CountBitsSet_8bit( u32 i )
{
	eiASSERT( (i & ~0xFFU)==0 );//only the lower 8 bits supported
	/*
#if !defined(eiBUILD_HAS_POPCNT)
	//platforms without a popcnt instruction
	//TODO - CountBitsSet_14bit may actually be faster than this -- a few ALU instead of the cache miss?
	static const unsigned char s_BitsSetTable256[256] = 
	{
#   define eiB2(n) n,        n+1,        n+1,        n+2
#   define eiB4(n) eiB2(n), eiB2(n+1), eiB2(n+1), eiB2(n+2)
#   define eiB6(n) eiB4(n), eiB4(n+1), eiB4(n+1), eiB4(n+2)
		            eiB6(0), eiB6(  1), eiB6(  1), eiB6(  2)
	};
#undef eiB6
#undef eiB4
#undef eiB2
	return s_BitsSetTable256[i];
#else*/
	return CountBitsSet_14bit(i);
//#endif
}

inline u32 CountBitsSet_24bit( u32 i )
{
	eiASSERT( (i & ~0xFFFFFF)==0 );//only the lower 24 bits supported
#if defined(eiBUILD_64BIT_MATH) && !defined(eiBUILD_HAS_POPCNT)
	//version for platforms without a popcnt instruction but have fast 64-bit math
	u32 c  =  ((i & 0xfff) * 0x1001001001001ULL & 0x84210842108421ULL) % 0x1f;
	c += (((i & 0xfff000) >> 12) * 0x1001001001001ULL & 0x84210842108421ULL) % 0x1f;
	return c;
#else
	return CountBitsSet(i);
#endif
}

eiFORCE_INLINE uint CountBitsSet( u64 i )
{
#if defined(eiBUILD_PS4) || defined(eiBUILD_XBONE)
	return _mm_popcnt_u64(i);
#else
	//if no 64bit instruction, implement in terms of the 32 bit version
	return CountBitsSet((u32)(i&0xFFFFFFFFU) + CountBitsSet((u32)((i>>32)&0xFFFFFFFFU)));
#endif
}

eiFORCE_INLINE u32 CountBitsSet( u16 i )
{
	return CountBitsSet_24bit( (u32)i );
}

eiFORCE_INLINE u32 CountBitsSet( u8 i )
{
	return CountBitsSet_8bit( (u32)i );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

eiFORCE_INLINE u32 ClearLeastSignificantBit(u32 in)
{
#if defined(eiBUILD_PS4)
	return __blsr_u32(in);
#elif defined(eiBUILD_XBONE)
	return _blsr_u32(in);
#else
	return in & (in-1);
#endif
}

eiFORCE_INLINE u64 ClearLeastSignificantBit(u64 in)
{
#if defined(eiBUILD_PS4)
	return __blsr_u64(in);
#elif defined(eiBUILD_XBONE)
	return _blsr_u64(in);
#else
	return in & (in-1);
#endif
}

eiFORCE_INLINE u32 ClearAllButLeastSignificantBit(u32 in)
{
#if defined(eiBUILD_PS4)
	return __blsi_u32(in);
#elif defined(eiBUILD_XBONE)
	return _blsi_u32(in);
#else
	return in & -in;//unary minus operator applied to unsigned type, result still unsigned
#endif
}

eiFORCE_INLINE u64 ClearAllButLeastSignificantBit(u64 in)
{
#if defined(eiBUILD_PS4)
	return __blsi_u64(in);
#elif defined(eiBUILD_XBONE)
	return _blsi_u64(in);
#else
	return in & -in;//unary minus operator applied to unsigned type, result still unsigned
#endif
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


eiFORCE_INLINE u32 MostSignificantBit( u32 value )
{
	eiASSERT( value != 0 );
#if defined(eiBUILD_WINDOWS) || defined(eiBUILD_XBONE)
	unsigned long result;
	_BitScanReverse( &result, value );
	return result;
#else //other platforms
	//TODO - PS4 has no BSR/BSF intrinsics??? implement in terms of LZCNT?
	return 31-CountLeadingZeros(value);
#endif
}

eiFORCE_INLINE u32 MostSignificantBit( u64 value )
{
	unsigned long result;
	eiASSERT( value != 0 );
#if defined(eiBUILD_WINDOWS) || defined(eiBUILD_XBONE)
#if defined(eiBUILD_64BIT)
	eiDEBUG( unsigned char retval = )_BitScanReverse64( &result, value );
	eiASSERT( retval != 0 );
# else
	unsigned long resultHigh, resultLow;
	unsigned char anyBitsHigh = _BitScanReverse( &resultHigh, u32((value>>32)&0xFFFFFFFF) );
	_BitScanReverse( &resultLow, u32(value&0xFFFFFFFF) );
	result = anyBitsHigh ? (32 + resultHigh) : resultLow;
# endif
#else//other platforms
	//TODO - PS4 has no BSR/BSF intrinsics??? implement in terms of LZCNT?
# if defined(eiBUILD_64BIT_MATH)
	result = 63-(u32)CountLeadingZeros( value );
# else
	u32 low = u32(value & 0xFFffFFff);
	u32 high = u32((value>>32) & 0xFFffFFff);
	u32 resultHigh = 31-CountLeadingZeros( high );
	u32 resultLow  = 31-CountLeadingZeros( low );
	result = high ? (32 + resultHigh) : resultLow;
# endif
#endif
	return result;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

eiFORCE_INLINE u32 LeastSignificantBit( u32 value )
{
	eiASSERT( value != 0 );
#if defined(eiBUILD_WINDOWS)
	unsigned long result;
	_BitScanForward( &result, value );
	return result;;
#elif defined(eiBUILD_PS4)
	return __tzcnt_u32(value);
#elif defined(eiBUILD_XBONE)
	return _tzcnt_u32(value);
#else //PPC has no TZCNT/BSF, but this works:
	return 31 - CountLeadingZeros(ClearAllButLeastSignificantBit(value));
#endif
}

eiFORCE_INLINE u32 LeastSignificantBit( u64 value )
{
	eiASSERT( value != 0 );
#if defined(eiBUILD_WINDOWS) || defined(eiBUILD_XBONE)
	unsigned long result;
# if defined(eiBUILD_64BIT_MATH)
	unsigned char anyBits = _BitScanForward64( &result, value );
	eiASSERT( anyBits != 0 ); eiUNUSED( anyBits );
# else
	unsigned long resultHigh, resultLow;
	unsigned char anyBitsHigh = _BitScanForward( &resultHigh, u32((value>>32)&0xFFFFFFFF) );
	unsigned char anyBitsLow  = _BitScanForward( &resultLow, u32(value&0xFFFFFFFF) );
	eiASSERT( anyBitsHigh || anyBitsLow ); eiUNUSED(anyBitsHigh);
	result = anyBitsLow ? resultLow : (32 + resultHigh);
# endif
	return result;
#elif defined(eiBUILD_PS4)
	return __tzcnt_u64(value);
#elif defined(eiBUILD_64BIT_MATH) //PPC has no TZCNT/BSF, but this works:
	return 63 - (u32)CountLeadingZeros(ClearAllButLeastSignificantBit(value));
#else
	//if no 64bit instruction, implement in terms of the 32 bit version
	u32 low = u32(value&0xFFFFFFFF);
	u32 high = u32((value>>32)&0xFFFFFFFF);
	return low ? LeastSignificantBit(low) : 32 + LeastSignificantBit(high);
#endif
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

eiFORCE_INLINE uint BitIndex( u32 value )
{
	eiASSERT( LeastSignificantBit( value ) == MostSignificantBit( value ) );
	return LeastSignificantBit( value );
}

eiFORCE_INLINE void SetOrClearBits(u32& inout, u32 bits, bool set)
{
	//if (set) inout |= bits; else inout &= ~bits; 
	inout = (inout & ~bits) | ((u32)set * bits);
}

eiFORCE_INLINE u32 BranchlessMin(u32 a, u32 b)
{
	s32 diff = a - b;
	s32 signMask = diff >> 31;
	s32 result = (s32)b + (diff & signMask);
	return (u32)result;
}

eiFORCE_INLINE u32 KeepEverySecondBit(u64 key)
{
	key &= 0x5555555555555555;//throw out every second bit -- 0a0b0c0d0e0f0g0h0i0j0k0l0m0n0o0p0A0B0C0D0E0F0G0H0I0J0K0L0M0N0O0P
	key = (key | (key >> 1)) & 0x3333333333333333;//          00ab00cd00ef00gh00ij00kl00mn00op00AB00CD00EF00GH00IJ00KL00MN00OP
	key = (key | (key >> 2)) & 0x0f0f0f0f0f0f0f0f;//          0000abcd0000efgh0000ijkl0000mnop0000ABCD0000EFGH0000IJKL0000MNOP
	key = (key | (key >> 4)) & 0x00ff00ff00ff00ff;//          00000000abcdefgh00000000ijklmnop00000000ABCDEFGH00000000IJKLMNOP
	key = (key | (key >> 8)) & 0x0000ffff0000ffff;//          0000000000000000abcdefghijklmnop0000000000000000ABCDEFGHIJKLMNOP
	key = (key | (key >>16)) & 0x00000000ffffffff;//          00000000000000000000000000000000abcdefghijklmnopABCDEFGHIJKLMNOP
	return (u32)key;
}

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------

#if defined(eiBUILD_WINDOWS) || defined(eiBUILD_XB360)
# pragma warning( pop )
#endif