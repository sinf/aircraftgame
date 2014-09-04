#include "system.h"
#include "support.h"
#include "models.h"

/*
	VERTICES: 144
	INDICES: 237
	144+237 = 381
*/

/* The vertex data of all models in one big array */
static const S8 MODEL_VERTICES[] = {
	/* aircraft */ -91,-108,15,-91,32,15,126,-72,8,126,17,8,60,81,-1,-9,51,-1,24,-91,126,-9,-91,126,60,-128,22,60,81,22,-58,-17,2,-115,-17,2,-89,-68,44,-128,-68,50,-128,126,-1,-78,126,-1,126,17,-1,-91,32,-1,126,-72,-1,-91,-108,-1,
	/* radar */ -2,-37,-128,-32,-57,-47,76,-62,-128,91,-96,-47,-32,-57,-1,91,-96,-1,57,80,-26,32,-128,-1,86,71,-1,-1,-95,-26,-44,-95,-26,-128,25,-26,-128,-95,-26,-84,25,-26,-128,-128,-1,
	/* gunship */ 117,46,126,-48,-119,46,-128,-48,-67,-107,-67,-84,-50,-107,-50,-84,-21,-107,-38,-107,-38,-84,-21,-84,-9,-84,-9,-107,-107,-107,-107,-84,-107,-128,-107,-48,-9,-48,-9,-128,
	/* battleship */ -111,-21,-128,-36,121,-5,126,-36,35,-65,35,-55,26,-65,26,-55,10,-65,19,-65,19,-55,10,-55,3,-55,3,-65,57,-65,57,-55,57,-75,57,-36,-44,-36,-44,-75,-98,20,121,20,-5,-65,-5,-55,-20,-65,-20,-55,-37,-65,-37,-55,-44,-55,-44,-65,35,-128,38,-128,38,-75,29,-111,26,-111,29,-75,
	/* debris */ -104,-128,-14,-87,-12,-112,126,16,-128,109,-99,-31,-128,-18,126,-111,97,29,102,126,12,85,10,110,
	/* aagun barrel */ 0,-19, 80,-19, 80,20, 0,20,
	/* aircraft flame */ -50,0, -70,-25, -100,0, -70,24,
	/* heli body */ -101,-10,-1,42,-22,18,85,-18,12,24,75,-1,35,-20,18,24,75,33,-94,28,7,-109,37,0,-99,20,52,-111,29,52,-128,126,0,-112,126,0,85,-18,-1,-52,-37,-1,-1,14,23,-9,-20,92,15,-18,126,15,-121,9,24,-121,9,12,-92,37,15,-121,37,24,-121,37,26,-92,37,12,-92,9,26,-92,9,-126,-39,0,-124,-57,0,-118,-57,0,-116,-39,0,-126,-39,12,-124,-57,12,-118,-57,12,-116,-39,12,62,-38,-1,-9,-51,83,-9,-23,70,48,-51,83,48,-23,70,48,-20,92,
	/* heli rotor */ -128,126,115,-117,126,126,-128,104,115,-117,104,126,115,126,126,126,126,115,115,104,126,126,104,115,-1,126,9,9,126,-1,-1,104,9,9,104,-1,-9,104,-1,-11,104,-1,-1,104,7,-11,126,-1,7,-1,-1,7,126,-1,-1,-1,7,-9,-1,-1,
	/* fighter jet */ 56,-43,26,-59,-26,26,74,41,12,-54,20,26,126,-24,-1,126,-17,-1,26,-1,78,11,-1,78,21,15,-1,-31,-61,126,-53,-54,126,-54,20,5,-63,126,-1,-74,116,-1,-59,-26,-1
};

/* All quad indices. An arithmetic encoder might be able to compress this a little bit */
static const U8 MODEL_INDICES[] = {
	/* aircraft */ 6,4,5,7,2,3,9,8,12,13,11,10,10,11,14,15,18,16,3,2,0,1,17,19,3,16,4,9,9,4,17,1,1,0,8,9,
	/* radar */ 1,0,2,3,9,6,8,7,13,10,12,11,9,7,14,12,3,5,4,1,
	/* gunship */ 1,0,2,3,7,10,9,6,16,14,13,19,11,12,13,8,18,12,15,17,14,15,5,4,
	/* battleship */ 2,3,1,0,7,10,9,6,29,14,16,19,11,12,13,8,23,25,24,22,20,21,2,0,14,15,5,4,27,28,29,26,28,15,17,18,32,4,30,31,35,33,34,6,
	/* debris */ 1,0,4,5,5,6,2,1,6,7,3,2,0,3,7,4,7,6,5,4,0,1,2,3,
	/* aagun barrel */ 0,1,2,3,
	/* aircraft flame */ 0,1,2,3,
	/* heli body */ 1,4,5,2,8,9,7,6,2,5,3,12,0,13,33,5,20,21,22,19,20,17,18,21,21,18,24,22,28,25,29,32,25,26,30,29,26,27,31,30,27,28,32,31,29,30,31,32,19,23,17,20,0,26,25,6,1,18,17,4,23,19,22,24,1,33,13,14,4,16,15,14,14,6,0,13,11,10,0,6,12,2,1,33,34,35,37,36,35,15,38,37,15,34,36,38,
	/* heli rotor */ 0,2,3,1,15,13,2,0,9,8,4,5,4,6,7,5,8,10,6,4,9,17,15,8,1,3,10,8,5,7,11,9,8,15,0,1,19,18,14,12,18,16,17,14,
	/* fighter jet */ 2,0,4,5,11,8,9,10,14,4,0,1,11,8,12,13,3,11,14,1,2,3,1,0,8,2,6,7
};

const MODEL_INFO_T MODEL_INFO[NUM_MODELS] = {
	#define FIX_SCALE 4
	#define FIX(x) ((unsigned)((x)*FIX_SCALE))
	#define MODEL(sx,sy,sz,flags,numv,numi,post_scale,color) {{FIX(sx*post_scale),FIX(sy*post_scale),FIX(sz*post_scale)},flags,numv,numi,color}
	#define BLACK RGBA_32( 0, 0, 0, 255 )
	#define WHITE RGBA_32( 255, 255, 255, 255 )
	MODEL( 3.02247, 1.17181, 3.5087, F_MIRROR, 20, 36, 0.3, BLACK ), /* aircraft */
	MODEL( 1, 0.942285, 0.671835, F_MIRROR, 15, 20, 0.8, BLACK ), /* radar */
	MODEL( 3.07596, 2.2489, 0, F_2D, 20, 24, 1.0, BLACK ), /* gunship */
	MODEL( 5.57596, 4.91751, 0, F_2D, 36, 44, 1.0, BLACK ), /* battleship */
	MODEL( 0.85931, 0.56695, 1.55972, F_NONE, 8, 24, 0.4, BLACK ), /* debris */
	MODEL( 2, 1, 0, F_2D, 4, 4, 1, BLACK ), /* aagun barrel */
	MODEL( 2, 1, 0, F_2D, 4, 4, 1, WHITE ), /* aircraft flame */
	MODEL( 3.8154, 1.45016, 2.32799, F_MIRROR, 39, 96, 0.4, BLACK ), /* heli body */
	MODEL( 2.14872, 0.664598 * 2, 2.14872, F_MIRROR, 20, 44, 0.4, BLACK ), /* heli rotor */
	MODEL( 1.30707, 0.430332, 0.709963, F_MIRROR, 15, 28, 1.4, BLACK ) /* fighter jet */
};

S32 model_vertices_unpacked[NUM_MODELS][MAX_MODEL_VERTS*3];
U8 const *model_indices_unpacked[NUM_MODELS];

static void unpack_bytes( const S8 *in_p, S32 *out_p, int count, int step, int scale )
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
	const S8 *vert_p = MODEL_VERTICES;
	const U8 *index_p = MODEL_INDICES;
	U8 m;
	
	for( m=0; m<NUM_MODELS; m++ )
	{
		int dim = MODEL_DIMENSIONS( MODEL_INFO[m] );
		int n_verts = MODEL_INFO[m].num_verts;
		int axis;
		
		for( axis=0; axis<dim; axis++ )
		{
			int scale = MODEL_INFO[m].scale[axis];
			unpack_bytes( vert_p+axis, model_vertices_unpacked[m]+axis, n_verts, dim, scale );
		}
		
		ASSERT( n_verts <= MAX_MODEL_VERTS );
		vert_p += n_verts * dim;
		
		if ( MODEL_INFO[m].flags & F_MIRROR )
		{
			S32 *src = model_vertices_unpacked[m];
			S32 *dst = model_vertices_unpacked[m] + n_verts * 3;
			int v;
			
			ASSERT( 2*n_verts <= MAX_MODEL_VERTS );
			
			for( v=0; v<n_verts; v++ ) {
				dst[3*v+0] = src[3*v+0];
				dst[3*v+1] = src[3*v+1];
				dst[3*v+2] = -src[3*v+2];
			}
		}
		
		model_indices_unpacked[m] = index_p;
		index_p += MODEL_INFO[m].num_indices;
	}
}
