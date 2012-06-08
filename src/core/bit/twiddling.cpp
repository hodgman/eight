//------------------------------------------------------------------------------
#include <eight/core/test.h>
#include <eight/core/debug.h>
#include <eight/core/bit/twiddling.h>
#include <eight/core/timer/timer.h>
#include <eight/core/alloc/stack.h>
#include <stdio.h>
using namespace eight;
//------------------------------------------------------------------------------


static inline uint CountBitsSet_Fallback( u32 i )
{
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return ((i + (i >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}

#ifdef eiBUILD_MSVC
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#ifdef eiBUILD_X64
#pragma intrinsic(_BitScanReverse64)
#endif
uint eight::MostSignificantBit( u32 value )
{
	unsigned long result;
	eiASSERT( value );
	unsigned char anyBits = _BitScanReverse( &result, value );
	eiASSERT( anyBits );
	return result;
}

uint eight::MostSignificantBit( u64 value )
{
	unsigned long result;
	eiASSERT( value );
#ifdef eiBUILD_X64
	unsigned char anyBits = _BitScanReverse64( &result, value );
	eiASSERT( anyBits );
#else
	unsigned long resultHigh, resultLow;
	unsigned char anyBitsHigh = _BitScanReverse( &resultHigh, u32(value>>32) );
	unsigned char anyBitsLow  = _BitScanReverse( &resultLow, u32(value) );
	eiASSERT( anyBitsHigh || anyBitsLow );
	result = anyBitsHigh ? 32 + resultHigh : resultLow;
#endif
	return result;
}

uint eight::LeastSignificantBit( u32 value )
{
	unsigned long result;
	eiASSERT( value );
	unsigned char anyBits = _BitScanForward( &result, value );
	eiASSERT( anyBits );
	return result;
}

uint eight::LeastSignificantBit( u64 value )
{
	unsigned long result;
	eiASSERT( value );
#ifdef eiBUILD_X64
	unsigned char anyBits = _BitScanForward64( &result, value );
	eiASSERT( anyBits );
#else
	unsigned long resultHigh, resultLow;
	unsigned char anyBitsHigh = _BitScanForward( &resultHigh, u32(value>>32) );
	unsigned char anyBitsLow  = _BitScanForward( &resultLow, u32(value) );
	eiASSERT( anyBitsHigh || anyBitsLow );
	result = anyBitsLow ? resultLow : 32 + resultHigh;
#endif
	return result;
}

uint eight::CountBitsSet( u32 i )
{
	return CountBitsSet_Fallback(i);
//	return __popcnt(i);//Support is indicated via the CPUID.01H:ECX.POPCNT[Bit 23] flag
}
uint eight::CountBitsSet( u64 i )
{
	return CountBitsSet_Fallback((u32)i) + CountBitsSet_Fallback((u32)(i>>32));
}

#elif defined(eiBUILD_GCC)

uint eight::CountBitsSet( u32 i )
{
	return (uint)__builtin_popcount(i);
}

#else

uint eight::CountBitsSet( u32 i )
{
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return ((i + (i >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}

#endif

eiTEST(Twiddling)
{
	for( uint i=0; i<32; ++i )
	{
		u32 mask = u32(1)<<i;
		uint bit = MostSignificantBit(mask);
		eiRASSERT( bit == i );
	}
	for( uint i=0; i<64; ++i )
	{
		u64 mask = u64(1)<<i;
		uint bit = MostSignificantBit(mask);
		eiRASSERT( bit == i );
	}

	for( uint i=0; i<8; ++i )
	{
		u8 answer1 = Pow2_8(i);
		u8 answer2 = Pow2_Flat8(i);
		eiRASSERT( answer1 == answer2 );
	}
	for( uint i=0; i<16; ++i )
	{
		u16 answer1 = Pow2_16(i);
		u16 answer2 = Pow2_Flat16(i);
		eiRASSERT( answer1 == answer2 );
	}
	for( uint i=0; i<32; ++i )
	{
		u32 answer1 = Pow2_32(i);
		u32 answer2 = Pow2_Flat32(i);
		eiRASSERT( answer1 == answer2 );
	}
	for( uint i=0; i<64; ++i )
	{
		u64 answer1 = Pow2_64(i);
		u64 answer2 = Pow2_Flat64(i);
		eiRASSERT( answer1 == answer2 );
	}
/*
	u8 buffer[256];
	StackAlloc stack( buffer, 256 );
	Scope a(stack, "");
	
	Timer::InitSystem();
	Timer* t = eiNew(a, Timer);
	int big = 1000000;
	volatile u32 answer;
	double t1 = t->Elapsed(true);
	for( int i=0; i<big; ++i )
	{
		for( uint i=0; i<32; ++i )
		{
			answer = Pow2_32(i);
		}
	}
	double t2 = t->Elapsed(true);
	for( int i=0; i<big; ++i )
	{
		for( uint i=0; i<32; ++i )
		{
			answer = Pow2_Flat32(i);
		}
	}
	double t3 = t->Elapsed(true);
	printf( "microcoded = %f\n", t2-t1 );
	printf( "flattened = %f\n", t3-t2 );
	Timer::ShutdownSystem();
	*/
}
//------------------------------------------------------------------------------