#include <stdlib.h>
#include "system.h"
#include "support.h"
#include "game.h"
#include "models.h"
#include "draw.h"
#include "lowgfx.h"

#include <emmintrin.h>

int r_resy;
int r_pitch;
U32 *r_canvas;
static Real eye_x=0, eye_y=0;
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

static Color unpack( U32 x )
{
	Color c;
	c.r = x >> 16 & 0xff;
	c.g = x >> 8 & 0xff;
	c.b = x & 0xff;
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

static void hline( unsigned y, U32 c )
{
	if ( y < (unsigned) r_resy ) {
		U32 *dst = r_canvas + r_pitch / 4 * y;
		for( int x=0; x<r_resx; ++x )
			dst[x] = c;
	}
}

static void vline( unsigned x, U32 c )
{
	if ( x < (unsigned) r_resx ) {
		for( int y=0; y<r_resy; ++y )
			r_canvas[r_pitch / 4 * y + x] = c;
	}
}

Real get_rel_x( Real x, Real x0 )
{
	Real c = x - x0;
	if ( c < -REALF( W_WIDTH/2 ) )
		c += REALF( W_WIDTH );
	else if ( c > REALF( W_WIDTH/2 ) )
		c -= REALF( W_WIDTH );
	return c;
}

static void hline_w( float w, U32 c )
{
	int y = r_resy / 2 + ( w - REALTOF( eye_y ) ) * view_scale;
	hline( y, c );
}

static void vline_w( float w, U32 c )
{
	float p = REALTOF( get_rel_x( REALF( w ), eye_x ) );
	int x = r_resx / 2 + p * view_scale;
	vline( x, c );
}

static void draw_bg( void )
{
	Color c0 = {0.694, 0.784, 0.804};
	Color c1 = {0.400, 0.435, 0.569};
	float t0 = -50;
	float t1 = 70;

	float view_h = r_resy / view_scale;
	float t = ( REALTOF( eye_y ) - view_h * 0.5f - t0 ) / ( t1 - t0 );
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
	int u_prec = 14;

	Real tmp = REALF( W_WIDTH ) + eye_x - REALF( HORZ_VIEW_RANGE/2 );
	U32 u0 = (tmp % REALF( W_WIDTH ) << u_prec)/REALF(WATER_ELEM_SPACING);
	U32 dudx = (1u<<u_prec) / ( WATER_ELEM_SPACING * view_scale );

	U32 *dst = r_canvas;
	int skip = r_pitch >> 2;

	Real y = eye_y - REALF( r_resy / view_scale * 0.5f ) - REALF( W_WATER_LEVEL );
	Real dy = REALF( 1.0f / view_scale );

	for( int ln=0; ln<r_resy; ++ln ) {
		U32 u = u0;
		for( int x=0; x<r_resx; ++x, u+=dudx ) {
			U32 m = WATER_RESOL;
			U32 i = u >> u_prec;
			U32 j = i%m;
			Real *z = WORLD.water.z;
			#if 0
			// triangle shaped waves
			DReal f = REAL_FRACT_PART( u >> u_prec - REAL_FRACT_BITS );
			Real l = z[j];
			Real r = z[(i+1u) % m];
			Real h = l + (( r - l ) * f >> REAL_FRACT_BITS );
			#else
			Real h = z[j];
			#endif

			if ( y > h ) {
				dst[x] = dst[x] >> 1 & 0x7f7f7f;
			}
		}
		y += dy;
		dst += skip;
	}
}

static int object_is_visible( Real x ) {
	return abs( get_rel_x( x, eye_x ) )
	< REALF( HORZ_VIEW_RANGE/2+MAX_THING_BOUND_R );
}

void draw_circle( float cx, float cy, float r, U32 color )
{
	int x0, y0, x1, y1, x, y;

	x0 = cx - r;
	x0 = MAX( x0, 0 );
	x0 &= ~3; // align to 16 byte boundary (aka 4 pixels)
	x1 = cx + r + 1.0f;
	x1 = x0 + ( x1 - x0 + 3 & ~3 ); // pad width to multiple of 4
	x1 = MIN( x1, r_resx );

	y0 = cy - r;
	y0 = MAX( y0, 0 );
	y1 = cy + r + 1.0f;
	y1 = MIN( y1, r_resy );

	int skip = r_pitch >> 2;
	U32 *dst = r_canvas + y0 * skip;

	float dx0s = x0 + 0.5f - cx;
	float dy0s = y0 + 0.5f - cy;

	static const float data[] = {1,1,1,1,4,4,4,4,8,8,8,8,16,16,16,16,0,1,2,3};
	__m128 a0, b, dx0, dy, one, c4, c8, c16, rr;
	rr = _mm_set1_ps( r*r );
	one = _mm_load_ps( data );
	c4 = _mm_load_ps( data+4 );
	c8 = _mm_load_ps( data+8 );
	c16 = _mm_load_ps( data+12 );

	// (dx0,dy): distance to centre
	dx0 = _mm_add_ps( _mm_set1_ps( dx0s ), _mm_load_ps( data+16 ) );
	dy = _mm_set1_ps( dy0s );
	
	// (a0,b): squared distance to centre
	a0 = _mm_mul_ps( dx0, dx0 );
	b = _mm_mul_ps( dy, dy );

	for( y=y0; y<y1; ++y ) {

		__m128 f = _mm_add_ps( a0, b );
		__m128 dx = dx0;

		for( x=x0; x<x1; x+=4 ) {

			int m = _mm_movemask_ps( _mm_cmplt_ps( f, rr ) );
			if ( m & 1 ) dst[x] = color;
			if ( m & 2 ) dst[x+1] = color;
			if ( m & 4 ) dst[x+2] = color;
			if ( m & 8 ) dst[x+3] = color;

			f = _mm_add_ps( f, c16 );
			f = _mm_add_ps( f, _mm_mul_ps( dx, c8 ) );
			dx = _mm_add_ps( dx, c4 );
		}

		b = _mm_add_ps( b, one );
		b = _mm_add_ps( b, _mm_add_ps( dy, dy ) );
		dy = _mm_add_ps( dy, one );
		dst += skip;
	}
}

void draw_blob( const GfxBlob blob[1] )
{
	Real rx = get_rel_x( blob->x, eye_x );
	Real ry = blob->y - eye_y;

	unsigned
	x = r_resx/2 + REALTOF( rx ) * view_scale,
	y = r_resy/2 + REALTOF( ry ) * view_scale;

	if ( x < r_resx && y < r_resy ) {
		draw_circle( x, y, REALTOF( blob->scale_x ) * view_scale, blob->color );
	}
}

void draw_blobs( unsigned num_blobs, const GfxBlob blobs[] )
{
	unsigned i;
	for( i=0; i<num_blobs; ++i )
		draw_blob( blobs+i );
}


static GfxBlob get_particle_blob( Thing *thing )
{
	static const Real PARTICLE_SIZE[] = {
		REALF(0.5), REALF(0.5), /* water1 */
		REALF(1.4), REALF(0.75), /* water2*/
		REALF(1), REALF(1), /* smoke (size should be random) */
	};
	
	ParticleType type = thing->data.pt.type;
	U8 g, a;
	Real w, h;
	GfxBlob blob;
	
	/* 8-bit grayscale values for particles:
		0xf4 ... water1
		0xf4 ... water2
		0x32 ... smoke */
	g = 0x32f4f4 >> ( type << 3 );
	
	#if ENABLE_BLEND
	/* Fade out */
	a = 0xFF - ( thing->age << 8 ) / ( REALF( MAX_PARTICLE_TIME ) + 1 );
	#else
	a = g;
	#endif
	
	w = PARTICLE_SIZE[type << 1];
	h = PARTICLE_SIZE[(type << 1) + 1];
	
	blob.mode = BLOB_SHARP;
	blob.color = RGBA_32( g, g, g, a );
	blob.x = thing->phys.pos.x;
	blob.y = thing->phys.pos.y;
	blob.scale_x = w;
	blob.scale_y = h;
	
	if ( type == PT_SMOKE ) {
		blob.scale_y = blob.scale_x = \
		REALF( 1.0 ) + REALF( 0.75 ) * thing->age / (unsigned)( MAX_PARTICLE_TIME * GAME_TICKS_PER_SEC );
		blob.mode = BLOB_FUZZY;
	}
	
	return blob;
}

static void render_world_fg( void )
{
	#define MAX_BLOBS 32000
	GfxBlob blobs[MAX_BLOBS];
	unsigned num_blobs = 0;
	unsigned n;
	
	for( n=0; n<WORLD.num_things; n++ )
	{
		Thing *t = WORLD.things + n;
		Real x = t->phys.pos.x;
		Real y = t->phys.pos.y;
		Real yaw = t->angle;
		Real roll = 0;
		ModelID mdl = BAD_MODEL_ID;
		int is_heli = 0;
		
		if ( !object_is_visible( x ) )
			continue;
		
		switch( t->type )
		{
			case T_AIRCRAFT:
				roll = yaw + REALF( PI );
				mdl = ( is_heli = t->data.ac.is_heli ) ? M_HELI_BODY : M_AIRCRAFT;
				break;
			
			case T_GUNSHIP:
				mdl = M_GUNSHIP;
				break;
			
			case T_AAGUN:
				if ( num_blobs < MAX_BLOBS ) {
					GfxBlob b;
					b.color = RGBA_32(0,0,0,255);
					b.x = x;
					b.y = y;
					b.scale_x = b.scale_y = REALF( 1.5 );
					blobs[num_blobs++] = b;
				}
				mdl = M_AAGUN_BARREL;
				break;
			
			case T_RADAR:
				roll = t->age;
				yaw = REALF( -PI / 2 );
				mdl = M_RADAR;
				if ( t->parent )
					yaw += t->parent->angle;
				break;
			
			case T_BATTLESHIP:
				mdl = M_BATTLESHIP;
				break;
			
			case T_PROJECTILE:
				if ( num_blobs < MAX_BLOBS ) {
					GfxBlob b;
					b.color = RGBA_32(0,0,0,255);
					b.x = x;
					b.y = y;
					b.scale_x = b.scale_y = REALF( 0.5 );
					blobs[num_blobs++] = b;
				}
				break;
			
			case T_PARTICLE:
				if ( num_blobs < MAX_BLOBS ) {
					blobs[num_blobs++] = get_particle_blob( t );
				}
				break;
			
			case T_DEBRIS:
				mdl = M_DEBRIS;
				roll = 2 * yaw + REALF( PI/3 );
				break;
			
			default:
				break;
		}
		
		if ( mdl != BAD_MODEL_ID )
		{
			mat_push();
			mat_translate( x, y, 0 );
			{
				mat_push();
				mat_rotate( 2, REALTOF( yaw ) );
				{
					mat_push();
					{
						mat_rotate( 0, REALTOF( roll ) );
						push_model_mat( mdl );
					}
					mat_pop();
					
					if ( t->type == T_AIRCRAFT && !is_heli && t->data.ac.throttle_on )
						push_model_mat( M_AIRCRAFT_FLAME );
				}
				mat_pop();
				
				if ( is_heli ) {
					mat_rotate( 2, REALTOF( yaw ) );
					mat_rotate( 0, REALTOF( roll ) );
					
					/* The main rotor */
					mat_push();
					{
						mat_translate( REALF(0.2), REALF(0.1), 0 );
						mat_rotate( 1, REALTOF( 16 * t->age % REALF( 2 * PI ) ) );
						push_model_mat( M_HELI_ROTOR );
					}
					mat_pop();
					
					/* Small rotor in the rear */
					mat_translate( REALF(-1.2), REALF(0.2), 0 );
					mat_rotate( 2, REALTOF( -16 * t->age % REALF( 2 * PI ) ) );
					mat_scale( 0.5, 0.5, 0.5 );
					mat_rotate( 0, -PI/2 );
					push_model_mat( M_HELI_ROTOR );
				}
			}
			mat_pop();
		}
		else
		{
		}

		if ( mdl != BAD_MODEL_ID ) {
			// make all entities visible as a dot
			float xx = r_resx * 0.5f + REALTOF( get_rel_x( x, eye_x ) ) * view_scale;
			float yy = r_resy * 0.5f + REALTOF( y - eye_y ) * view_scale;
			draw_circle( xx, yy, 2.5f, 0xFFFFFF );
		}
	}
	
	ASSERT( num_blobs < MAX_BLOBS );
	
	flush_models();
	draw_blobs( num_blobs, blobs );
	
	#if DRAW_CLOUDS
	draw_blobs( NUM_CLOUD_BLOBS, cloud_blobs );
	#endif
}


void render( void )
{
	if ( WORLD.player )
	{
		eye_x = WORLD.player->phys.pos.x;
		eye_y = WORLD.player->phys.pos.y;
	}

	draw_bg();
	draw_water();

	hline_w( REALTOF( eye_y ), 0xe0e0e0 );
	vline_w( REALTOF( eye_x ), 0xe0e0e0 );
	hline_w( 0, 0x808080 );
	vline_w( 0, 0x808080 );
	//hline_w( W_WATER_LEVEL, 0xFF0000 );
	hline_w( W_WATER_LEVEL + W_WATER_DEPTH, 0x7F0000 );
	hline_w( W_WATER_DEATH_LEVEL, 0x1F003F );
	hline_w( W_WATER_LEVEL + W_HEIGHT, 0x1f0000 );

	mat_push();
	mat_translate( -eye_x, -eye_y, 0 );

	render_world_fg();

	mat_pop();

	if ( 1 ) {
		static float a = 0;
		int x = 50.5f + cosf( a ) * 40;
		int y = 50.5f + sinf( a) * 20;
		draw_circle( x, y, 20, 0xFF );
		a += PI/300.0f;
	}
}

