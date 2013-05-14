//------------------------------------------------------------------------------
#pragma once
#include <eight/core/math/matrix.h>
namespace eight {
//------------------------------------------------------------------------------

struct FrustumPlanes
{
	Vec4 planes[6];
	void Extract( const Mat4& viewProjection_ )
	{
		Mat4 viewProjection = Transpose(viewProjection_);

		planes[0] = -viewProjection[0] + viewProjection[3];//r
		planes[1] =  viewProjection[0] + viewProjection[3];//l
		planes[2] = -viewProjection[1] + viewProjection[3];//t
		planes[3] =  viewProjection[1] + viewProjection[3];//b
		planes[4] = -viewProjection[2] + viewProjection[3];//f
		planes[5] =  viewProjection[2] + viewProjection[3];//n
		
		for( uint i=0; i!=6; ++i )
			planes[i] = -NormalizePlane(planes[i]);
	}
	bool SphereInFrustum( const Vec4& sphere )
	{
		bool inside = true;
		for ( uint i = 0; i < 6; i++ )
		{
			inside &= ( PlaneDotPoint( planes[i], sphere ) < sphere[3] );
		}
		return inside;
	}
	void Sphere4InFrustum( const Vec4& x, const Vec4& y, const Vec4& z, const Vec4& radius, Vec4i& output );
/*	{
		Vec4i inside = {{~0U, ~0U, ~0U, ~0U}};
		for ( uint i = 0; i < 6; i++ )
		{
			Vec4 px = {{planes[i][0], planes[i][0], planes[i][0], planes[i][0]}};
			Vec4 py = {{planes[i][1], planes[i][1], planes[i][1], planes[i][1]}};
			Vec4 pz = {{planes[i][2], planes[i][2], planes[i][2], planes[i][2]}};
			Vec4 pw = {{planes[i][3], planes[i][3], planes[i][3], planes[i][3]}};
			Vec4 dots = px * x + py * y + pz * z + pw;
			Vec4i cmp = dots < radius;
			inside &= cmp;
		}
		output = inside;
	}*/
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
