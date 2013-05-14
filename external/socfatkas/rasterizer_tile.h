#pragma once

#include <eight/core/debug.h>
#include <windows.h>
#include "math.h"

#pragma warning(disable:4127)

struct _MM_ALIGN16 Triangle
{
	__m128 x0, x1, x2;
	__m128 y0, y1, y2;
	int mask;
};

struct _MM_ALIGN16 Tile
{
	struct CachedTriangleBin
	{
		float				m_z;
		int*				m_triangles;
		CachedTriangleBin*	m_next;
		int*				m_flag;
		int					m_count;
	};

	static const int		g_tile_height = 720;
	static const int		g_tile_width = 128;

	__m128i					m_frame_buffer[ g_tile_height ];
	int						m_x;
	CachedTriangleBin*		m_triangles;

	static const int		g_max_triangles = 128*1024;
	static Triangle			g_triangles[ g_max_triangles ];
	static int				g_current_triangle;

	static const int		g_max_triangle_indices = 256*1024;
	static int				g_triangle_indices[ g_max_triangle_indices ];
	static int				g_current_triangle_index;
	
	static __m128			g_tile_size;
	static const __m128		g_almost_one;
	static __m128i			g_x_shifts[ 10 ][ 4096 ];

	inline static Triangle* allocate_triangles( int count, int& start_index )
	{
		eiASSERT( count + g_current_triangle < g_max_triangles );
		start_index = g_current_triangle;
		g_current_triangle += count;
		return g_triangles + start_index;
	}

	inline CachedTriangleBin* allocate_bin( int* flag, float z, int count )
	{
		eiASSERT( g_current_triangle_index + count + 5 < g_max_triangle_indices );

		int triangle_offset = g_current_triangle_index;
		g_current_triangle_index += count + 5;

		CachedTriangleBin* bin = (CachedTriangleBin*)( g_triangle_indices + triangle_offset );
		bin->m_count = count;
		bin->m_z = z;
		bin->m_triangles = g_triangle_indices + triangle_offset + 5;			
		bin->m_flag = flag;
		bin->m_next = (CachedTriangleBin*)m_triangles;
		m_triangles = bin;

		return bin;
	}

	Tile() : m_triangles( 0 )
	{
		for ( int i = 0; i < g_tile_height; ++i )
			m_frame_buffer[i] = VecIntZero();
	}

	inline void sort( __m128 &A0, __m128 &A1, __m128 &B0, __m128 &B1 )
	{
		__m128 mask = _mm_cmple_ps( B0, B1 );
		__m128 sx = _mm_add_ps( A0, A1 );
		A0 = _mm_or_ps( _mm_and_ps( mask, A0 ), _mm_andnot_ps( mask, A1 ) );
		A1 = _mm_sub_ps( sx, A0 );
			
		__m128 sy = _mm_add_ps( B0, B1 );
		B0 = _mm_or_ps( _mm_and_ps( mask, B0 ), _mm_andnot_ps( mask, B1 ) );
		B1 = _mm_sub_ps(sy, B0);			
	}
		
	template < bool is_occluder > __forceinline void draw_4triangles( const Triangle& tri, int* flag )
	{
		__m128 x0 = tri.x0, x1 = tri.x1, x2 = tri.x2;
		__m128 y0 = tri.y0, y1 = tri.y1, y2 = tri.y2;

		sort( x0, x1, y0, y1 );
		sort( x1, x2, y1, y2 );
		sort( x0, x1, y0, y1 );

		_MM_ALIGN16 int iy0[4], iy1[4], iy2[4], ix0[4], ix1[4], dx1[4], dx2[4], dx3[4];
		VecIntStore( iy0, VecFloat2Int(y0) );
		VecIntStore( iy1, VecFloat2Int(y1) );
		VecIntStore( iy2, VecFloat2Int(y2) );
		VecIntStore( ix0, VecFloat2Int(x0) );
		VecIntStore( ix1, VecFloat2Int(x1) );

		VecIntStore( dx1, VecFloat2Int( _mm_mul_ps( _mm_sub_ps( x2, x0 ), _mm_rcp_ps( _mm_sub_ps( y2, y0 ) ) ) ) );
		VecIntStore( dx2, VecFloat2Int( _mm_mul_ps( _mm_sub_ps( x1, x0 ), _mm_rcp_ps( _mm_sub_ps( y1, y0 ) ) ) ) );
		VecIntStore( dx3, VecFloat2Int( _mm_mul_ps( _mm_sub_ps( x2, x1 ), _mm_rcp_ps( _mm_sub_ps( y2, y1 ) ) ) ) );
		
		eiASSERT(m_x<10);
		__m128i* masks = g_x_shifts[m_x];
		for ( size_t i = 0, mask = 1; i < 4; ++i, mask <<= 1 ) 
		{
			if ( ( tri.mask & mask ) == 0 )
				continue;

			int xs1 = ix0[i], xs2 = ix0[i], xs3 = ix1[i];
		    int xa1 = dx1[i], xa2 = dx2[i], xa3 = dx3[i];
			int y1 = iy0[i], y2 = iy1[i], y3 = iy2[i];
		
			for( int scanline = y1; scanline < y2; ++scanline )
			{
				int xb = xs1 >> 16;
				int xe = xs2 >> 16;
				xs1 += xa1;
				xs2 += xa2;

				if ( is_occluder )
					m_frame_buffer[scanline] = VecIntOr( m_frame_buffer[scanline], VecIntXor( masks[xb], masks[xe] ) );
				else
				{
					__m128i span = VecIntXor( masks[xb], masks[xe] );
					if ( _mm_movemask_epi8( VecIntCmpEqual( VecIntAnd( m_frame_buffer[scanline], span ), span ) ) != 65535 )
					{
						*flag = 1;
						return;
					}
				}
			}
			
			for( int scanline = y2; scanline < y3; ++scanline )
			{
				int xb = xs1 >> 16;
				int xe = xs3 >> 16;
				xs1 += xa1;
				xs3 += xa3;

				__m128i span = VecIntXor( masks[xb], masks[xe] );
				if ( is_occluder )
					m_frame_buffer[scanline] = VecIntOr( m_frame_buffer[scanline], span );
				else if ( _mm_movemask_epi8( VecIntCmpEqual( VecIntAnd( m_frame_buffer[scanline], span ), span ) ) != 65535 )
				{
					*flag = 1;
					return;
				}
			}
		
		}
	}
		
	void draw_triangles( )
	{		
		int tris = 0;
		CachedTriangleBin* triangles = sort( tris );
		if ( tris == 0 )
			return;
		
		for ( int i = 0; i < tris; ++i )
		{
			CachedTriangleBin& bin = triangles[i];
			if ( bin.m_flag )
			{
				for ( int j = 0; j < bin.m_count; ++j )
					draw_4triangles< false >( g_triangles[ bin.m_triangles[ j ] ], bin.m_flag );
			}
			else
				for ( int j = 0; j < bin.m_count; ++j )
					draw_4triangles< true >( g_triangles[ bin.m_triangles[ j ] ], 0 );
		}
	}

	Tile::CachedTriangleBin* sort( int& tris );
};
