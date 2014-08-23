#include "system.h"
#include "support.h"

#include "game.h"
#include "graphics.h"
#include "models.h"
#include "lowgfx.h"

/* You can see this many game units horizontally on the screen */
#define HORZ_VISION_RANGE 90.0f

/* Size of the biggest game object. This value is used for occlusion culling */
#define LARGEST_OBJECT_RADIUS 5.0

#define ENABLE_BLEND 1
#define ENABLE_WIREFRAME 1
#define ENABLE_GRID ENABLE_WIREFRAME

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

void setup2D( void )
{
	unpack_models();
	init_gfx();
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
	
	blob.color = RGBA_32( g, g, g, a );
	blob.x = thing->pos.x;
	blob.y = thing->pos.y;
	blob.scale_x = w;
	blob.scale_y = h;
	
	return blob;
}

static void draw_water( const Water w[1] )
{
	GfxVertex verts[(WATER_RESOL+1)*2];
	
	const U32 c_surf = RGBA_32( 0, 0, 255, 80 );
	const U32 c_bottom = RGBA_32( 0, 0, 32, 255 );
	
	S32 offset_y = REALF( W_WATER_LEVEL );
	S32 bottom = REALF( 50.0 );
	unsigned n;
	S32 pos_x = 0;
	
	for( n=0; n<WATER_RESOL; n++ ) {
		unsigned v0 = 2*n, v1 = 2*n+1;
		verts[v0].x = pos_x;
		verts[v0].y = w->z[n] + offset_y;
		verts[v0].color = c_surf;
		verts[v1].x = pos_x;
		verts[v1].y = bottom;
		verts[v1].color = c_bottom;
		pos_x += REALF( WATER_ELEM_SPACING );
	}
	
	verts[2*WATER_RESOL].x = pos_x;
	verts[2*WATER_RESOL].y = verts[0].y;
	verts[2*WATER_RESOL].color = c_surf;
	
	verts[2*WATER_RESOL+1].x = pos_x;
	verts[2*WATER_RESOL+1].y = bottom;
	verts[2*WATER_RESOL+1].color = c_bottom;
	
	draw_triangle_strip( (WATER_RESOL+1)*2, verts );
}

static void draw_sky( void )
{
	draw_box(
		0, REALF( W_WATER_LEVEL - W_HEIGHT - 64 ),
		REALF( W_WIDTH ), REALF( W_HEIGHT + 128 ),
		RGB_32( 0xd8, 0xd8, 0xac )
	);
}

static void render_world_bg( void )
{
	/* This clears the framebuffer for the first time */
	draw_sky();
}

static void render_world_fg( Real eye_x )
{
	#define MAX_MODEL_INST 512
	#define MAX_BLOBS 4096
	
	float matr[NUM_MODELS][MAX_MODEL_INST][16];
	GfxBlob blobs[MAX_BLOBS];
	
	unsigned num_inst[NUM_MODELS] = {0};
	unsigned num_blobs = 0;
	unsigned n;
	
	const Real clip_r = REALF( HORZ_VISION_RANGE/2 + LARGEST_OBJECT_RADIUS );
	const Real clip_west = eye_x - clip_r;
	const Real clip_east = eye_x + clip_r;
	
	for( n=0; n<WORLD.num_things; n++ )
	{
		Thing *t = WORLD.things + n;
		Real x = t->pos.x;
		Real y = t->pos.y;
		Real yaw = t->angle;
		Real roll = 0;
		ModelID mdl = BAD_MODEL_ID;
		
		#if 0
		if ( x < clip_west || x > clip_east )
			continue;
		#endif
		
		switch( t->type )
		{
			case T_AIRCRAFT:
				roll = yaw + REALF( PI );
				mdl = M_AIRCRAFT;
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
			if ( num_inst[mdl] < MAX_MODEL_INST )
			{
				mat_push();
				
				mat_translate( x, y, 0 );
				mat_rotate( 2, REALTOF( yaw ) );
				mat_rotate( 0, REALTOF( roll ) );
				mat_store( matr[mdl][num_inst[mdl]++] );
				
				if ( t->type == T_AIRCRAFT && t->data.ac.throttle_on ) {
					if ( num_inst[M_AIRCRAFT_FLAME] < MAX_MODEL_INST ) {
						mat_rotate( 0, REALTOF( roll ) );
						mat_store( matr[M_AIRCRAFT_FLAME][num_inst[M_AIRCRAFT_FLAME]++] );
					}
				}
				
				mat_pop();
			}
		}
		
		#if 0
		if ( t->type != T_PARTICLE && t->type != T_PROJECTILE )
			draw_crosshair( x, y );
		#endif
	}
	
	for( n=0; n<NUM_MODELS; n++ )
		draw_models( num_inst[n], n, &matr[n][0][0] );
	
	draw_blobs( num_blobs, blobs );
}

void render( void )
{
	static Real eye_x=0, eye_y=0;
	static Real px = 0;
	
	if ( WORLD.player )
	{
		eye_x = WORLD.player->pos.x;
		eye_y = WORLD.player->pos.y;
		px = WORLD.player->pos.x < REALF( W_WIDTH / 2 ) ? REALF( -W_WIDTH ) : REALF( W_WIDTH );
	}
	
	mat_push();
	mat_translate( -eye_x, -eye_y, 0 );
	
	render_world_bg();
	mat_push();
	mat_translate( px, 0, 0 );
	render_world_bg();
	mat_pop();
	
	render_world_fg( eye_x );
	mat_push();
	mat_translate( px, 0, 0 );
	render_world_fg( eye_x );
	mat_pop();
	
	draw_water( &WORLD.water );
	mat_push();
	mat_translate( px, 0, 0 );
	draw_water( &WORLD.water );
	mat_pop();
	
	mat_pop();
}
