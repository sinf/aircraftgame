#include "system.h"
#include "support.h"
#include "models.h"
#include "opengl.h"

/*
	VERTICES: 144
	INDICES: 237
	144+237 = 381
*/

/* Maximum vertices per ANY mesh */
#define MAX_VERTS 32

static const U8 COLORS[NUM_MODELS][4] = {
	#define C(r,g,b,a) {r,g,b,a}
	C(0,0,0,255), /* aircraft */
	C(0,0,0,255), /* radar */
	C(0,0,0,255), /* gunship */
	C(0,0,0,255), /* battleship */
	C(0,0,0,255), /* debris */
	C(0,0,0,255), /* aa gun barrel */
	C(255,255,255,255) /* aircraft flame */
};

/* The vertex data of all models in one big array */
static const S8 VERTICES[] = {
	/* aircraft */ -91,-108,15,-91,32,15,126,-72,8,126,17,8,60,81,-1,-9,51,-1,24,-91,126,-9,-91,126,60,-128,22,60,81,22,-58,-17,2,-115,-17,2,-89,-68,44,-128,-68,50,-128,126,-1,-78,126,-1,126,17,-1,-91,32,-1,126,-72,-1,-91,-108,-1,
	/* radar */ -2,-37,-128,-32,-57,-47,76,-62,-128,91,-96,-47,-32,-57,-1,91,-96,-1,57,80,-26,32,-128,-1,86,71,-1,-1,-95,-26,-44,-95,-26,-128,25,-26,-128,-95,-26,-84,25,-26,-128,-128,-1,
	/* gunship */ 117,46,126,-48,-119,46,-128,-48,-67,-107,-67,-84,-50,-107,-50,-84,-21,-107,-38,-107,-38,-84,-21,-84,-9,-84,-9,-107,-107,-107,-107,-84,-107,-128,-107,-48,-9,-48,-9,-128,
	/* battleship */ 109,-35,126,-61,-123,-8,-128,-61,-37,-111,-37,-94,-28,-111,-28,-94,-12,-111,-21,-111,-21,-94,-12,-94,-5,-94,-5,-111,-59,-111,-59,-94,-59,-128,-59,-61,42,-61,42,-128,96,36,-123,36,3,-111,3,-94,26,-111,26,-94,35,-111,35,-94,42,-94,42,-111,
	/* debris */ -104,-128,-14,-87,-12,-112,126,16,-128,109,-99,-31,-128,-18,126,-111,97,29,102,126,12,85,10,110,
	/* aagun barrel */ 0,-19, 80,-19, 80,20, 0,20,
	/* aircraft flame */ -50,0, -70,-25, -100,0, -70,24
};

/* All quad indices. An arithmetic encoder might be able to compress this a little bit */
static const U8 INDICES[] = {
	/* aircraft */ 6,4,5,7,2,3,9,8,12,13,11,10,10,11,14,15,18,16,3,2,0,1,17,19,3,16,4,9,9,4,17,1,1,0,8,9,
	/* radar */ 1,0,2,3,9,6,8,7,13,10,12,11,9,7,14,12,3,5,4,1,
	/* gunship */ 1,0,2,3,7,10,9,6,16,14,13,19,11,12,13,8,18,12,15,17,14,15,5,4,
	/* battleship */ 1,0,2,3,7,10,9,6,29,14,16,19,11,12,13,8,23,25,24,22,2,0,20,21,14,15,5,4,27,28,29,26,28,15,17,18,
	/* debris */ 1,0,4,5,5,6,2,1,6,7,3,2,0,3,7,4,7,6,5,4,0,1,2,3,
	/* aagun barrel */ 0,1,2,3,
	/* aircraft flame */ 0,1,2,3
};

/* MODEL_INFO_T flags */
#define F_NONE 0
#define F_2D 1
#define F_MIRROR 2

static const struct MODEL_INFO_T {
	U8 scale[3];
	U8 flags;
	U8 num_verts;
	U8 num_indices;
} MODEL_INFO[NUM_MODELS] = {
	#define FIX_SCALE 4
	#define FIX(x) ((unsigned)((x)*FIX_SCALE))
	#define MODEL(sx,sy,sz,flags,numv,numi,post_scale) {{FIX(sx*post_scale),FIX(sy*post_scale),FIX(sz*post_scale)},flags,numv,numi}
	MODEL( 3.02247, 1.17181, 3.5087, F_MIRROR, 20, 36, 0.3 ), /* aircraft */
	MODEL( 1, 0.942285, 0.671835, F_MIRROR, 15, 20, 0.8 ), /* radar */
	MODEL( 3.07596, 2.2489, 0, F_2D, 20, 24, 1.0 ), /* gunship */
	MODEL( 5.57596, 2.85614, 0, F_2D, 30, 36, 1.0 ), /* battleship */
	MODEL( 0.85931, 0.56695, 1.55972, F_NONE, 8, 24, 0.4 ), /* debris */
	MODEL( 2, 1, 0, F_2D, 4, 4, 1 ), /* aagun barrel */
	MODEL( 2, 1, 0, F_2D, 4, 4, 1 ) /* aircraft flame */
};

#define DIMENSIONS(mesh) (3-((mesh).flags & F_2D))

static GLint vertex_arrays[NUM_MODELS][MAX_VERTS*3];
static U8 const *index_arrays[NUM_MODELS];

static void unpack_bytes( const S8 *in_p, GLint *out_p, int count, int step, int scale )
{
	int n;
	for( n=0; n<count; n++ )
	{
		/* Works if REAL_FRACT_BITS=8 */
		*out_p = ( (*in_p) * scale ) >> 1;
		
		out_p += step;
		in_p += step;
	}
}

void unpack_models( void )
{
	const S8 *vert_p = VERTICES;
	const U8 *index_p = INDICES;
	U8 m;
	
	for( m=0; m<NUM_MODELS; m++ )
	{
		int dim = DIMENSIONS( MODEL_INFO[m] );
		int n_verts = MODEL_INFO[m].num_verts;
		int axis;
		
		for( axis=0; axis<dim; axis++ )
		{
			int scale = MODEL_INFO[m].scale[axis];
			unpack_bytes( vert_p+axis, vertex_arrays[m]+axis, n_verts, dim, scale );
		}
		
		vert_p += n_verts * dim;
		
		index_arrays[m] = index_p;
		index_p += MODEL_INFO[m].num_indices;
	}
}

void draw_model( ModelID m, Real x, Real y, Real r_yaw, Real r_roll )
{
	U8 num_quads = MODEL_INFO[m].num_indices >> 2;
	U8 const *index_p = index_arrays[m];
	
	translate2( x, y );
	rotate( DEGREES( REALTOF( r_yaw ) ), 0, 0, 1 );
	rotate( DEGREES( REALTOF( r_roll ) ), 1, 0, 0 );
	
	set_color_p( COLORS[m] );
	set_vertex_pointer( vertex_arrays[m], DIMENSIONS( MODEL_INFO[m] ), VT_INT );
	draw_quads_indexed( index_p, 1, num_quads );
	
	if ( MODEL_INFO[m].flags & F_MIRROR )
	{
		scale3( 1, 1, -1 );
		draw_quads_indexed( index_p, 1, num_quads );
	}
}
