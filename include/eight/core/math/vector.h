//------------------------------------------------------------------------------
#pragma once
namespace eight {
//------------------------------------------------------------------------------

struct Vec4
{
	float elements[4];
	      float& operator[]( uint i )       { return elements[i]; }
	const float& operator[]( uint i ) const { return elements[i]; }
	const Vec4 operator *( float s ) const
	{
		Vec4 result = {{ elements[0]*s, elements[1]*s, elements[2]*s, elements[3]*s }};
		return result;
	}
	const Vec4 operator *( const Vec4& vec ) const
	{
		Vec4 result = {{ elements[0]*vec[0], elements[1]*vec[1], elements[2]*vec[2], elements[3]*vec[3] }};
		return result;
	}
	const Vec4 operator +( const Vec4& vec ) const
	{
		Vec4 result = {{ elements[0]+vec[0], elements[1]+vec[1], elements[2]+vec[2], elements[3]+vec[3] }};
		return result;
	}
};

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
