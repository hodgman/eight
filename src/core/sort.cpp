#include <eight/core/sort/radix.h>
#include <eight/core/debug.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/timer/timer.h>
#include <ctime>
#include <vector>
#include <algorithm>

using namespace eight;

enum DataSource
{
	Random,
	Sorted,
	Reverse,
};

static Timer* g_t = 0;

int g_x = 1, g_y = 2;
template<class T>
bool Pred ( T elem1, T elem2 )
{
	return elem1 == elem2 ? (elem1+g_x == elem2+g_x ? elem1*g_y > elem2*g_y : elem1+g_x > elem2+g_x)
	   : elem1 >= elem2;
}

template<class T>
void TestSort( int runs, int count, DataSource data, float& r8, float& r16, float& q )
{
	float total1=0, total2=0, total3=0;
	for( int i=0; i<runs; ++i )
	{
		std::vector<T> a1, a2, a3, b;
		a1.resize( count );
		a2.resize( count );
		a3.resize( count );
		b .resize( count );
		if( data == Random )
		{
			for( int i=0; i!=count; ++i )
			{
				T r = (T)rand();
				r = r < 0 ? -r : r;
				a1[i] = a2[i] = a3[i] = r < 0 ? 0 : r;
			}
		}
		else if( data == Sorted )
		{
			for( int i=0; i!=count; ++i )
			{
				T r = (T)i;
				r = r < 0 ? -r : r;
				a1[i] = a2[i] = a3[i] = r < 0 ? 0 : r;
			}
		}
		else if( data == Reverse )
		{
			for( int i=0; i!=count; ++i )
			{
				T r = (T)(count-i);
				r = r < 0 ? -r : r;
				a1[i] = a2[i] = a3[i] = r < 0 ? 0 : r;
			}
		}
		else
		{
	//		ASSERT( false )
		}
		for( int i=0; i!=count; ++i )
		{
	//		ASSERT( a1[i] >= 0 )
	//		ASSERT( a2[i] >= 0 )
	//		ASSERT( a3[i] >= 0 )
		}

		double start;
		start = g_t->Elapsed();
		b.resize( a1.size() );
		RadixSort( &a1[0], &b[0], (uint)a1.size() );
		total1 = (float)(g_t->Elapsed()-start);
/*
		start = g_t->Elapsed();
		RadixSort16( a2, b );
		total2 = (float)(g_t->Elapsed()-start);
*/
		start = g_t->Elapsed();
		std::sort( a3.begin(), a3.end(), Pred<T> );
		total3 = (float)(g_t->Elapsed()-start);

	/*	T prev1 = 0, prev2 = 0, prev3 = 0;
		for( int i=0; i!=count; ++i )
		{
			eiRASSERT( prev1 <= a1[i] );
			eiRASSERT( prev2 <= a2[i] );
			eiRASSERT( prev3 <= a3[i] );
			prev1 = a1[i];
			prev2 = a2[i];
			prev3 = a3[i];
		}*/
	}
	r8  = total1 / runs;
	r16 = total2 / runs;
	q   = total3 / runs;
}

void TODO_test_sort(Scope& temp)
{
	//observation, if count is smaller than ~200, quick sort wins
	//             if count is smaller than ~65k, radix 8 wins
	//             if count is *massive*, radix16 starts to become feasible
	float total1=0, total2=0, total3=0;

	Scope a(temp, "temp");
	g_t = eiNewInterface(a,Timer);
	
#if 0
	int runs = 1;
	int count = 0xFFF;

	TestSort<int8>  ( runs, count, Random, total1, total2, total3 );
	TestSort<int16> ( runs, count, Random, total1, total2, total3 );
	TestSort<int64> ( runs, count, Random, total1, total2, total3 );
	TestSort<int32> ( runs, count, Random, total1, total2, total3 );
	TestSort<double>( runs, count, Random, total1, total2, total3 );
	TestSort<float> ( runs, count, Random, total1, total2, total3 );

#else
	int runs = 100;
	int count = 1000000;//0xF;
	
	//printf( "Data, Input, Items, Radix8 ms, Radix16 ms, QuickSort ms\n" );
	printf( "Algorithm, Items, Input, Time ms\n" );

	for( int i=0; i!=5; ++i )
	{/*
		TestSort<int64>( runs, count, Random, total1, total2, total3 );
		printf( "int64, Random, %d, %f, %f, %f\n", count, total1, total2, total3 );
		TestSort<int64>( runs, count, Sorted, total1, total2, total3 );
		printf( "int64, Sorted, %d, %f, %f, %f\n", count, total1, total2, total3 );
		TestSort<int64>( runs, count, Reverse, total1, total2, total3 );
		printf( "int64, Reverse, %d, %f, %f, %f\n", count, total1, total2, total3 );

		TestSort<int32>( runs, count, Random, total1, total2, total3 );
		printf( "int32, Random, %d, %f, %f, %f\n", count, total1, total2, total3 );
		TestSort<int32>( runs, count, Sorted, total1, total2, total3 );
		printf( "int32, Sorted, %d, %f, %f, %f\n", count, total1, total2, total3 );
		TestSort<int32>( runs, count, Reverse, total1, total2, total3 );
		printf( "int32, Reverse, %d, %f, %f, %f\n", count, total1, total2, total3 );
		*/
		TestSort<int32>( runs, count, Random, total1, total2, total3 );
		printf( "Radix8, %d, Random, %f\n", count, total1 );
	//	printf( "Radix16, %d, Random, %f\n", count, total2 );
		printf( "QuickSort, %d, Random, %f\n", count, total3 );
		TestSort<int32>( runs, count, Sorted, total1, total2, total3 );
		printf( "Radix8, %d, Sorted, %f\n", count, total1 );
	//	printf( "Radix16, %d, Sorted, %f\n", count, total2 );
		printf( "QuickSort, %d, Sorted, %f\n", count, total3 );
		TestSort<int32>( runs, count, Reverse, total1, total2, total3 );
		printf( "Radix8, %d, Reverse, %f\n", count, total1 );
	//	printf( "Radix16, %d, Reverse, %f\n", count, total2 );
		printf( "QuickSort, %d, Reverse, %f\n", count, total3 );

		count <<= 4;
		count |= 0xFF;
	}

	int asdf = 23; (void)asdf;
	asdf = 5454;(void)asdf;
#endif
}