#include <eight/core/math/frustum.h>
#include <eight/core/alloc/new.h>
#include <eight/core/alloc/malloc.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/test.h>
#include <eight/core/macro.h>
#include <eight/core/timer/timer.h>
#include <eight/core/thread/atomic.h>
#include "frustum_ispc.h"
#include <xmmintrin.h>
#include <emmintrin.h>
#include <math.h>
//------------------------------------------------------------------------------
using namespace eight;
//------------------------------------------------------------------------------

void FrustumPlanes::Sphere4InFrustum( const Vec4& x_, const Vec4& y_, const Vec4& z_, const Vec4& w_, Vec4i& output_ )
{
	__m128 x = _mm_load_ps(x_.elements);
	__m128 y = _mm_load_ps(y_.elements);
	__m128 z = _mm_load_ps(z_.elements);
	__m128 w = _mm_load_ps(w_.elements);
	__m128i inside = _mm_set1_epi32(~0U);
	for ( uint i = 0; i < 6; i++ )
	{
		__m128 plane = _mm_load_ps(planes[i].elements);
		__m128 px = _mm_shuffle_ps(plane, plane, _MM_SHUFFLE(0,0,0,0));
		__m128 py = _mm_shuffle_ps(plane, plane, _MM_SHUFFLE(1,1,1,1));
		__m128 pz = _mm_shuffle_ps(plane, plane, _MM_SHUFFLE(2,2,2,2));
		__m128 pw = _mm_shuffle_ps(plane, plane, _MM_SHUFFLE(3,3,3,3));
		__m128 dots = _mm_add_ps (
			_mm_add_ps( _mm_mul_ps(px,x), _mm_mul_ps(py,y) ),
			_mm_add_ps( _mm_mul_ps(pz,z), pw ) );
		__m128 cmp = _mm_cmplt_ps( dots, w );
		inside = _mm_and_si128(inside, (__m128i&)cmp);
	}
	_mm_store_ps((float*)output_.elements, (__m128&)inside);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

eiInfoGroup( SphereFrustumTest, false );

struct SphereFrustumArgs
{
	Scope& a;
	Atomic finished;
};
int SphereFrustumThreadEntry( void* arg, uint threadIndex, uint numThreads, uint systemId )
{
	if( threadIndex != 0 )
	{
		YieldThreadUntil( WaitForTrue(((SphereFrustumArgs*)arg)->finished) );
		return 0;
	}
	Scope& a = ((SphereFrustumArgs*)arg)->a;
	Timer* timer = eiNewInterface(a, Timer)();
	struct Sphere4
	{
		Vec4 x;
		Vec4 y;
		Vec4 z;
		Vec4 w;
	};
	uint count = (15000+3)&~0x3U;

	eiASSERT( (count & ~0x3U) == count );

	float* sphereXs = eiAllocArray(a, float, count);
	float* sphereYs = eiAllocArray(a, float, count);
	float* sphereZs = eiAllocArray(a, float, count);
	float* sphereWs = eiAllocArray(a, float, count);
	Vec4* spheres = eiAllocArray(a, Vec4, count);
	Sphere4* sphere4s = eiAllocArray(a, Sphere4, count>>2);
	uint* results1 = eiAllocArray(a, uint, count);
	Vec4i* results2 = eiAllocArray(a, Vec4i, count>>2);
	uint* results3 = eiAllocArray(a, uint, count);
	ispc::SphereInfo* results4 = eiAllocArray(a, ispc::SphereInfo, count);
	ispc::SphereInfo* results5 = eiAllocArray(a, ispc::SphereInfo, count);
	ispc::SphereInfo* results6 = eiAllocArray(a, ispc::SphereInfo, count);
	uint numResults4 = 0;
	uint numResults5 = 0;
	uint numResults6 = 0;
	uint* ids = eiAllocArray(a, uint, count);
	Vec4 corner = {1,1,1};
	float radius = Length3D(corner);
	for( int i=0, end=count; i!=end; ++i )
	{
		int x = i % 10 ;
		int y = i / 10;
		int z = y % 10;
		y /= 10;
		float s = 2.5f;// + 5;
		Vec4 vecTranslate = {x*s - 5, y*s -5, z*s - 5};

		Vec4 sphere = vecTranslate;
		sphere[3] = radius;
		sphere4s[i>>2].x[i&0x3] = sphere[0];
		sphere4s[i>>2].y[i&0x3] = sphere[1];
		sphere4s[i>>2].z[i&0x3] = sphere[2];
		sphere4s[i>>2].w[i&0x3] = sphere[3];
		spheres[i] = sphere;
		sphereXs[i] = sphere[0];
		sphereYs[i] = sphere[1];
		sphereZs[i] = sphere[2];
		sphereWs[i] = sphere[3];
		ids[i] = i;
	}

	Mat4 matProj = Perspective( DegToRad(80.0f), 1.0f, 0.1, 100 );
	FrustumPlanes planes;
	planes.Extract( matProj );

	for( uint cycle = 0; cycle != 1; ++cycle )
	{
		double time1 = timer->Elapsed();
		for( uint i=0; i!=count; ++i )
		{
			results1[i] = planes.SphereInFrustum(spheres[i]) ? 0xFFFFFFFFU : 0;
		}
		double time2 = timer->Elapsed();
		for( uint i=0, end=count>>2; i!=end; ++i )
		{
			planes.Sphere4InFrustum(sphere4s[i].x, sphere4s[i].y, sphere4s[i].z, sphere4s[i].w, results2[i]);
		}
		double time3 = timer->Elapsed();

		SpheresInFrustum( (ispc::float4*)planes.planes, sphereXs, sphereYs, sphereZs, sphereWs, results3, count );
		double time4 = timer->Elapsed();

		SpheresFrustumInfo( (ispc::float4*)planes.planes, ids, sphereXs, sphereYs, sphereZs, sphereWs, results4, count, &numResults4 );
		double time5 = timer->Elapsed();

		SpheresFrustumInfo_Smp( (ispc::float4*)planes.planes, ids, sphereXs, sphereYs, sphereZs, sphereWs, results5, count, &numResults5 );
		double time6 = timer->Elapsed();


		static double avgT1 = 0; static int avgT1cnt = 0;
		static double avgT2 = 0; static int avgT2cnt = 0;
		static double avgT3 = 0; static int avgT3cnt = 0;
		static double avgT4 = 0; static int avgT4cnt = 0;
		static double avgT5 = 0; static int avgT5cnt = 0;

		double t1 = (time2-time1)*1000;
		double t2 = (time3-time2)*1000;
		double t3 = (time4-time3)*1000;
		double t4 = (time5-time4)*1000;
		double t5 = (time6-time5)*1000;
		avgT1 = (avgT1*avgT1cnt + t1)/(avgT1cnt+1); ++avgT1cnt;
		avgT2 = (avgT2*avgT2cnt + t2)/(avgT2cnt+1); ++avgT2cnt;
		avgT3 = (avgT3*avgT3cnt + t3)/(avgT3cnt+1); ++avgT3cnt;
		avgT4 = (avgT4*avgT4cnt + t4)/(avgT4cnt+1); ++avgT4cnt;
		avgT5 = (avgT5*avgT5cnt + t5)/(avgT5cnt+1); ++avgT5cnt;

		eiInfo( SphereFrustumTest, "1) %f, %f", t1, avgT1 );
		eiInfo( SphereFrustumTest, "2) %f, %f", t2, avgT2 );	
		eiInfo( SphereFrustumTest, "3) %f, %f", t3, avgT3 );	
		eiInfo( SphereFrustumTest, "4) %f, %f", t4, avgT4 );	
		eiInfo( SphereFrustumTest, "5) %f, %f", t5, avgT5 );	

		uint numVisible = 0;
		eiRASSERT( numResults4 == numResults5 );
		for( uint i=0, j=0; i!=count; ++i )
		{
			eiRASSERT( results1[i] == ((uint*)results2)[i] );
			eiRASSERT( results1[i] == results3[i] );
			if( results1[i] )
				++numVisible;
			if( j < numResults4 )
			{
				if( results1[i] )
				{
					eiRASSERT( results4[j].id == i );
					eiRASSERT( results5[j].id == i );
					++j;
				}
			}
			else
				eiRASSERT( !results1[i] );
		}
		eiRASSERT( numVisible == numResults4 );
	}
	((SphereFrustumArgs*)arg)->finished = 1;
	return 0;
}
eiTEST( SphereFrustum )
{
	MallocStack memory( eiMiB(128) );
	Scope a( memory.stack, "test" );
	SphereFrustumArgs args = { a };
	StartThreadPool( a, &SphereFrustumThreadEntry, &args, 4 );
}
//------------------------------------------------------------------------------

#include "stdlib.h"

eiInfoGroup( FluidSim, false );

struct FluidSimArgs
{
	Scope& a;
	Atomic finished;
};
int FluidSimThreadEntry( void* arg, uint threadIndex, uint numThreads, uint systemId )
{
	if( threadIndex != 0 )
	{
		YieldThreadUntil( WaitForTrue(((FluidSimArgs*)arg)->finished) );
		return 0;
	}
	Scope& a = ((SphereFrustumArgs*)arg)->a;
	Timer* timer = eiNewInterface(a, Timer)();

	using namespace ispc;

	uint count = 100000;
	float* pressure = eiAllocArray(a, float, count);
	Positions positionsA = 
	{
		eiAllocArray(a, float, count),
		eiAllocArray(a, float, count),
		eiAllocArray(a, float, count),
	};
	Positions positionsB = 
	{
		eiAllocArray(a, float, count),
		eiAllocArray(a, float, count),
		eiAllocArray(a, float, count),
	};
	Positions positionsC = 
	{
		eiAllocArray(a, float, count),
		eiAllocArray(a, float, count),
		eiAllocArray(a, float, count),
	};
	BucketDesc bucketDesc = 
	{
		{ -25, -5, -10 },
		{ 25, 5, 10 },
		{ 32, 32, 32 },
	};

	uint bucketCount = bucketDesc.count.v[0]*bucketDesc.count.v[1]*bucketDesc.count.v[2];
//	int* bucketOffsets = eiAllocArray(a, int, bucketCount);
	int* bucketCounts  = eiAllocArray(a, int, bucketCount);

	uint numTasks = 4;
	int** taskBucketOffsets = eiAllocArray(a, int*, numTasks);
	int** taskBucketCounts  = eiAllocArray(a, int*, numTasks);
	for( int i=0, end=numTasks; i!=end; ++i )
	{
		taskBucketOffsets[i] = eiAllocArray(a, int, bucketCount);
		taskBucketCounts[i] = eiAllocArray(a, int, bucketCount);
	}

	int* keyIndexA  = eiAllocArray(a, int, count);
	int* keyIndexB  = eiAllocArray(a, int, count);

	srand(42);
	for( int i=0, end=count; i!=end; ++i )
	{
		int x = rand()%10000;
		int y = rand()%10000;
		int z = rand()%10000;
	/*	int x = i % 10;
		int y = i / 10;
		int z = y % 10;
		y /= 10;
		y = y % 10;*/
		positionsA.x[i] = bucketDesc.minimum.v[0] + (bucketDesc.maximum.v[0]-bucketDesc.minimum.v[0]) * (x/10000.0f);
		positionsA.y[i] = bucketDesc.minimum.v[1] + (bucketDesc.maximum.v[1]-bucketDesc.minimum.v[1]) * (y/10000.0f);
		positionsA.z[i] = bucketDesc.minimum.v[2] + (bucketDesc.maximum.v[2]-bucketDesc.minimum.v[2]) * (z/10000.0f);
	}
	memcpy( positionsB.x, positionsA.x, sizeof(float)*count );
	memcpy( positionsB.y, positionsA.y, sizeof(float)*count );
	memcpy( positionsB.z, positionsA.z, sizeof(float)*count );

	for( uint cycle = 0; cycle != 1; ++cycle )
	{
		double time[8];
		 time[0] = timer->Elapsed();
		ApplyForcesPredictPositions(positionsB, positionsA, count, numTasks);
		 time[1] = timer->Elapsed();
	//	RebucketPositions(bucketOffsets, bucketCounts, positionsC, positionsB, count, bucketDesc);
	//	ParticleSort( bucketCounts, taskBucketCounts, taskBucketOffsets, positionsC, positionsB, keyIndexA, keyIndexB, numTasks, count, bucketDesc );
		ParticleSort1( bucketCounts, taskBucketCounts, taskBucketOffsets, positionsC, positionsB, keyIndexA, keyIndexB, numTasks, count, bucketDesc );
		 time[2] = timer->Elapsed();
		ParticleSort2( bucketCounts, taskBucketCounts, taskBucketOffsets, positionsC, positionsB, keyIndexA, keyIndexB, numTasks, count, bucketDesc );
		 time[3] = timer->Elapsed();
		ParticleSort3( bucketCounts, taskBucketCounts, taskBucketOffsets, positionsC, positionsB, keyIndexA, keyIndexB, numTasks, count, bucketDesc );
		 time[4] = timer->Elapsed();
		ParticleSort4( bucketCounts, taskBucketCounts, taskBucketOffsets, positionsC, positionsB, keyIndexA, keyIndexB, numTasks, count, bucketDesc );
		 time[5] = timer->Elapsed();
		for( uint iteration = 0; iteration != 1; ++iteration )
		{
			MeasurePressure(pressure, positionsC, count, bucketDesc, taskBucketOffsets[0], bucketCounts, numTasks);
		}
		time[6] = timer->Elapsed();
		memcpy( positionsA.x, positionsC.x, sizeof(float)*count );
		memcpy( positionsA.y, positionsC.y, sizeof(float)*count );
		memcpy( positionsA.z, positionsC.z, sizeof(float)*count );
		time[7] = timer->Elapsed();

		static double avgT[7]  = {}; static int avgTcnt[7] = {};
		double t[7];
		for( uint i=0; i!=7; ++i )
		{
			t[i] = (time[i+1] - time[i])*1000;
			if( t[i] < avgT[i]*2 || avgT[i] < 0.5f ) { avgT[i] = (avgT[i]*avgTcnt[i] + t[i])/(avgTcnt[i]+1); ++avgTcnt[i]; }
			eiInfo( FluidSim, "%d) %f, %f", i, t[i], avgT[i] );
		}
	}

	((FluidSimArgs*)arg)->finished = 1;
	return 0;
}

eiTEST( FluidSim )
{
	MallocStack memory( eiMiB(512) );
	Scope a( memory.stack, "test" );
	FluidSimArgs args = { a };
	StartThreadPool( a, &FluidSimThreadEntry, &args, 4 );
}