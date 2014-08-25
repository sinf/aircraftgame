#include <stdlib.h>
#include "system.h"
#include "support.h"

#include "game.h"
#include "graphics.h"
#include "models.h"
#include "lowgfx.h"

/* You can see this many game units horizontally on the screen */
#define HORZ_VISION_RANGE 90.0f

#define VERT_VISION_RANGE (HORZ_VISION_RANGE/P_SCREEN_RATIO)

/* Size of the biggest game object. This value is used for occlusion culling */
#define LARGEST_OBJECT_RADIUS 10.0

#define ENABLE_BLEND 1
#define ENABLE_WIREFRAME 1
#define ENABLE_GRID ENABLE_WIREFRAME

static Real clip_test_x0 = 0;
static Real clip_test_x1 = 0;
static int object_is_visible( Real x ) {
	return ( clip_test_x0 <= x ) && ( x < clip_test_x1 );
}

const float COMMON_MATRICES[2][16] = {
	{1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1},
	
	/* The projection matrix is pre-scaled so that values of type Real can be passed to GL without translating/scaling */
	#define COORD_SCALE ( 1.0f / (1<<REAL_FRACT_BITS) )
	
	#define P_SCREEN_RATIO ((float)SCREEN_H/(float)SCREEN_W)
	
	#define P_RIGHT (HORZ_VISION_RANGE/2.0)
	#define P_LEFT (-P_RIGHT)
	#define P_BOTTOM ( P_SCREEN_RATIO * HORZ_VISION_RANGE / 2.0 )
	#define P_TOP (-P_BOTTOM)
	#define P_NEAR -128.0
	#define P_FAR 128.0
	
	/* Le orthographic projection matrix: */
	{2.0 / ( P_RIGHT - P_LEFT ) * COORD_SCALE, 0, 0, 0,
	0, 2.0 / ( P_TOP - P_BOTTOM ) * COORD_SCALE, 0, 0,
	0, 0, -1.0 / ( P_FAR - P_NEAR ) * COORD_SCALE, 0,
	-( P_RIGHT + P_LEFT ) / ( P_RIGHT - P_LEFT ), -( P_TOP + P_BOTTOM ) / ( P_TOP - P_BOTTOM ), -P_NEAR / ( P_FAR - P_NEAR ), 1}
};

/*
128 -> 6.5
*/

#define DRAW_CLOUDS 1
#define NUM_CLOUD_BLOBS (50*12)
static GfxBlob cloud_blobs[NUM_CLOUD_BLOBS];

static void generate_one_cloud( GfxBlob out[], unsigned num_blobs )
{
	const Real max_rotation = REALF( RADIANS( 360 ) );
	const DReal min_step = REALF( 2.0 );
	const DReal max_step = REALF( 5.0 );
	const Real tau = REALF( 2 * PI );
	Real r = REALF( 10.0 / 32.0 ) * num_blobs;
	unsigned blob = 0;
	Vec2 p;
	Real a;
	
	const unsigned g = 240; /* 220 + prng_next() % 32; */
	
	p.x = prng_next() % REALF( W_WIDTH );
	p.y = REALF( W_WATER_LEVEL - W_HEIGHT + 5 );
	a = prng_next() % tau;
	
	for( blob=0; blob<num_blobs; blob++ )
	{
		DReal step_length = min_step + prng_next() % ( max_step - min_step );
		Vec2 dir = v_sincos( a = ( a + prng_next() % max_rotation - max_rotation / 2 + tau ) % tau );
		
		out[blob].mode = BLOB_FUZZY;
		out[blob].color = RGBA_32( g, g, g, 40 );
		out[blob].x = p.x;
		out[blob].y = p.y;
		out[blob].scale_x = r;
		out[blob].scale_y = r;
		
		dir.y >>= 1;
		p = v_addmul( p, dir, step_length );
		r -= prng_next() % REALF( 0.5 );
	}
}

static void generate_clouds( void )
{
	#if 1
	GfxBlob *blobs = cloud_blobs;
	unsigned blobs_left = NUM_CLOUD_BLOBS;
	
	do {
		/* s:
		when value is small there will be lots of small and fragmented clouds
		when value is large there will be only few clouds but they are BIG!
		*/
		unsigned s = MIN( blobs_left, 30 + prng_next() % 20 );
		
		generate_one_cloud( blobs, s );
		blobs += s;
		blobs_left -= s;
	} while( blobs_left );
	#else
	generate_one_cloud( cloud_blobs, NUM_CLOUD_BLOBS );
	#endif
}

void setup2D( void )
{
	unpack_models();
	init_gfx();
	generate_clouds();
}

static GfxBlob get_particle_blob( Thing *thing )
{
	static const Real PARTICLE_SIZE[] = {
		REALF(0.5), REALF(0.5), /* water1 */
		REALF(2), REALF(1), /* water2*/
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
	
	return blob;
}

static void draw_water( void )
{
	const U32 c_surf = RGBA_32( 0, 0, 255, 80 );
	const U32 c_bottom = RGBA_32( 0, 0, 32, 255 );
	const Water *w = &WORLD.water;
	
	GfxVertex *verts;
	unsigned begin, end, n;
	unsigned num_cells, num_verts;
	
	S32 offset_y = REALF( W_WATER_LEVEL );
	S32 bottom = REALF( W_WATER_LEVEL + W_WATER_DEPTH );
	S32 pos_x;
	
	begin = MAX( WATER_CELL_AT_X( clip_test_x0 ), 0 );
	end = WATER_CELL_AT_X( clip_test_x1 );
	
	num_cells = ( end - begin + 1 );
	num_verts = 2 * num_cells;
	verts = alloca( num_verts * sizeof(*verts) );
	
	pos_x = begin * REALF( WATER_ELEM_SPACING );	
	
	for( n=0; n<num_cells; n++ ) {
		unsigned a = 2*n;
		unsigned b = a+1;
		verts[a].x = pos_x;
		verts[a].y = w->z[(begin+n)%WATER_RESOL] + offset_y;
		verts[a].color = c_surf;
		verts[b].x = pos_x;
		verts[b].y = bottom;
		verts[b].color = c_bottom;
		pos_x += REALF( WATER_ELEM_SPACING );
	}
	
	draw_triangle_strip( num_verts, verts );
}

#define MAX_MODEL_INST 512
static float model_inst_matr[NUM_MODELS][MAX_MODEL_INST][16] = {{{0}}};
static unsigned model_inst_count[NUM_MODELS] = {0};

static void push_model_mat( ModelID m )
{
	if ( model_inst_count[m] < MAX_MODEL_INST ) {
		unsigned i = model_inst_count[m]++;
		mat_store( model_inst_matr[m][i] );
	}
}

static void flush_models( void )
{
	int n;
	for( n=0; n<NUM_MODELS; n++ ) {
		draw_models( model_inst_count[n], n, &model_inst_matr[n][0][0] );
		model_inst_count[n] = 0;
	}
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
	}
	
	ASSERT( num_blobs < MAX_BLOBS );
	
	flush_models();
	draw_blobs( num_blobs, blobs );
	
	#if DRAW_CLOUDS
	draw_blobs( NUM_CLOUD_BLOBS, cloud_blobs );
	#endif
}

static void draw_wrapped( Real eye_x, void (*draw_stuff)(void) )
{
	#if 0
	/* Make it very clear what is being clipped */
	const Real view_dist = REALF( 10 );
	#else
	const Real view_dist = REALF( HORZ_VISION_RANGE/2 + LARGEST_OBJECT_RADIUS );
	#endif
	
	const Real ww = REALF( W_WIDTH );
	Real view_limit_east = eye_x + view_dist;
	Real view_limit_west = eye_x - view_dist;
	
	if ( view_limit_west < 0 ) {
		
		clip_test_x0 = 0;
		clip_test_x1 = view_limit_east;
		draw_stuff();
		
		clip_test_x0 = view_limit_west + ww;
		clip_test_x1 = ww;
		mat_push();
		mat_translate( REALF( -W_WIDTH ), 0, 0 );
		draw_stuff();
		mat_pop();
		
	} else if ( view_limit_east > ww ) {
		
		clip_test_x0 = view_limit_west;
		clip_test_x1 = ww;
		draw_stuff();
		
		clip_test_x0 = 0;
		clip_test_x1 = view_limit_east - ww;
		mat_push();
		mat_translate( REALF( W_WIDTH ), 0, 0 );
		draw_stuff();
		mat_pop();
	} else {
		clip_test_x0 = view_limit_west;
		clip_test_x1 = view_limit_east;
		draw_stuff();
	}
}

static void draw_background( void )
{
	const U32 sky_color = RGB_32( 0xd8, 0xd8, 0xac );
	const float top_extra = 120;
	Real w = REALF( W_WIDTH );
	
	draw_box(
		-w, REALF( W_WATER_LEVEL - W_HEIGHT - top_extra ),
		3 * w, REALF( W_HEIGHT + top_extra + W_WATER_DEPTH ), sky_color );
}

void render( void )
{
	static Real eye_x=0, eye_y=0;
	/*static Real px = 0;*/
	
	#if DEBUG
	extern void glClear( int );
	glClear( 0x4000 );
	#endif
	
	if ( WORLD.player )
	{
		eye_x = WORLD.player->phys.pos.x;
		eye_y = WORLD.player->phys.pos.y;
	}
	
	mat_push();
	mat_translate( -eye_x, -eye_y, 0 );
	
	draw_background();
	draw_wrapped( eye_x, render_world_fg );
	draw_wrapped( eye_x, draw_water );
	
	draw_box(
		REALF( -W_WIDTH ), REALF( W_WATER_LEVEL + W_WATER_DEPTH ),
		REALF( 3 * W_WIDTH ), REALF( 100 ), RGB_32( 0, 0, 0 ) );
	
	mat_pop();
}
