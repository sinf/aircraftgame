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

void generate_clouds( void );

void setup2D( void )
{
	unpack_models();
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
	
	if ( type == PT_SMOKE ) {
		blob.scale_y = blob.scale_x = \
		REALF( 1.0 ) + REALF( 0.75 ) * thing->age / (unsigned)( MAX_PARTICLE_TIME * GAME_TICKS_PER_SEC );
		blob.mode = BLOB_FUZZY;
	}
	
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
	
	//draw_triangle_strip( num_verts, verts );
}

