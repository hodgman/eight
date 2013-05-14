
#include "eight/core/math/types.ispc.h"

struct Frustum
{
	Vec4 planes[6];
};

typedef unsigned int uint;

inline uint SphereInFrustum( uniform const Vec4 planes[6], float sx, float sy, float sz, float sradius, varying float& depth )
{
	uint inside = ~0U;
	float dot;
	for( uniform uint i=0; i!=6; ++i )
	{
		uniform float px = planes[i].x;
		uniform float py = planes[i].y;
		uniform float pz = planes[i].z;
		uniform float pw = planes[i].w;
		dot = px * sx + py * sy + pz * sz + pw;
		inside &= sign_extend(dot < sradius);
	}
	depth = dot;
	return inside;
}

export void SpheresInFrustum( uniform const Vec4 planes[6], 
	uniform const float x[], uniform const float y[], uniform const float z[], uniform const float radius[],
	uniform uint out_inside_flag[], uniform uint count )
{
	assert( (count & ~0x3U) == count );
	count &= ~0x3U;
	foreach( index = 0 ... count )
	{
		float depth;
		uint inside = SphereInFrustum( planes, x[index], y[index], z[index], radius[index], depth );
		out_inside_flag[index] = inside;
	}
}

struct SphereInfo
{
	uint id;
	float depth;
};

export void SpheresFrustumInfo( uniform const Vec4 planes[6], uniform const uint sphere_ids[],
	uniform const float x[], uniform const float y[], uniform const float z[], uniform const float radius[],
	uniform SphereInfo out_info[], uniform uint count, uint* uniform out_count )
{
	assert( (count & ~0x3U) == count );
	count &= ~0x3U;
	uniform uint idCount = 0;
	foreach( index = 0 ... count )
	{
		float depth;
		uint inside = SphereInFrustum( planes, x[index], y[index], z[index], radius[index], depth);
		cif( inside )
		{
			foreach_active(lane)
			{
				out_info[idCount].id = sphere_ids[extract(index, lane)];
				out_info[idCount].depth = extract(depth, lane);
				idCount++;
			}
		}
	}
	*out_count = idCount;
}

inline task void SpheresFrustumInfo_Task( uniform const Vec4 planes[6], uniform const uint sphere_ids[],
	uniform const float x[], uniform const float y[], uniform const float z[], uniform const float radius[],
	uniform SphereInfo out_info[], uniform uint count, uniform uint out_counts[] )
{
	const uniform uint step = (((count+taskCount-1) / taskCount)+3)&~0x3U;
	uniform uint taskId = taskIndex;
	uniform uint start = step * taskIndex;
	uniform uint end = min( start+step, count );
	SpheresFrustumInfo( planes, sphere_ids+start, x+start, y+start, z+start, radius+start, out_info+start, end-start, out_counts+taskIndex );
}

export void SpheresFrustumInfo_Smp( uniform const Vec4 planes[6], uniform const uint sphere_ids[],
	uniform const float x[], uniform const float y[], uniform const float z[], uniform const float radius[],
	uniform SphereInfo out_info[], uniform uint count, uint* uniform out_count )
{
//	const uniform uint numTasks = 10;
	const uniform uint step = 10000;//(((count+numTasks-1) / numTasks)+3)&~0x3U;
	uniform uint start = 0;
	uniform uint end = 0;
	uniform uint numTasks = (count+step-1)/step;
	uniform uint* uniform taskOutCount = uniform new uint[numTasks];//todo - scope/stack alloc within ispc?
	launch[numTasks] SpheresFrustumInfo_Task( planes, sphere_ids, x, y, z, radius, out_info, count, taskOutCount );
	sync;
	uniform uint totalCount = taskOutCount[0];
	start = step;
	for( uniform uint i=1; i!=numTasks; ++i, start = end )
	{
	//	end = min( start+step, count );
	//	memmove( out_info+totalCount, out_info+start, (end-start)*sizeof(uniform SphereInfo) );
		totalCount += taskOutCount[i];
	}
	*out_count = totalCount;
	delete taskOutCount;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

struct Positions
{
	float* x, *y, *z;
};

struct BucketDesc
{
	float<3> minimum;
	float<3> maximum;
	int<3> count;
};
struct BucketRange
{
	float<3> invRange;
	float<3> offset;
	int<3> maxCell;
	int<3> count;
};

typedef int* uniform intptr;

inline void CalcBucketRange( uniform BucketRange& result, uniform BucketDesc& bucketDesc )
{
	result.invRange = 1/( bucketDesc.maximum - bucketDesc.minimum );
	result.offset = bucketDesc.minimum * result.invRange;
	result.maxCell = bucketDesc.count - 1;
	result.count = bucketDesc.count;
}
inline uint BucketIndex( float x, float y, float z, uniform BucketRange& b )
{
	float fx = x * b.invRange.x - b.offset.x;
	float fy = y * b.invRange.y - b.offset.y;
	float fz = z * b.invRange.z - b.offset.z;
		fx = ((int)(fx*0xFFFF))/((float)0xFFFF);
		fy = ((int)(fy*0xFFFF))/((float)0xFFFF);
		fz = ((int)(fz*0xFFFF))/((float)0xFFFF);
	int ix =  min( b.maxCell.x, max( 0, (int)floor(fx * b.count.x) ) );
	int iy =  min( b.maxCell.y, max( 0, (int)floor(fy * b.count.y) ) );
	int iz =  min( b.maxCell.z, max( 0, (int)floor(fz * b.count.z) ) );
	return iz*b.count.x*b.count.y + iy*b.count.x + ix;
}

inline void BucketCount( const uniform Positions& positions, uniform uint posCount,
					     uniform BucketDesc& bucketDesc, int* uniform bucketCounts, uniform uint c)
{
//	print( "cc %[%], (%,%,%)\n", posCount, c, positions.x, positions.y, positions.z );
	assert( (posCount&~3U) == posCount );
	posCount = (posCount+3)&~3U;
	float invRange = 1/(bucketDesc.maximum[c] - bucketDesc.minimum[c]);
	float offset = bucketDesc.minimum[c] * invRange;
	int maxCell = bucketDesc.count[c]-1;
	int endCell = bucketDesc.count[c];
	float* positionsv = c == 0 ? positions.x : (c == 1 ? positions.y : positions.z);
	foreach( i = 0...posCount )
	{
		float x = positions.x[i];
		float y = positions.y[i];
		float z = positions.z[i];// + 0.000002;
		float v = c == 0 ? x : (c == 1 ? y : z);
	//	float v = positionsv[i];
		//float v = select(c==0, x, select(c==1, y, z ) );
		float f = v * invRange - offset;
		f = ((int)(f*0xFFFF))/((float)0xFFFF);
		int cell = min( maxCell, max( 0, (int)floor(f * endCell) ) );
		atomic_add_local(&bucketCounts[cell], 1);
	/*	foreach_active(lane)
		{
			{
				uniform BucketRange b;
				CalcBucketRange( b, bucketDesc );
				uniform uint idx = extract(BucketIndex( x, y, z, b ),lane);
				uniform uint ucell = extract(cell, lane);
				uniform uint globalIdx = idx;
				switch(c)
				{
				case 0: idx &= 63; break;
				case 1: idx /= b.count.x; idx &= 63; break;
				case 2: idx /= b.count.x*b.count.y; idx &= 63; break;
				default: assert(false);
				}
				if( idx != ucell )
				{
					print( "%, %\n", c, globalIdx );
					print( "%!=%, [%,%,%]\n", idx, ucell, extract(x, lane), extract(y, lane), extract(z, lane) );
					float fx = extract(x, lane) * b.invRange.x - b.offset.x;
					float fy = extract(y, lane) * b.invRange.y - b.offset.y;
					float fz = extract(z, lane) * b.invRange.z - b.offset.z;
		fx = ((int)(fx*0xFFFF))/((float)0xFFFF);
		fy = ((int)(fy*0xFFFF))/((float)0xFFFF);
		fz = ((int)(fz*0xFFFF))/((float)0xFFFF);
					int ix =  min( b.maxCell.x, max( 0, (int)floor(fx * b.count.x) ) );
					int iy =  min( b.maxCell.y, max( 0, (int)floor(fy * b.count.y) ) );
					int iz =  min( b.maxCell.z, max( 0, (int)floor(fz * b.count.z) ) );
					print( "[%,%,%], [%,%,%]\n", fx, fy, fz, ix, iy, iz );
					print( "%==%\n", b.invRange.z, extract(invRange, lane) );
					print( "%==%\n", b.offset.z, extract(offset, lane) );
					print( "%==%\n", b.maxCell.z, extract(maxCell, lane) );
					float temp = fz * b.count.z;
					print( "%==%, %, %, %\n", temp, extract(f * (float)endCell, lane), f, (float)endCell, f*endCell );
					print( "%==%, %, %, %\n", fz * (b.count.z-1), extract(f * (float)maxCell, lane), f, (float)maxCell, f*maxCell );
					print( "%==%\n", (int)temp, extract((int)(f * endCell), lane) );
					print( "%==%\n", fz, extract(f, lane) );
					print( "%==%\n", iz, extract(cell, lane) );
				}
				assert( idx == ucell );
			}
		}*/
	}
}
inline void BucketWrite( uniform Positions& out_positions, const uniform Positions& positions, uniform uint posCount,
					     uniform BucketDesc& bucketDesc, int* uniform bucketWriten, int* uniform bucketOffsets, uniform uint c)
{
//	print( "ww %[%], (%,%,%)\n", posCount, c, positions.x, positions.y, positions.z );
	assert( (posCount&~3U) == posCount );
	posCount = (posCount+3)&~3U;
	float invRange = 1/(bucketDesc.maximum[c] - bucketDesc.minimum[c]);
	float offset = bucketDesc.minimum[c] * invRange;
	int maxCell = bucketDesc.count[c] - 1;
	int endCell = bucketDesc.count[c];
	foreach( i = 0...posCount )
	{
		float x = positions.x[i];
		float y = positions.y[i];
		float z = positions.z[i];
		float v = c == 0 ? x : (c == 1 ? y : z);
		//float v = select(c==0, x, select(c==1, y, z ) );
		float f = v * invRange - offset;
		f = ((int)(f*0xFFFF))/((float)0xFFFF);
		int cell = min( maxCell, max( 0, (int)floor(f * endCell) ) );
	//	int bucketSubOffset = atomic_add_local(&bucketWriten[cell], 1);
		int bucketSubOffset = 0;
		foreach_active(lane)
		{
		/*	{
				uniform BucketRange range;
				CalcBucketRange( range, bucketDesc );
				uniform uint idx = BucketIndex( extract(x, lane), extract(y, lane), extract(z, lane), range );
				uniform uint ucell = extract(cell, lane);
				switch(c)
				{
				case 0: idx &= 63; break;
				case 1: idx /= 64; idx &= 63; break;
				case 2: idx /= 64*64; idx &= 63; break;
				default: assert(false);
				}
				if( idx != ucell )
					print( "%!=%, [%,%,%]\n", idx, ucell, extract(x, lane), extract(y, lane), extract(z, lane) );
				assert( idx == ucell );
			}*/
			uniform uint cellIdx = extract(cell, lane);
			bucketSubOffset = insert(bucketSubOffset, lane, bucketWriten[cellIdx]++ );
		}
		int outIndex = bucketOffsets[cell] + bucketSubOffset;
		out_positions.x[outIndex] = x;
		out_positions.y[outIndex] = y;
		out_positions.z[outIndex] = z;
	}
}

inline task void BucketCount_Task( const uniform Positions& positions, uniform uint posCount,
					               uniform BucketDesc& bucketDesc, uniform int* uniform bucketCounts[], uniform uint c)
{
	uniform uint step = (posCount+taskCount-1) / taskCount;
	step = (step+3)&~3U;
	uniform uint begin = taskIndex * step;
	uniform uint end = min( begin+step, posCount );
	uniform uint taskPosCount = end-begin;
	uniform Positions taskPositions = 
	{
		positions.x + begin,
		positions.y + begin,
		positions.z + begin
	};
	uniform int* uniform taskBucketCounts = bucketCounts[taskIndex];
//	print( "c(%/%) %-% [%], (%,%,%)\n", taskIndex, taskCount, begin, end, c, taskPositions.x, taskPositions.y, taskPositions.z );
	BucketCount( taskPositions, taskPosCount, bucketDesc, taskBucketCounts, c );
}
inline task void BucketWrite_Task( uniform Positions& out_positions, const uniform Positions& positions, uniform uint posCount,
					          uniform BucketDesc& bucketDesc, uniform int* uniform bucketWriten[], uniform int* uniform bucketOffsets[], uniform uint c )
{
	uniform uint step = (posCount+taskCount-1) / taskCount;
	step = (step+3)&~3U;
	uniform uint begin = taskIndex * step;
	uniform uint end = min( begin+step, posCount );
	uniform uint taskPosCount = end-begin;
	uniform Positions taskPositions = 
	{
		positions.x + begin,
		positions.y + begin,
		positions.z + begin
	};
	uniform int* uniform taskBucketWriten = bucketWriten[taskIndex];
	uniform int* uniform taskBucketOffsets = bucketOffsets[taskIndex];
//	print( "w(%/%) %-% [%], (%,%,%)\n", taskIndex, taskCount, begin, end, c, taskPositions.x, taskPositions.y, taskPositions.z );
//	print( "w[%] (%,%,%) <= (%,%,%)\n", taskIndex, out_positions.x, out_positions.y, out_positions.z, taskPositions.x, taskPositions.y, taskPositions.z );
	BucketWrite( out_positions, taskPositions, taskPosCount, bucketDesc, taskBucketWriten, taskBucketOffsets, c );
}

//todo - generate key/index pair list, sort that, then use sorted indices to reorder?
void Rebucket( int* uniform out_bucketBegin, int* uniform out_bucketCount, uniform Positions& out_positions, const uniform Positions& in_positions, uniform uint posCount, uniform BucketDesc& bucketDesc )
{
	uniform uint numTasks = 4;
	assert(numTasks);
	uniform intptr* uniform bucketCounts = uniform new intptr[numTasks];
	uniform intptr* uniform bucketCounts2 = bucketCounts;//uniform new intptr[numTasks];
	uniform intptr* uniform bucketOffsets = uniform new intptr[numTasks];
	uniform uint maxBuckets = max( bucketDesc.count.x, max( bucketDesc.count.y, bucketDesc.count.z ) );
	for( uniform uint i=0; i!=numTasks; ++i )
	{
		bucketCounts[i] = uniform new int[maxBuckets];
		bucketCounts2[i] = uniform new int[maxBuckets];
		bucketOffsets[i] = uniform new int[maxBuckets];
	}
	uniform Positions bufferA = in_positions;
	uniform Positions bufferB = out_positions;
	for( uniform uint c=0; c!=3; ++c )
	{
		uniform uint numBuckets = bucketDesc.count[c];
		for( uniform uint i=0; i!=numTasks; ++i )
		{
			memset( bucketCounts[i], 0, sizeof(uniform int)*numBuckets );
		}
	//	if( numTasks > 1 )
		{
			launch[numTasks] BucketCount_Task( bufferA, posCount, bucketDesc, bucketCounts, c );
			sync;
		}
	//	else
	//		BucketCount( bufferA, posCount, bucketDesc, bucketCounts[0], c );

		uniform int currentOffset = 0;
		for( uniform uint bucketIdx=0; bucketIdx!=numBuckets; ++bucketIdx )
		{
			out_bucketBegin[bucketIdx] = currentOffset;
		//	print( "out_bucketBegin[%] = %\n", bucketIdx, out_bucketBegin[bucketIdx] );
			for( uniform uint taskIdx=0; taskIdx!=numTasks; ++taskIdx )
			{
				if( bucketCounts[taskIdx][bucketIdx] )
				{
					bucketOffsets[taskIdx][bucketIdx] = currentOffset;
					currentOffset += bucketCounts[taskIdx][bucketIdx];
				}
				else
					bucketOffsets[taskIdx][bucketIdx] = 0xFFFFFFFF;
			}
			out_bucketCount[bucketIdx] = currentOffset - out_bucketBegin[bucketIdx];
		}
		assert( currentOffset == posCount );
		
		for( uniform uint i=0; i!=numTasks; ++i )
		{
			memset( bucketCounts2[i], 0, sizeof(uniform int)*numBuckets );
		}

	//	if( numTasks > 1 )
		{
			launch[numTasks] BucketWrite_Task( bufferB, bufferA, posCount, bucketDesc, bucketCounts2, bucketOffsets, c );
			sync;
		}
	//	else
	//		BucketWrite( bufferB, bufferA, posCount, bucketDesc, bucketCounts[0], bucketOffsets[0], c );

	/*	currentOffset = 0;
		for( uniform uint bucketIdx=0; bucketIdx!=numBuckets; ++bucketIdx )
		{
			for( uniform uint taskIdx=0; taskIdx!=numTasks; ++taskIdx )
			{
				if( bucketCounts[taskIdx][bucketIdx] != bucketCounts2[taskIdx][bucketIdx] )
					print( "c% == w%, [%][%] \n", bucketCounts[taskIdx][bucketIdx], bucketCounts2[taskIdx][bucketIdx], taskIdx,bucketIdx );
				assert( bucketCounts[taskIdx][bucketIdx] == bucketCounts2[taskIdx][bucketIdx] );
				currentOffset += bucketCounts2[taskIdx][bucketIdx];
			}
		}
		assert( currentOffset == posCount );*/

	/*	foreach( i = 0...numBuckets )
		{
		//	print( "i = %, w = %, c = %, numBuckets = %\n", i, bucketWriten[i], bucketCounts[0][i], numBuckets );
			assert( bucketWriten[i] == bucketCounts[0][i] );
		}*/

		uniform Positions temp = bufferA;
		bufferA = bufferB;
		bufferB = temp;
	}
	/*
	uniform BucketRange range;
	CalcBucketRange( range, bucketDesc );
	uniform uint prevIdx = 0;
	for( uniform uint i=0; i!=posCount; ++i )
	{
		uniform float x = out_positions.x[i];
		uniform float y = out_positions.y[i];
		uniform float z = out_positions.z[i];
		uniform uint idx = extract(BucketIndex( x, y, z, range ),0);
		if( idx < prevIdx )
			print( "idx %, prev %, [%,%,%]\n", idx, prevIdx, x, y, z );
		assert( idx >= prevIdx );
		prevIdx = idx;
	}*/
	for( uniform uint i=0; i!=numTasks; ++i )
	{
		delete bucketCounts[i];
	}
	delete bucketCounts;
}

export void RebucketPositions( int* uniform out_bucketBegin, int* uniform out_bucketCount, uniform Positions& out_positions, const uniform Positions& positions, uniform uint posCount, uniform BucketDesc& bucketDesc )
{
	Rebucket( out_bucketBegin, out_bucketCount, out_positions, positions, posCount, bucketDesc );
}


inline uint MakeKeyIndex( uint key, uint index )
{
	assert( key   <  0x8000U );
	assert( index < 0x20000U );
	return key | (index << 15);
}
inline uint ExtractKey( uint keyIndex )
{
	return keyIndex & 0x7FFFU;
}
inline uint ExtractIndex( uint keyIndex )
{
	return keyIndex >> 15U;
}

inline void GenerateSortKeys_Inline( int*uniform out_bucketsCount, int* uniform out_keyIndex, 
								 const uniform Positions& positions, uniform uint begin, uniform uint end, uniform BucketDesc& bucketDesc )
{
	assert( (begin&~3U) == begin );
	assert( (end&~3U) == end );
	begin = (begin+3)&~3U;
	end = (end+3)&~3U;
	uniform BucketRange b;
	CalcBucketRange( b, bucketDesc );
	foreach( i = begin...end )
	{
		float x = positions.x[i];
		float y = positions.y[i];
		float z = positions.z[i];
		uint cell = BucketIndex( x, y, z, b );
		atomic_add_local(&out_bucketsCount[cell], 1);
		out_keyIndex[i] = MakeKeyIndex( cell, i );
	}
}
task void GenerateSortKeys_Task( int**uniform out_TaskBucketsCount, int* uniform out_keyIndex, 
								 const uniform Positions& positions, uniform uint posCount, uniform BucketDesc& bucketDesc, uniform uint numBuckets )
{
	uniform uint step = (posCount+taskCount-1) / taskCount;
	step = (step+3)&~3U;
	uniform uint begin = taskIndex * step;
	uniform uint end = min( begin+step, posCount );
	uniform uint taskPosCount = end-begin;
	uniform int* uniform taskBucketCount = out_TaskBucketsCount[taskIndex];
	memset( taskBucketCount, 0, sizeof(uniform int)*numBuckets );
	GenerateSortKeys_Inline( taskBucketCount, out_keyIndex, positions, begin, end, bucketDesc );
}


inline void RadixWrite_Inline( int* uniform out_keyIndex, int*uniform out_bucketsWritten,
							   int*uniform bucketsOffset, int* uniform in_keyIndex, uniform uint count )
{
	count = (count+3)&~3U;
	foreach( i = 0...count )
	{
		int keyIndex = in_keyIndex[i];
	//	assert( keyIndex != 0xffffffff );
		int key = ExtractKey( keyIndex );
	/*	if( reduce_equal(key) )
		{
			uniform uint cellIdx = extract(key, 0);
			uniform uint base = bucketsOffset[cellIdx] + out_bucketsWritten[cellIdx];
			int outIndex = base + programIndex;
			out_keyIndex[outIndex] = keyIndex;
			out_bucketsWritten[cellIdx] += programCount;
		}
		else*/
		{
			foreach_active(lane)
			{
				uniform uint cellIdx = extract(key, lane);
				uniform int outIndex = bucketsOffset[cellIdx] + out_bucketsWritten[cellIdx]++;
			//	assert( out_keyIndex[outIndex] == 0xFFFFFFFF );
				out_keyIndex[outIndex] = extract(keyIndex, lane);
			}
		}
	}
}
task void RadixWrite_Task( int* uniform out_keyIndex, int**uniform out_tasksBucketsWritten,
						   int**uniform tasksBucketsOffset, int* uniform in_keyIndex, uniform uint count, uniform uint numBuckets )
{
	uniform uint step = (count+taskCount-1) / taskCount;
	step = (step+3)&~3U;
	uniform uint begin = taskIndex * step;
	uniform uint end = min( begin+step, count );
	uniform uint taskKeyCount = end-begin;
	uniform int* uniform taskKeyIndex = in_keyIndex + begin;
	uniform int* uniform taskBucketsWritten = out_tasksBucketsWritten[taskIndex];
	uniform int* uniform taskBucketsOffset = tasksBucketsOffset[taskIndex];
	memset( taskBucketsWritten, 0, sizeof(uniform int)*numBuckets );
	RadixWrite_Inline( out_keyIndex, taskBucketsWritten, taskBucketsOffset, taskKeyIndex, taskKeyCount );
}


inline void RadixScatter_Inline( uniform Positions& out_positions, const uniform Positions& positions, int* uniform keyIndex, uniform uint indexBegin, uniform uint indexEnd )
{
	assert( (indexBegin&~3U) == indexBegin );
	assert( (indexEnd&~3U) == indexEnd );
	indexBegin = (indexBegin+3)&~3U;
	indexEnd = (indexEnd+3)&~3U;
	uniform uint prev = 0;
	foreach( i = indexBegin...indexEnd )
	{
	//	assert( i >= indexBegin && i < indexEnd );
		uint ki = keyIndex[i];
		uint kk = ExtractKey( ki );
		uint index = ExtractIndex( ki );
	//	assert( ki != 0xffffffff );
		float x = positions.x[index];
		float y = positions.y[index];
		float z = positions.z[index];
		out_positions.x[i] = x;
		out_positions.y[i] = y;
		out_positions.z[i] = z;
	}
}
task void RadixScatter_Task( uniform Positions& out_positions, const uniform Positions& positions, int* uniform keyIndex, uniform uint indexCount/*, uniform BucketDesc& bucketDesc*/ )
{
	uniform uint step = (indexCount+taskCount-1) / taskCount;
	step = (step+3)&~3U;
	uniform uint begin = taskIndex * step;
	uniform uint end = min( begin+step, indexCount );
	RadixScatter_Inline( out_positions, positions, keyIndex, begin, end/*, bucketDesc*/ );
}

export void ParticleSort( int*uniform out_TotalBucketsCount, int**uniform out_TaskBucketsCount, int**uniform out_TaskBucketsOffset, 
						  uniform Positions& out_positions, const uniform Positions& positions, 
						  int* uniform out_keyIndexA, int* uniform out_keyIndexB, uniform int numTasks, 
						  uniform uint posCount, uniform BucketDesc& bucketDesc )
{
	uniform uint numBuckets = bucketDesc.count.x*bucketDesc.count.y*bucketDesc.count.z;
	launch[numTasks] GenerateSortKeys_Task( out_TaskBucketsCount, out_keyIndexA, positions, posCount, bucketDesc, numBuckets );
	sync;
	uniform int currentOffset = 0;
	for( uniform uint bucketIdx=0; bucketIdx!=numBuckets; ++bucketIdx )
	{
		uniform int bucketBegin = currentOffset;
		for( uniform uint taskIdx=0; taskIdx!=numTasks; ++taskIdx )
		{
			out_TaskBucketsOffset[taskIdx][bucketIdx] = currentOffset;
			currentOffset += out_TaskBucketsCount[taskIdx][bucketIdx];
		}
		out_TotalBucketsCount[bucketIdx] = currentOffset - bucketBegin;
	}
	assert( currentOffset == posCount );
	launch[numTasks] RadixWrite_Task( out_keyIndexB, out_TaskBucketsCount, out_TaskBucketsOffset, out_keyIndexA, posCount, numBuckets/*, bucketDesc*/ );
	sync;
	launch[numTasks] RadixScatter_Task( out_positions, positions, out_keyIndexB, posCount );
}

export void ParticleSort1( int*uniform out_TotalBucketsCount, int**uniform out_TaskBucketsCount, int**uniform out_TaskBucketsOffset, 
						  uniform Positions& out_positions, const uniform Positions& positions, 
						  int* uniform out_keyIndexA, int* uniform out_keyIndexB, uniform int numTasks, 
						  uniform uint posCount, uniform BucketDesc& bucketDesc )
{
	memset( out_keyIndexA, 0xFFFFFFFF, sizeof(uniform uint)*posCount );
	uniform uint numBuckets = bucketDesc.count.x*bucketDesc.count.y*bucketDesc.count.z;
	launch[numTasks] GenerateSortKeys_Task( out_TaskBucketsCount, out_keyIndexA, positions, posCount, bucketDesc, numBuckets );
}
export void ParticleSort2( int*uniform out_TotalBucketsCount, int**uniform out_TaskBucketsCount, int**uniform out_TaskBucketsOffset, 
						  uniform Positions& out_positions, const uniform Positions& positions, 
						  int* uniform out_keyIndexA, int* uniform out_keyIndexB, uniform int numTasks, 
						  uniform uint posCount, uniform BucketDesc& bucketDesc )
{//todo - break into tasks
	uniform uint numBuckets = bucketDesc.count.x*bucketDesc.count.y*bucketDesc.count.z;
	uniform int currentOffset = 0;
	for( uniform uint bucketIdx=0; bucketIdx!=numBuckets; ++bucketIdx )
	{
		uniform int bucketBegin = currentOffset;
		for( uniform uint taskIdx=0; taskIdx!=numTasks; ++taskIdx )
		{
			out_TaskBucketsOffset[taskIdx][bucketIdx] = currentOffset;
			currentOffset += out_TaskBucketsCount[taskIdx][bucketIdx];
		}
		out_TotalBucketsCount[bucketIdx] = currentOffset - bucketBegin;
	}
	assert( currentOffset == posCount );
}
export void ParticleSort3( int*uniform out_TotalBucketsCount, int**uniform out_TaskBucketsCount, int**uniform out_TaskBucketsOffset, 
						  uniform Positions& out_positions, const uniform Positions& positions, 
						  int* uniform out_keyIndexA, int* uniform out_keyIndexB, uniform int numTasks, 
						  uniform uint posCount, uniform BucketDesc& bucketDesc )
{
	memset( out_keyIndexB, 0xFFFFFFFF, sizeof(uniform uint)*posCount );
	uniform uint numBuckets = bucketDesc.count.x*bucketDesc.count.y*bucketDesc.count.z;
	launch[numTasks] RadixWrite_Task( out_keyIndexB, out_TaskBucketsCount, out_TaskBucketsOffset, out_keyIndexA, posCount, numBuckets );
}
export void ParticleSort4( int*uniform out_TotalBucketsCount, int**uniform out_TaskBucketsCount, int**uniform out_TaskBucketsOffset, 
						  uniform Positions& out_positions, const uniform Positions& positions, 
						  int* uniform out_keyIndexA, int* uniform out_keyIndexB, uniform int numTasks, 
						  uniform uint posCount, uniform BucketDesc& bucketDesc )
{
	launch[numTasks] RadixScatter_Task( out_positions, positions, out_keyIndexB, posCount/*, bucketDesc*/ );
//	RadixScatter_Inline( out_positions, positions, out_keyIndexB, 0, posCount/*, bucketDesc*/ );
}


inline void ApplyForcesPredictPositions_Inline( uniform Positions& inPrev_outEstimate, const uniform Positions& original, uniform uint posCount )
{
	float invPrevDt = 1/0.01;
	float currDt = 0.01;
	foreach( i = 0...posCount )
	{
		float xPrev = inPrev_outEstimate.x[i];
		float yPrev = inPrev_outEstimate.y[i];
		float zPrev = inPrev_outEstimate.z[i];
		float xCurr = original.x[i];
		float yCurr = original.y[i];
		float zCurr = original.z[i];

		float xAccel = 0;
		float yAccel = -9.8;
		float zAccel = 0;

		float xVel = (xCurr-xPrev)*invPrevDt + xAccel*currDt;
		float yVel = (yCurr-yPrev)*invPrevDt + yAccel*currDt;
		float zVel = (zCurr-zPrev)*invPrevDt + zAccel*currDt;
		
		float xEstimate = xCurr;// + xVel*currDt*0.00000001;
		float yEstimate = yCurr;// + yVel*currDt*0.00000001;
		float zEstimate = zCurr;// + zVel*currDt*0.00000001;//!

		assert( !isnan(xEstimate) );
		assert( !isnan(yEstimate) );
		assert( !isnan(zEstimate) );
//  		xEstimate = isnan(xEstimate) ? 0 : xEstimate;
//  		yEstimate = isnan(yEstimate) ? 0 : yEstimate;
//  		zEstimate = isnan(zEstimate) ? 0 : zEstimate;

		inPrev_outEstimate.x[i] = xEstimate;
		inPrev_outEstimate.y[i] = yEstimate;
		inPrev_outEstimate.z[i] = zEstimate;
	}
}
task void ApplyForcesPredictPositions_Task( uniform Positions& inPrev_outEstimate, const uniform Positions& original, uniform uint posCount )
{
	uniform uint step = (posCount+taskCount-1) / taskCount;
	step = (step+3)&~3U;
	uniform uint begin = taskIndex * step;
	uniform uint end = min( begin+step, posCount );
	uniform uint taskPosCount = end-begin;
	uniform Positions taskPosA = 
	{
		inPrev_outEstimate.x + begin,
		inPrev_outEstimate.y + begin,
		inPrev_outEstimate.z + begin
	};
	uniform Positions taskPosB = 
	{
		original.x + begin,
		original.y + begin,
		original.z + begin
	};
	ApplyForcesPredictPositions_Inline( taskPosA, taskPosB, taskPosCount );
}
export void ApplyForcesPredictPositions( uniform Positions& inPrev_outEstimate, const uniform Positions& original, uniform uint posCount, uniform uint numTasks )
{
	assert(numTasks);
	if( numTasks > 1 )
		launch[numTasks] ApplyForcesPredictPositions_Task( inPrev_outEstimate, original, posCount );
	else
		ApplyForcesPredictPositions_Inline( inPrev_outEstimate, original, posCount );
}


inline uniform float Poly6HScale( uniform float h )
{
	return 315.0/( 64 * 3.1415926535897932384626433832795 * pow(h,9) );
}
inline float Poly6( uniform float hScale, uniform float hSq, float rSq )
{
	float c = max( 0, hSq - rSq );
	return (hScale * c) * (c * c);
}

inline uniform float SpikyHScale( uniform float h )
{
	return 15.0/( 3.1415926535897932384626433832795 * pow(h,6) );
}
inline float Spiky( uniform float hScale, uniform float h, float r )
{
	float c = max( 0, h - r );
	return (hScale * c) * (c * c);
}

inline uniform float<4> ViscosityHData( uniform float h )
{
	uniform float<4> data = 
	{
		h,
		1/(h*h),
		-1/((2*h)*(h*h)),
		15.0/( 3.1415926535897932384626433832795 * pow(h,6) )
	};
	return data;
}
inline float Viscosity( uniform float<4> hData, float r, float rSq )
{
	uint c = sign_extend( r < hData.x );
	return floatbits(intbits(hData.w)&c) * (r*rSq*hData.z + rSq*hData.y + hData.x/(2*r) - 1);
}


task void MeasurePressure_Task(uniform float pressure[], const uniform Positions& positions, uniform uint posCount, uniform BucketDesc& bucketDesc, int* uniform bucketBegin, int* uniform bucketCount )
{
	uniform BucketRange b;
	CalcBucketRange( b, bucketDesc );
	uniform uint numBuckets = bucketDesc.count.x*bucketDesc.count.y*bucketDesc.count.z;
	
//	uniform uint step = (numBuckets+taskCount-1) / taskCount;
	uniform uint step = (bucketDesc.count.z+taskCount-1) / taskCount;
	step = (step+3)&~3U;
	uniform uint begin = taskIndex * step;
	uniform uint end = min( begin+step, (uniform uint)bucketDesc.count.z );
//	uniform uint end = min( begin+step, numBuckets );

	const uniform float h = 0.05;
	const uniform float poly6hScale = Poly6HScale(h);
	const uniform float hSq = h*h;

//	uniform float opsAvg = 0; uniform int opCnt = 0;

//	for( uniform uint cell=begin; cell!=end; ++cell )
	for( uniform uint zCell=begin; zCell!=end; ++zCell )
	for( uniform uint yCell=0; yCell!=bucketDesc.count.y; ++yCell )
	for( uniform uint xCell=0; xCell!=bucketDesc.count.x; ++xCell )
	{
//		uniform uint ops = 0;
		uniform uint xFirst = max( 0, ((int)xCell)-1 );
		uniform uint xLast = min( (uniform uint)bucketDesc.count.x-1, xCell+1 );
		uniform int cell = zCell*b.count.x*b.count.y + yCell*b.count.x + xCell;
		uniform uint bBegin = bucketBegin[cell];
		uniform uint bEnd = bBegin + bucketCount[cell];
		uniform int cellFirst = zCell*b.count.x*b.count.y + yCell*b.count.x + xFirst;
		uniform int cellLast = zCell*b.count.x*b.count.y + yCell*b.count.x + xLast;
		uniform uint bInnerBegin = bucketBegin[cellFirst];
		uniform uint bInnerEnd = bucketBegin[cellLast] + bucketCount[cellLast];
		for( uniform uint i=bBegin; i!=bEnd; ++i )
		{
			uniform float d = 0;
			uniform float x = positions.x[i];
			uniform float y = positions.y[i];
			uniform float z = positions.z[i];
			foreach( j = bInnerBegin...bInnerEnd )
			{
				float x2 = positions.x[j];
				float y2 = positions.y[j];
				float z2 = positions.z[j];
				float xDiff = x - x2;
				float yDiff = y - y2;
				float zDiff = z - z2;
				float distSq = ( xDiff*xDiff + yDiff*yDiff + zDiff*zDiff );
				float w = Poly6(poly6hScale, hSq, distSq);
				w = floatbits( intbits(w) & sign_extend( i!=j ) );
				d += reduce_add( w );
//				atomic_add_local(&ops, 1);
			}
			pressure[i] = d;
		}
// 		opsAvg = (opsAvg*opCnt + ops)/(opCnt+1);
// 		++opCnt;
	}
//	print( "opsAvg %\n", opsAvg );
}
export void MeasurePressure(uniform float pressure[], const uniform Positions& positions, uniform uint posCount, uniform BucketDesc& bucketDesc, int* uniform bucketBegin, int* uniform bucketCount, uniform uint numTasks)
{
/*	uniform BucketRange b;
	CalcBucketRange( b, bucketDesc );
	uint prev = 0;
	for( uniform uint i=0; i!=posCount; ++i )
	{
		uniform float x = positions.x[i];
		uniform float y = positions.y[i];
		uniform float z = positions.z[i];
		uniform uint key = extract( BucketIndex( x, y, z, b ), 0);
	//	print( "%\n", key );
		assert( key >= prev );
		prev = key;
	}*/
	launch[numTasks] MeasurePressure_Task( pressure, positions, posCount, bucketDesc, bucketBegin, bucketCount );
	/*
	for( uniform uint i=0; i!=posCount; ++i )
	{
		uniform float xPos = positions.x[i];
		uniform float yPos = positions.y[i];
		uniform float zPos = positions.z[i];
		uniform float fx = xPos * b.invRange.x - b.offset.x;
		uniform float fy = yPos * b.invRange.y - b.offset.y;
		uniform float fz = zPos * b.invRange.z - b.offset.z;
			fx = ((int)(fx*0xFFFF))/((float)0xFFFF);
			fy = ((int)(fy*0xFFFF))/((float)0xFFFF);
			fz = ((int)(fz*0xFFFF))/((float)0xFFFF);
		uniform int ix =  min( b.maxCell.x, max( 0, (int)floor(fx * b.count.x) ) );
		uniform int iy =  min( b.maxCell.y, max( 0, (int)floor(fy * b.count.y) ) );
		uniform int iz =  min( b.maxCell.z, max( 0, (int)floor(fz * b.count.z) ) );

		ix -= fx < 0.5 ? 1 : 0;
		iy -= fy < 0.5 ? 1 : 0;
		iz -= fz < 0.5 ? 1 : 0;
		ix = max(ix,0);
		iy = max(iy,0);
		iz = max(iz,0);
		uniform int ix2 = min(ix+1,b.maxCell.x);
		uniform int ey = min(iy+2,b.count.y);
		uniform int ez = min(iz+2,b.count.z);
		
		uniform float p = 0;
		for( uniform uint z=iz; z!=ez; ++z )
		{
			for( uniform uint y=iy; y!=ey; ++y )
			{
				uniform int cell1 = z*b.count.x*b.count.y + y*b.count.x + ix;
				uniform int cell2 = z*b.count.x*b.count.y + y*b.count.x + ix2;
				uniform int begin = bucketBegin[cell1];
				uniform int count = bucketCount[cell1] + (cell1==cell2?0:bucketCount[cell2]);
				foreach( j = begin...begin+count )
				{
					float xPos2 = positions.x[j];
					float yPos2 = positions.y[j];
					float zPos2 = positions.z[j];
					float xDiff = xPos - xPos2;
					float yDiff = yPos - yPos2;
					float zDiff = zPos - zPos2;
					float dist = sqrt( xDiff*xDiff + yDiff*yDiff + zDiff*zDiff );
					p += reduce_add(1/dist);
				}
			}
		}
		pressure[i] = p;
	}*/
}