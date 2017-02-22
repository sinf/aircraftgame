#include <stdlib.h>
#include "system.h"
#include "support.h"
#include "game.h"
#include "models.h"
#include "draw.h"

int r_resy;
int r_pitch;
U32 *r_canvas;
static float eye_x=0, eye_y=0;
#define HORZ_VIEW_RANGE 90
static float view_scale = r_resx / HORZ_VIEW_RANGE; //world unit -> pixel

typedef struct { float r, g, b; } Color;

static Color lerp( Color a, Color b, float t )
{
	Color c;
	c.r = a.r + ( b.r - a.r ) * t;
	c.g = a.g + ( b.g - a.g ) * t;
	c.b = a.b + ( b.b - a.b ) * t;
	return c;
}

static U32 pack( Color c )
{
	float t=255;
	int r = c.r*t, g = c.g*t, b = c.b*t;
	return r << 16 | g << 8 | b;
}

static U32 gray( float t )
{
	Color c = {t,t,t};
	return pack( c );
}

static void hline( int y, U32 c )
{
	if ( y >= 0 && y < r_resy ) {
		U32 *dst = r_canvas + r_pitch / 4 * y;
		for( int x=0; x<r_resx; ++x )
			dst[x] = c;
	}
}

static void hline_w( float w, U32 c )
{
	int y = r_resy / 2 + ( w - eye_y ) * view_scale;
	hline( y, c );
}

static void draw_bg( void )
{
	Color c0 = {0.694, 0.784, 0.804};
	Color c1 = {0.400, 0.435, 0.569};
	float t0 = -50;
	float t1 = 70;

	float view_h = r_resy / view_scale;
	float t = ( eye_y - view_h * 0.5f - t0 ) / ( t1 - t0 );
	float dt = 1.0 / (( t1 - t0 ) * view_scale );
	U32 *dst = r_canvas;
	int skip = r_pitch >> 2;

	for( int y=0; y<r_resy; ++y ) {
		float q = CLIP( t, 0, 1 );
		U32 c = pack( lerp( c0, c1, q ) );
		//U32 c = gray( q );
		for( int x=0; x<r_resx; ++x )
			dst[x] = c; // todo: dither
		t += dt;
		dst += skip;
	}

	hline_w( t0, pack( c1 ) );
	hline_w( t1, pack( c0 ) );
}

void draw_water( void )
{
	float view_w = r_resx / view_scale;
	float view_h = r_resy / view_scale;

	int u_prec = 13;
	S32 u_one = 1 << u_prec;
	S32 u0 = ( eye_x - view_w * 0.5f ) / WATER_ELEM_SPACING * u_one;
	S32 dudx = 1.0f / ( WATER_ELEM_SPACING * view_scale ) * u_one;

	U32 *dst = r_canvas;
	int skip = r_pitch >> 2;

	Real y = REALF( eye_y - view_h * 0.5f );
	Real dy = REALF( 1.0f / view_scale );

	y -= REALF( W_WATER_LEVEL );

	for( int ln=0; ln<r_resy; ++ln ) {
		Real u = u0;
		for( int x=0; x<r_resx; ++x, u+=dudx ) {
			unsigned m = WATER_RESOL, i = u >> u_prec;
			Real *z = WORLD.water.z;
			#if 0
			// triangle shaped waves
			DReal f = REAL_FRACT_PART( u >> u_prec - REAL_FRACT_BITS );
			Real l = z[i % m];
			Real r = z[(i+1u) % m];
			Real h = l + (( r - l ) * f >> REAL_FRACT_BITS );
			#else
			Real h = z[i%m];
			#endif

			if ( y > h )
				dst[x] = dst[x] >> 1 & 0x7f7f7f;
		}
		y += dy;
		dst += skip;
	}
}

void render( void )
{
	if ( WORLD.player )
	{
		eye_x = REALTOF( WORLD.player->phys.pos.x );
		eye_y = REALTOF( WORLD.player->phys.pos.y );
	}

	draw_bg();
	draw_water();

	hline_w( eye_y, 0xe0e0e0 );
	hline_w( 0, 0x808080 );
	//hline_w( W_WATER_LEVEL, 0xFF0000 );
	hline_w( W_WATER_LEVEL + W_WATER_DEPTH, 0x7F0000 );
	hline_w( W_WATER_DEATH_LEVEL, 0x1F003F );
	hline_w( W_WATER_LEVEL + W_HEIGHT, 0x1f0000 );
}

