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
static const float view_scale = r_resx / HORZ_VIEW_RANGE; //world unit -> pixel

float the_view_mat[16];

///static struct { S16 r[4], g[4], b[4]; } r_canvas16[MAX_RESY][r_resx/4];

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

static void draw_point( U32 x, U32 y, U32 c )
{
	if ( x < r_resx && y < r_resy )
		r_canvas[r_pitch / 4 * y + x] = c;
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

static S32 current_mvp[16];

void set_mvp_matrix_f( const float m[16] )
{
	for( int i=0; i<16; ++i )
		current_mvp[i] = REALF( m[i] );
}

static void transform_vertex( S32 out[3], S32 const in[3], S32 const m[16] )
{
	int i, p = REAL_FRACT_BITS;
	for( i=0; i<3; ++i ) {
		out[i] = ( m[0+i]*in[i] + m[4+i]*in[i] + m[8+i]*in[i] >> p ) + m[12+i];
		//out[i] = ( m[4*i+0]*in[i] + m[4*i+1]*in[i] + m[4*i+2]*in[i] >> p ) + m[4*i+3];
	}
	#if 0
	// eye space to window coords
	//S32 f = REALF( view_scale );
	float g = view_scale / ( 1 << p );
	out[0] = r_resx/2 + out[0] * g; //( DREAL_MUL( out[0], f ) >> p );
	out[1] = r_resy/2 + out[1] * g; //( DREAL_MUL( out[1], f ) >> p );
	#endif
}

static const S32 *get_transformed_vertex( const S32 in[3] )
{
	#define CACHE_SIZE 4
	static S32 const *cache_p[CACHE_SIZE] = {0};
	static S32 cache_v[CACHE_SIZE][3];
	static int wr = 0;
	S32 *out;

	for( int i=0; i<CACHE_SIZE; ++i ) {
		if ( cache_p[i] == in )
			return cache_v[i];
	}

	cache_p[wr] = in;
	out = cache_v[wr];
	wr = ( wr + 1 ) % CACHE_SIZE;

	transform_vertex( out, in, current_mvp );
	return out;
	#undef CACHE_SIZE
}

static S32 edge_setup( S32 u[2], const S32 a[3], const S32 b[3], S32 x0, S32 y0, int p )
{
	S32 ux, uy;
	u[0] = ( ux = b[1] - a[1] );
	u[1] = ( uy = a[0] - b[0] );
	return (x0-a[0])*ux + (y0-a[1])*uy >> p;
}

void draw_triangle( const S32 v0[3], const S32 v1[3], const S32 v2[3], U32 color )
{
	const int p = REAL_FRACT_BITS;
	const int skip = r_pitch / 4;

	S32 u0[2], u1[2], u2[2];
	S32 d0, d1, d2;
	S32 x0, y0, x1, y1, x, y;
	U32 *dst;

	x0 = MIN( MIN( v0[0], v1[0] ), v2[0] ) >> p;
	y0 = MIN( MIN( v0[1], v1[1] ), v2[1] ) >> p;
	x1 = ( MAX( MAX( v0[0], v1[0] ), v2[0] ) >> p ) + 1;
	y1 = ( MAX( MAX( v0[1], v1[1] ), v2[1] ) >> p ) + 1;
	x0 = MAX( x0, 0 );
	y0 = MAX( y0, 0 );
	x1 = MIN( x1, r_resx );
	y1 = MIN( y1, r_resy );
	dst = r_canvas + y0 * skip;

	S32 x0p = x0 << p, y0p = y0 << p;
	d0 = edge_setup( u0, v0, v1, x0p, y0p, p );
	d1 = edge_setup( u1, v1, v2, x0p, y0p, p );
	d2 = edge_setup( u2, v2, v0, x0p, y0p, p );

	for( y=y0; y<y1; ++y ) {
		S32 d0_x0=d0, d1_x0=d1, d2_x0=d2;
		for( x=x0; x<x1; ++x ) {
			int s0 = d0 > 0;
			int s1 = d1 > 0;
			int s2 = d2 > 0;
			if ( s0 == s1 && s1 == s2 ) {
				dst[x] = color;
			}
			d0 += u0[0];
			d1 += u1[0];
			d2 += u2[0];
		}
		d0 = d0_x0 + u0[1];
		d1 = d1_x0 + u1[1];
		d2 = d2_x0 + u2[1];
		dst += skip;
	}

	draw_point( x0, y0, 0xFF );
	draw_point( x1-1, y0, 0xFF );
	draw_point( x0, y1-1, 0xFF );
	draw_point( x1-1, y1-1, 0xFF );
	draw_point( v0[0]>>p, v0[1]>>p, 0xFF00 );
	draw_point( v1[0]>>p, v1[1]>>p, 0xD600 );
	draw_point( v2[0]>>p, v2[1]>>p, 0x8000 );
}

void draw_triangle_t( const S32 v0[3], const S32 v1[3], const S32 v2[3], U32 color )
{
	const S32 *a, *b, *c;
	a = get_transformed_vertex( v0 );
	b = get_transformed_vertex( v1 );
	c = get_transformed_vertex( v2 );
	draw_triangle( a, b, c, color );
}

void draw_triangles( const S32 verts[][3], const U8 idx[], int n_idx, U32 color )
{
	int i = 0;
	do {
		draw_triangle_t( verts[idx[i]], verts[idx[i+1]], verts[idx[i+2]], color );
		i += 3;
	} while ( i < n_idx );
}

void draw_quads( const S32 verts[][3], const U8 idx[], int n_idx, U32 color )
{
	int i = 0;
	do {
		draw_triangle_t( verts[idx[i]], verts[idx[i+1]], verts[idx[i+2]], color );
		draw_triangle_t( verts[idx[i+1]], verts[idx[i+2]], verts[idx[i+3]], color );
		i += 4;
	} while ( i < n_idx );
}

// fractional bits in circle coordinates (cx,cy,r0,r)
#define CIRCLE_FB 2

// r0: interior radius
// r1: exterior radius
// max_alpha: 256 corresponds to 1.0
void draw_circle( S32 cx, S32 cy, S32 r0, S32 r, U32 color, U32 max_alpha )
{
	#define p CIRCLE_FB
	int x0, y0, x1, y1, x, y;
	// x0,y0,x1,y1: integer bounding box of pixels to be processed

	x0 = cx - r >> p;
	x0 = MAX( x0, 0 );
	x0 &= ~3; // align to 16 byte boundary (aka 4 pixels)

	x1 = cx + r + (1<<p) >> p;
	x1 = x0 + ( x1 - x0 + 7 & ~7 ); // pad width to multiple of 8
	x1 = MIN( x1, r_resx );

	if ( x1 == r_resx ) {
		// since inner loop writes 8 pixels, the last 4 pixels may overflow to the next scanline:
		// better fix: drop this and add extra padding to scanlines
		x0 = x1 - ( x1 - x0 + 7 & ~7 );
	}

	y0 = cy - r >> p;
	y0 = MAX( y0, 0 );
	y1 = cy + r + (1<<p) >> p;
	y1 = MIN( y1, r_resy );

	int skip = r_pitch >> 2;
	U32 *dst = r_canvas + y0 * skip;

	S32 half = 1<<p-1;
	S32 dx0s = ( x0 << p ) + half - cx;
	S32 dy0s = ( y0 << p ) + half - cy;

	static const S16 off[] = {0<<p,1<<p,2<<p,3<<p,4<<p,5<<p,6<<p,7<<p};
	__m128i a0, b, dx0, dy, rr;
	__m128i zero = _mm_setzero_si128();
	__m128i allset = _mm_cmpeq_epi16( allset, allset );
	rr = _mm_set1_epi16( r*r + half >> p );

	int a_bits = 6;
	__m128i a_edge = _mm_set1_epi16( r0*r0 >> p );
	__m128i a_mul = _mm_set1_epi16( (max_alpha<<2*p+a_bits) / (U32)(r*r - r0*r0) );

	__m128i one = _mm_srli_epi16( allset, 15 );
	__m128i
	dy_inc = _mm_slli_epi16( one, p ), //1<<p
	dx_inc = _mm_slli_epi16( one, 3+p ); //8<<p
	__m128i
	b_inc = dy_inc, //1<<p
	f_inc = _mm_slli_epi16( one, 6+p ); //64<<p

	// (dx0,dy): distance to centre
	dx0 = _mm_add_epi16( _mm_set1_epi16( dx0s ), _mm_load_si128( (void*) off ) );
	dy = _mm_set1_epi16( dy0s );

	// (a0,b): squared distance to centre
	a0 = _mm_srli_epi16( _mm_mullo_epi16( dx0, dx0 ), p );
	b = _mm_srli_epi16( _mm_mullo_epi16( dy, dy ), p );

	__m128i colx;
	// color as 16bit
	colx = _mm_set_epi64x( 0, (U64) color | (U64) color << 32 );
	colx = _mm_unpacklo_epi8( colx, zero );

	for( y=y0; y<y1; ++y ) {

		__m128i
		f = _mm_add_epi16( a0, b ),
		dx = dx0;

		for( x=x0; x<x1; x+=8 ) {

			__m128i m = _mm_cmplt_epi16( f, rr );
			int mi = _mm_movemask_epi8( m );

			if ( mi ) {
				__m128i alpha;
				alpha = _mm_sub_epi16( rr, _mm_max_epi16( f, a_edge ) );
				alpha = _mm_mullo_epi16( alpha, a_mul );
				alpha = _mm_srli_epi16( alpha, p+a_bits );
				//alpha = _mm_max_epi16( alpha, zero );

				for( int j=0; j<2; ++j )
				{
					__m128i c0, c1, c2, c3, c4, c6, c7, M;
					void *mem = dst + x + 4*j;

					__m128i a2, al, ah;
					a2 = _mm_unpacklo_epi16( alpha, alpha );
					al = _mm_unpacklo_epi32( a2, a2 );
					ah = _mm_unpackhi_epi32( a2, a2 );

					c0 = _mm_load_si128( mem ); // rgb.rgb.rgb.rgb.
					c1 = _mm_unpacklo_epi8( c0, zero ); // first two pixels as 16bit
					c2 = _mm_add_epi16( _mm_mullo_epi16( _mm_sub_epi16( colx, c1 ), al ),
						_mm_slli_epi16( c1, 8 ) );

					c3 = _mm_unpackhi_epi8( c0, zero ); // last two pixels as 16bit
					c4 = _mm_add_epi16( _mm_mullo_epi16( _mm_sub_epi16( colx, c3 ), ah ),
						_mm_slli_epi16( c3, 8 ) );

					c6 = _mm_packus_epi16( _mm_srli_epi16( c2, 8 ), _mm_srli_epi16( c4, 8 ) );

					M = _mm_unpacklo_epi16( m, m );
					c7 = _mm_or_si128( _mm_and_si128( M, c6 ), _mm_andnot_si128( M, c0 ) );

					_mm_store_si128( mem, c7 );
					m = _mm_srli_si128( m, 8 );
					alpha = _mm_srli_si128( alpha, 8 );
				}
			}

			f = _mm_add_epi16( f, f_inc );
			f = _mm_add_epi16( f, _mm_slli_epi16( dx, 4 ) );
			dx = _mm_add_epi16( dx, dx_inc );
		}

		b = _mm_add_epi16( b, b_inc );
		b = _mm_add_epi16( b, _mm_add_epi16( dy, dy ) );
		dy = _mm_add_epi16( dy, dy_inc );
		dst += skip;
	}
	#undef p
}

void draw_blob( const GfxBlob blob[1] )
{
	Real rx = get_rel_x( blob->x, eye_x );
	Real ry = blob->y - eye_y;
	Real z = REALF( view_scale );

	int p = CIRCLE_FB;
	int s = REAL_FRACT_BITS - p;

	S32
	x = ( r_resx/2 << p ) + ( DREAL_MUL( rx, z ) >> s ),
	y = ( r_resy/2 << p ) + ( DREAL_MUL( ry, z ) >> s );

	S32 r1 = DREAL_MUL( blob->scale_x, z ) >> s;
	S32 r0 = blob->mode == BLOB_FUZZY ? 0 : r1 - ( 1 << p - 1 );

	draw_circle( x, y, r0, r1, blob->color, 256 );
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
	
	blob.mode = BLOB_FUZZY;
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
			float p = 1 << CIRCLE_FB;
			draw_circle( xx*p, yy*p, p*2.0f, p*2.5f, 0xFFFFFF, 256 );
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

	mat_push();
	//mat_translate( r_resx*0.5f, r_resy*0.5f, 0 );
	mat_scale( view_scale, view_scale, 1 );
	//mat_translate( 1, 1, 0 );
	mat_store( the_view_mat );
	mat_pop();

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

	#define COORD_SCALE 1
	#define P_SCREEN_RATIO (r_resy/(float)r_resx)
	#define W (r_resx/HORZ_VIEW_RANGE)
	#define P_RIGHT (W/2.0)
	#define P_LEFT (-P_RIGHT)
	#define P_BOTTOM ( P_SCREEN_RATIO * W / 2.0 )
	#define P_TOP (-P_BOTTOM)
	#define P_NEAR -128.0
	#define P_FAR 128.0
	float m[] = {
		2.0 / ( P_RIGHT - P_LEFT ) * COORD_SCALE, 0, 0, 0,
	0, 2.0 / ( P_TOP - P_BOTTOM ) * COORD_SCALE, 0, 0,
	0, 0, -1.0 / ( P_FAR - P_NEAR ) * COORD_SCALE, 0,
	-( P_RIGHT + P_LEFT ) / ( P_RIGHT - P_LEFT ), -( P_TOP + P_BOTTOM ) / ( P_TOP - P_BOTTOM ), -P_NEAR / ( P_FAR - P_NEAR ), 1
	};

	memcpy( the_view_mat, m, sizeof(m) );

	mat_push();
	mat_translate( -eye_x, -eye_y, 0 );

	render_world_fg();

	mat_pop();

	if ( 1 ) {
		static float a = 0;
		float
		x = 50.5f + cosf( a ) * 40,
		y = 50.5f + sinf( a ) * 20,
		r = 20.0f + cosf( 0.3f * a ) * 15.0f,
		p = 1<<CIRCLE_FB;
		hline( y, ~0 );
		vline( x, ~0 );
		draw_circle( x*p, y*p, 0, r*p, ~0, 128 );
		a += PI/300.0f;
	}

	if ( 1 ) {
		float k = view_scale * 20;
		S32 verts[][3] = {
			#define R3(x,y,z) {REALF(r_resx/2+x*k),REALF(r_resy/2+y*k),REALF(z)}
			R3( 0.2, 0.7, 0 ),
			R3( 0.8, 0.3, 0 ),
			R3( 0.1, 0.15, 0 )
		};
		float m[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
		set_mvp_matrix_f( m );
		draw_triangle_t( verts[0], verts[1], verts[2], 0 );
	}
}

