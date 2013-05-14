
#include "rasterizer.h"
#include <stdio.h>

//__m128 Rasterizer::g_one = Vector4( 1.f, 1.f, 1.f, 1.f );
const __m128 Rasterizer::g_tile_bounds = Vector4( (float)Rasterizer::g_width, 1.f, (float)Rasterizer::g_width, 1.f );
const __m128 Rasterizer::g_fixed_point_factor = Vector4( 65536.f, 65536.f, 65536.f, 65536.f );
const __m128 Rasterizer::g_total_width_v = Vector4( (float)Rasterizer::g_total_width );
const __m128 Rasterizer::g_total_height_v = Vector4( (float)Rasterizer::g_total_height);

inline static int Shift( int val ) 
{
	if( val > 31 )
		return 0;
	if( val < 0 )
		return 0xffffffff;
	return 0xffffffff >> val;
}

/*
int Rasterizer::Present() const
{
	int count = 0;

	// int indices[4] = { 1, 0, 3, 2 };
	int indices[4] = { 0, 1, 2, 3 };
	{
		for( int i = 0; i < Tile::g_tile_height; ++i )
		{				
			for ( int x = 0; x < g_width; ++x )
			{
				for ( int k = 0; k < 4; ++k )
				{
					int frame_buffer = *( (int*)&m_tiles[x].m_frame_buffer[i] + indices[k]);
					for( int j = 0; j < 32; ++j )
					{
						char c = ( frame_buffer >> j ) & 1;
						count += c != 0 ? 1 : 0;
					}
				}					
			}
		}
	}
	return count;
}*/

void Rasterizer::Init()
{
	static bool init = false;
	if( init )
		return;
	for ( int j = 0; j < 10; ++j )
	{
		for (int i = 0; i < j*128; ++i )
			Tile::g_x_shifts[ j ][ i ] = Vector4Int( 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff );
		for (size_t i = (j+1)*128; i < 4096; ++i )
			Tile::g_x_shifts[ j ][ i ] = VecIntZero();
		for (size_t i = 0; i < 128; ++i ) 
			Tile::g_x_shifts[ j ][ j*128 + i ] = _mm_set_epi32( Shift(i), Shift(i - 32), Shift(i - 64), Shift(i - 96) );
	}
	init = true;
}
