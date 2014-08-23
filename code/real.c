#include <xmmintrin.h>
#include "system.h"
#include "real.h"

Vec2 v_lerp( Vec2 a, Vec2 b, DReal t )
{
	Vec2 c;
	DReal s = REALF( 1 ) - t;
	c.x = ( a.x * s + b.x * t ) >> REAL_FRACT_BITS;
	c.y = ( a.y * s + b.y * t ) >> REAL_FRACT_BITS;
	return c;
}

Vec2 v_addmul( Vec2 a, Vec2 b, DReal b_scale )
{
	Vec2 c;
	c.x = a.x + DREAL_MUL( b.x, b_scale );
	c.y = a.y + DREAL_MUL( b.y, b_scale );
	return c;
}

Vec2 v_sub( Vec2 a, Vec2 b )
{
	Vec2 c;
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	return c;
}

Vec2 v_sincos( Real r )
{
	float a = REALTOF( r );
	Vec2 v;
	v.x = REALF( sinf( a ) );
	v.y = REALF( cosf( a ) );
	return v;
}

Vec2 v_rotate( Vec2 v, Vec2 sc )
{
	Vec2 v1;	
	v1.x = ( v.x * sc.y - v.y * sc.x ) >> REAL_FRACT_BITS;
	v1.y = ( v.x * sc.x + v.y * sc.y ) >> REAL_FRACT_BITS;
	return v1;
}

Real v_lensq( Vec2 v )
{
	return v.x * v.x + v.y * v.y >> REAL_FRACT_BITS;
}

Real v_length( Vec2 v )
{
	return sqrt( v_lensq( v ) );
}

/*
y = 1/x²
1/x² - y = 0

f(x) = 1/(x²) - y
D f(x) = -2/(x³)

x1 = x - ( 1/(x²) - y ) / ( -2/(x³) )
x1 = x - ( 1/(x²) - y ) * ( (x³)/-2 )
x1 = x - 1/(x²)*( (x³)/-2 ) - y*( (x³)/-2 )
x1 = x - ( (x³)/-2 )/(x²) - y*(x³)/-2
x1 = x - (x³)/(-2*x²) - y*(x³)/-2
x1 = x - x/(-2) - y*(x³)/-2
x1 = x + x/2 + y*x³/2
x1 = x + ( x + y*x³ ) * 0.5
x1 = x + x * 0.5 * ( 1 + y*x*x )
*/

static float rsqrtf( float a )
{
	float x = _mm_cvtss_f32( _mm_rsqrt_ss( _mm_set_ss( a ) ) );
	x = x + x * 0.5f * ( 1.0f + a * x * x );
	return x;
}

Vec2 v_normalize( Vec2 v )
{
	float t = rsqrtf( v_lensq( v ) );
	Vec2 n;
	n.x = v.x * t;
	n.y = v.y * t;
	return n;
}
