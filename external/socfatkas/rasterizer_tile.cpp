
#include "rasterizer_tile.h"

__m128 						Tile::g_tile_size = Vector4( 1.f / (float)Tile::g_tile_width, 1.f / (float)Tile::g_tile_height, 1.f / (float)Tile::g_tile_width, 1.f / (float)Tile::g_tile_height );
const __m128 				Tile::g_almost_one = Vector4( 0.0f, 0.0f, 0.9999f, 0.9999f );
__m128i						Tile::g_x_shifts[ 10 ][ 4096 ];
int							Tile::g_triangle_indices[ Tile::g_max_triangle_indices ];
int							Tile::g_current_triangle_index = 0;

Triangle					Tile::g_triangles[ Tile::g_max_triangles ];
int							Tile::g_current_triangle = 0;

static Tile::CachedTriangleBin g_sort_array0[ 32768 ];
static Tile::CachedTriangleBin g_sort_array1[ 32768 ];

inline unsigned int radix_unsigned_float_predicate( float v )
{
	return *reinterpret_cast<const unsigned int*>(&v);
}


Tile::CachedTriangleBin* Tile::sort( int& tris )
{
	unsigned int histograms[256*4];

	for (size_t i = 0; i < 256*4; i += 4)
	{
		histograms[i+0] = 0;
		histograms[i+1] = 0;
		histograms[i+2] = 0;
		histograms[i+3] = 0;
	}

	unsigned int* h0 = histograms;
	unsigned int* h1 = histograms + 256;
	unsigned int* h2 = histograms + 256*2;
	unsigned int* h3 = histograms + 256*3;

	tris = 0;
	CachedTriangleBin* bin = m_triangles;
	while ( bin )
	{
		g_sort_array0[ tris++ ] = *bin;
#define _0(h) ((h) & 255)
#define _1(h) (((h) >> 8) & 255)
#define _2(h) (((h) >> 16) & 255)
#define _3(h) ((h) >> 24)

		unsigned int h = radix_unsigned_float_predicate(bin->m_z);
		h0[_0(h)]++; 
		h1[_1(h)]++; 
		h2[_2(h)]++; 
		h3[_3(h)]++;
		bin = bin->m_next;
	}
	eiASSERT( tris < 32768 );

	unsigned int sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;
	unsigned int tsum;

	for (unsigned int i = 0; i < 256; i += 4)
	{
		tsum = h0[i+0] + sum0; h0[i+0] = sum0 - 1; sum0 = tsum;
		tsum = h0[i+1] + sum0; h0[i+1] = sum0 - 1; sum0 = tsum;
		tsum = h0[i+2] + sum0; h0[i+2] = sum0 - 1; sum0 = tsum;
		tsum = h0[i+3] + sum0; h0[i+3] = sum0 - 1; sum0 = tsum;

		tsum = h1[i+0] + sum1; h1[i+0] = sum1 - 1; sum1 = tsum;
		tsum = h1[i+1] + sum1; h1[i+1] = sum1 - 1; sum1 = tsum;
		tsum = h1[i+2] + sum1; h1[i+2] = sum1 - 1; sum1 = tsum;
		tsum = h1[i+3] + sum1; h1[i+3] = sum1 - 1; sum1 = tsum;

		tsum = h2[i+0] + sum2; h2[i+0] = sum2 - 1; sum2 = tsum;
		tsum = h2[i+1] + sum2; h2[i+1] = sum2 - 1; sum2 = tsum;
		tsum = h2[i+2] + sum2; h2[i+2] = sum2 - 1; sum2 = tsum;
		tsum = h2[i+3] + sum2; h2[i+3] = sum2 - 1; sum2 = tsum;

		tsum = h3[i+0] + sum3; h3[i+0] = sum3 - 1; sum3 = tsum;
		tsum = h3[i+1] + sum3; h3[i+1] = sum3 - 1; sum3 = tsum;
		tsum = h3[i+2] + sum3; h3[i+2] = sum3 - 1; sum3 = tsum;
		tsum = h3[i+3] + sum3; h3[i+3] = sum3 - 1; sum3 = tsum;
	}
#define RADIX_PASS(src, src_end, dst, hist, func) \
	for (const CachedTriangleBin* i = src; i != src_end; ++i) \
	{ \
		unsigned int h = radix_unsigned_float_predicate(i->m_z); \
		dst[++hist[func(h)]] = *i; \
	}

	RADIX_PASS(g_sort_array0, g_sort_array0 + tris, g_sort_array1, h0, _0);
	RADIX_PASS(g_sort_array1, g_sort_array1 + tris, g_sort_array0, h1, _1);
	RADIX_PASS(g_sort_array0, g_sort_array0 + tris, g_sort_array1, h2, _2);
	RADIX_PASS(g_sort_array1, g_sort_array1 + tris, g_sort_array0, h3, _3);

	return g_sort_array0;
}
